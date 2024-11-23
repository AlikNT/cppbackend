#include "sdk.h"

#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "app.h"
#include "infrastructure.h"

using namespace std::literals;
namespace net = boost::asio;
namespace fs = std::filesystem;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

struct Args {
    int tick_period = 0;
    std::string config_file;
    std::string www_root;
    std::string state_file;
    int state_period = 0;
    bool randomize_spawn_points = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"Allowed options"s};
    Args args;

    desc.add_options()
            ("help,h", "produce help message")
            ("tick-period,t", po::value<int>(&args.tick_period)->value_name("milliseconds"s), "set tick period")
            ("config-file,c", po::value<std::string>(&args.config_file)->value_name("file"s), "set config file path")
            ("www-root,w", po::value<std::string>(&args.www_root)->value_name("dir"s), "set static files root")
            ("state-file", po::value<std::string>(&args.state_file)->value_name("file"s), "set state file path")
            ("state-period", po::value<int>(&args.state_period)->value_name("milliseconds"s), "set state period")
            ("randomize-spawn-points", po::bool_switch(&args.randomize_spawn_points), "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }
    // Проверяем наличие опций config-file  и www-root
    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file path has not been specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static files path has not specified"s);
    }

    return args;
}

}  // namespace

int main(int argc, const char* argv[]) {
    server_logging::InitLogging();
    try {
        if (auto args = ParseCommandLine(argc, argv)) {
            // 1. Загружаем карту из файла и создаем модель игры
            model::Game game = json_loader::LoadGame(args->config_file);
            app::Application app(game);
            std::unique_ptr<infrastructure::SerializingListener> listener = nullptr;
            std::unique_ptr<sig::scoped_connection> conn = nullptr;
            if (!args->state_file.empty()) {
                listener = std::make_unique<infrastructure::SerializingListener>(app, milliseconds(args->state_period), args->state_file);
                listener->Load();
                // Подключаем метод Save как обработчик сигнала tick
                conn = std::make_unique<sig::scoped_connection>(app.DoOnTick([&listener](milliseconds delta) {
                    listener->Save(delta);
                }));
            }

            // 2. Инициализируем io_context
            const unsigned num_threads = std::thread::hardware_concurrency();
            net::io_context ioc(num_threads);

            // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
            net::signal_set signals(ioc, SIGINT, SIGTERM);
            signals.async_wait([&ioc, &args, &listener](const sys::error_code &ec, [[maybe_unused]] int signal_number) {
                if (!ec) {
                    ioc.stop();
                    if (!args->state_file.empty()) {
                        listener->Save();
                    }
                    server_logging::LogServerExit(0, "");
                }
            });

            // strand для выполнения запросов к API
            using Strand = net::strand<net::io_context::executor_type>;
            Strand api_strand = net::make_strand(ioc);

            // Создаем обработчик запросов в куче, управляемый shared_ptr
            std::shared_ptr<http_handler::RequestHandler> handler;
            handler = std::make_shared<http_handler::RequestHandler>(game, app, fs::path{args->www_root}, api_strand, args->tick_period);
            server_logging::LoggingRequestHandler logging_handler{handler};

            // Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
            const auto address = net::ip::make_address("0.0.0.0");
            constexpr net::ip::port_type port = 8080;

            // Запускаем обработку запросов
            http_server::ServeHttp(ioc, {address, port},
                                   [&logging_handler](auto &&endpoint, auto &&req, auto &&send) {
                                       logging_handler(std::forward<decltype(endpoint)>(endpoint),
                                                       std::forward<decltype(req)>(req),
                                                       std::forward<decltype(send)>(send));
                                   });

            server_logging::LogServerStart(port, address.to_string());

            if (args->tick_period) {
                // Настраиваем вызов метода Application::Tick каждые 50 миллисекунд внутри strand
                std::chrono::milliseconds tick_period_ms(args->tick_period);
                auto ticker = std::make_shared<http_handler::Ticker>(api_strand, tick_period_ms,
                                                                     [&app](std::chrono::milliseconds delta) {
                                                                         app.Tick(delta);
                                                                     }
                );
                ticker->Start();
            }

            // 6. Запускаем обработку асинхронных операций
            RunWorkers(std::max(1u, num_threads), [&ioc] {
                ioc.run();
            });
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        server_logging::LogServerExit(EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }
}

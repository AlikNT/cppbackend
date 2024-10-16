#include "sdk.h"
//
#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"

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

}  // namespace

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <file name>"sv << std::endl;
        return EXIT_FAILURE;
    }
    server_logging::InitLogging();
    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
                server_logging::LogServerExit(0, "");
            }
        });

        // strand для выполнения запросов к API
        using Strand = net::strand<net::io_context::executor_type>;
        Strand api_strand = net::make_strand(ioc);

        // Создаем обработчик запросов в куче, управляемый shared_ptr
        std::shared_ptr<http_handler::RequestHandler> handler;
        handler = std::make_shared<http_handler::RequestHandler>(game, fs::path{argv[2]}, api_strand);
        server_logging::LoggingRequestHandler logging_handler{handler};

        // Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        // Запускаем обработку запросов
        http_server::ServeHttp(ioc, {address, port},
       [&logging_handler](auto&& endpoint, auto&& req, auto&& send){
                       logging_handler(std::forward<decltype(endpoint)>(endpoint),
                       std::forward<decltype(req)>(req),
                       std::forward<decltype(send)>(send));
               });

        server_logging::LogServerStart(port, address.to_string());

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        server_logging::LogServerExit(EXIT_FAILURE, ex.what());
        return EXIT_FAILURE;
    }
}

#include "seabattle.h"

#include <atomic>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;
using namespace std::literals;

void PrintFieldPair(const SeabattleField& left, const SeabattleField& right) {
    auto left_pad = "  "s;
    auto delimeter = "    "s;
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
    for (size_t i = 0; i < SeabattleField::field_size; ++i) {
        std::cout << left_pad;
        left.PrintLine(std::cout, i);
        std::cout << delimeter;
        right.PrintLine(std::cout, i);
        std::cout << std::endl;
    }
    std::cout << left_pad;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << delimeter;
    SeabattleField::PrintDigitLine(std::cout);
    std::cout << std::endl;
}

template <size_t sz>
static std::optional<std::string> ReadExact(tcp::socket& socket) {
    boost::array<char, sz> buf;
    boost::system::error_code ec;

    net::read(socket, net::buffer(buf), net::transfer_exactly(sz), ec);

    if (ec) {
        return std::nullopt;
    }

    return {{buf.data(), sz}};
}

static bool WriteExact(tcp::socket& socket, std::string_view data) {
    boost::system::error_code ec;

    net::write(socket, net::buffer(data), net::transfer_exactly(data.size()), ec);

    return !ec;
}

class SeabattleAgent {
public:
    SeabattleAgent(const SeabattleField& field)
        : my_field_(field) {
    }

    void StartGame(tcp::socket& socket, bool my_initiative) {
        while (!IsGameEnded()) {
            if (my_initiative) {
                PrintFields();
                // Ввод координат для выстрела
                std::string move;
                std::cout << "Your turn (for example, B4): "s;
                std::cin >> move;

                auto parsed_move = ParseMove(move);
                if (!parsed_move) {
                    std::cout << "Incorrect input. Try again."s << std::endl;
                    continue;
                }

                // Отправляем ход сопернику
                if (!WriteExact(socket, move)) {
                    std::cerr << "Error sending move."s << std::endl;
                    break;
                }

                // Ждем результат выстрела
                auto result = ReadExact<1>(socket);
                if (!result) {
                    std::cerr << "Error while getting shot result."s << std::endl;
                    break;
                }

                SeabattleField::ShotResult shot_result = static_cast<SeabattleField::ShotResult>(result->at(0));

                // Обрабатываем результат
                switch (shot_result) {
                    case SeabattleField::ShotResult::MISS:
                        other_field_.MarkMiss(parsed_move->first, parsed_move->second);
                        std::cout << "Miss!"s << std::endl;
                        my_initiative = false;
                        break;
                    case SeabattleField::ShotResult::HIT:
                        other_field_.MarkHit(parsed_move->first, parsed_move->second);
                        std::cout << "Hit!"s << std::endl;
                        break;
                    case SeabattleField::ShotResult::KILL:
                        other_field_.MarkKill(parsed_move->first, parsed_move->second);
                        std::cout << "Kill!"s << std::endl;
                        break;
                }
            } else {
                PrintFields();

                // Ожидаем ход соперника
                std::cout << "Waiting for turn..." << std::endl;
                auto move = ReadExact<2>(socket);
                if (!move) {
                    std::cerr << "Error when receiving opponent's move." << std::endl;
                    break;
                }

                auto parsed_move = ParseMove(*move);
                if (!parsed_move) {
                    std::cerr << "Incorrect data from the opponent." << std::endl;
                    break;
                }

                std::cout << "Shot to "s << *move << "("s << parsed_move->first << ", "s << parsed_move->second << ")"s << std::endl;

                // Выполняем выстрел по своему полю
                auto shot_result = my_field_.Shoot(parsed_move->first, parsed_move->second);

                // Отправляем результат сопернику
                if (!WriteExact(socket, std::string(1, static_cast<char>(shot_result)))) {
                    std::cerr << "Error sending shot result."s << std::endl;
                    break;
                }

                // Обновляем поле и выводим на экран
//                PrintFields();

                if (shot_result == SeabattleField::ShotResult::MISS) {
                    my_initiative = true;
                }
            }
        }
        if (my_field_.IsLoser()) {
            std::cout << "You lost!" << std::endl;
        } else if(other_field_.IsLoser()) {
            std::cout << "You won!" << std::endl;
        }
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

//        std::cout << p1 << " "s << p2 << std::endl;

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p2, p1}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char x = static_cast<char>(move.first);
        char y = static_cast<char>(move.second);
        x += 'A';
        y += '1';
        char buff[] = {x, y};
        return {buff, 2};
    }

    void PrintFields() const {
        PrintFieldPair(my_field_, other_field_);
    }

    bool IsGameEnded() const {
        return my_field_.IsLoser() || other_field_.IsLoser();
    }

    // TODO: добавьте методы по вашему желанию

private:
    SeabattleField my_field_;
    SeabattleField other_field_;
};

void StartServer(const SeabattleField& field, unsigned short port) {
    SeabattleAgent agent(field);

    net::io_context io_context;

    // используем конструктор tcp::v4 по умолчанию для адреса 0.0.0.0
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..." << std::endl;

    boost::system::error_code ec;
    tcp::socket socket(io_context);
    acceptor.accept(socket, ec);

    if (ec) {
        throw boost::system::system_error(ec, "Can't accept connection");
    }

//    std::cout << "Client connected. Game starts!" << std::endl;

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    boost::system::error_code ec;
    auto endpoint = tcp::endpoint(net::ip::make_address(ip_str, ec), port);

    if (ec) {
        throw boost::system::system_error(ec, "Wrong IP format");
    }

    net::io_context io_context;
    tcp::socket socket(io_context);

    socket.connect(endpoint, ec);

//    std::cout << "Connecting to the server. The game begins!" << std::endl;

    agent.StartGame(socket, true);
};

int main(int argc, const char** argv) {
    if (argc != 3 && argc != 4) {
        std::cout << "Usage: program <seed> [<ip>] <port>" << std::endl;
        return 1;
    }

    std::mt19937 engine(std::stoi(argv[1]));
    SeabattleField fieldL = SeabattleField::GetRandomField(engine);

    if (argc == 3) {
        StartServer(fieldL, std::stoi(argv[2]));
    } else if (argc == 4) {
        StartClient(fieldL, argv[2], std::stoi(argv[3]));
    }
}

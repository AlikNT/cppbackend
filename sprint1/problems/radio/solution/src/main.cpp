// когда используем заголовочные файлы библиотеки Boost.Asio
#ifdef WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio.hpp>
#include "audio.h"
#include <iostream>
#include <string>

namespace net = boost::asio;
using net::ip::udp;

using namespace std::literals;

void StartClient(const std::string &server_ip, unsigned short port) {
    try {
        net::io_context io_context;
        udp::socket socket(io_context, udp::v4());

        boost::system::error_code ec;
        auto receiver_endpoint = udp::endpoint(net::ip::make_address(server_ip, ec), port);

        Recorder recorder(ma_format_u8, 1);

        auto rec_result = recorder.Record(65000, 1.5s);
        std::cout << "Recording done" << std::endl;

        std::cout << "Sending audio data to " << server_ip << ":" << port << std::endl;
        socket.send_to(net::buffer(rec_result.data, rec_result.frames * recorder.GetFrameSize()),
                       receiver_endpoint);
        std::cout << "Audio data sent" << std::endl;
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void StartServer(unsigned short port) {
    try {
        net::io_context io_context;
        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        Player player(ma_format_u8, 1);
        std::vector<char> recv_buffer(65000); // Buffer to receive audio data

        while (true) {
            udp::endpoint remote_endpoint;
            size_t len = socket.receive_from(net::buffer(recv_buffer), remote_endpoint);

            std::cout << "Received audio data from " << remote_endpoint << std::endl;
            const size_t frames = len / player.GetFrameSize();
            player.PlayBuffer(recv_buffer.data(), frames, 1.5s);
            std::cout << "Playing done" << std::endl;
        }
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <client|server> <port>" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    unsigned short port = static_cast<unsigned short>(std::stoi(argv[2]));

    if (mode == "client") {
        while (true) {
            std::string server_ip;
            std::cout << "Input IP address and press Enter to record message..." << std::endl;
            std::getline(std::cin, server_ip);
            StartClient(server_ip, port);
        }
    } else if (mode == "server") {
        StartServer(port);
    } else {
        std::cerr << "Unknown mode: " << mode << std::endl;
        return 1;
    }

    return 0;
}

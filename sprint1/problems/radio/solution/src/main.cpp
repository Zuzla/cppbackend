#include "audio.h"
#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <string_view>

namespace net = boost::asio;
using namespace std::literals;
using net::ip::udp;

int StartServer(int port)
{
    std::string str;
    Player player(ma_format_u8, 1);

    const int start_size = 1024;

    net::io_context io;
    boost::system::error_code ec;

    udp::socket socket(io, udp::endpoint(udp::v4(), port));
    std::string res;

    std::cout << "Start to get audio!" << std::endl;
    while(1)
    {
        std::array<char, start_size> buf;
        udp::endpoint endpoint;

        auto size = socket.receive_from(net::buffer(buf), endpoint);

        if (std::string_view(buf.data(),4) == "End!")
            break;

        res += std::move(buf.data());
    }
    std::cout << "End to get audio!" << std::endl;

    auto count_frames = res.size() / player.GetFrameSize();
    
    player.PlayBuffer(res.c_str(), count_frames, 1.5s);
    std::cout << "Playing done" << std::endl;

    
    return 0;
    
}

int StartClient(int port)
{
    std::string str;
    Recorder recorder(ma_format_u8, 1);

    const int start_size = 1024;

    net::io_context io;
    boost::system::error_code ec;
    udp::socket socket(io, udp::v4());
    auto endpoint = udp::endpoint(net::ip::make_address("127.0.0.1", ec),port);

    // Record
    std::cout << "Press Enter to record message..." << std::endl;
    std::getline(std::cin, str);

    auto rec_result = recorder.Record(65000, 1.5s);
    std::cout << "Recording done" << std::endl;

    auto size_record = rec_result.frames * recorder.GetFrameSize();
    std::string data = std::move(rec_result.data.data());

    // Send
    std::cout << "Start send!" << std::endl;

    while (data.size() != 0)
    {
        if (data.size() > 1024)
        {
            socket.send_to(net::buffer(data.substr(0, 1024)), endpoint);
            data.erase(0, 1024);
        }
        else
        {
            socket.send_to(net::buffer(data.substr(0, data.size())), endpoint);
            data.erase(0, data.size());
        }
    }
    std::cout << "End send!" << std::endl;
    socket.send_to(net::buffer("End!"), endpoint);
    

    return 0;
}

int main(int argc, char* argv[]) {
    
    if (argc != 3)
        return -2;

    std::string str = argv[1];
    int port = std::atoi(argv[2]);

    try
    {
        if (str == "server")
        {
            StartServer(port);
        }
        else if (str == "client")
        {
            StartClient(port);
        }
        else
        {
            std::cout << "Error arguments" << std::endl;
            return -1;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }


/*
    while (true) {
        std::string str;

        std::cout << "Press Enter to record message..." << std::endl;
        std::getline(std::cin, str);

        auto rec_result = recorder.Record(65000, 1.5s);
        std::cout << "Recording done" << std::endl;

        player.PlayBuffer(rec_result.data.data(), rec_result.frames, 1.5s);
        std::cout << "Playing done" << std::endl;
    }
*/
    return 0;
}

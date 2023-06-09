#include <boost/asio.hpp>
#include <array>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
// TCP больше не нужен, импортируем имя UDP
using net::ip::udp;

using namespace std::literals;

int main()
{
    static const int PORT = 3333;
    static const int size_buffer = 1024;

    try
    {
        boost::asio::io_context io;

        udp::socket socket(io, udp::endpoint(udp::v4(), PORT));

        while(1)
        {
            std::array<char, size_buffer> recv;
            udp::endpoint remote_end_pornt;

            auto size = socket.receive_from(boost::asio::buffer(recv), remote_end_pornt);

            std::cout << "Client said: "sv << std::string_view(recv.data(), size) << std::endl;

            boost::system::error_code ignored_error;
            socket.send_to(boost::asio::buffer("Hellow world UDP-server"sv), remote_end_pornt, 0, ignored_error);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}
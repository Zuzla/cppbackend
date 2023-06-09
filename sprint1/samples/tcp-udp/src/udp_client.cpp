#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
using net::ip::udp;

using namespace std::literals;

int main(int argc, char** argv)
{
    static const int port = 3333;
    static const int size_buffer = 1024;

    if (argc != 2)
    {
        return -1;
    }

    
    try
    {
        net::io_context io;
        udp::socket socket(io, udp::v4());

        boost::system::error_code ec;

        auto endpoint = udp::endpoint(net::ip::make_address(argv[1], ec), port);
        socket.send_to(net::buffer("Hello for UDP-Clietn"), endpoint);

        std::array<char, size_buffer> buffer;
        udp::endpoint sender_endpoint;
        size_t size = socket.receive_from(net::buffer(buffer), sender_endpoint);

        std::cout << "Server responsed "sv << std::string_view(buffer.data(), size) << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
}
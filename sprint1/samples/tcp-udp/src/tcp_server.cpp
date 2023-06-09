#include <iostream>
#include <string>
#include <string_view>

#include <boost/asio.hpp>

namespace net = boost::asio;
using net::ip::tcp;

using namespace std::literals;

int main()
{
    static const int port = 3333;

    net::io_context io;

    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port));
    std::cout << "Waiting for connection..."sv << std::endl;

    boost::system::error_code ec;
    tcp::socket socket{io};
    acceptor.accept(socket, ec);

    if (ec)
    {
        std::cout << "Accept error - " << ec.message() << std::endl;
        return 1;
    }

    net::streambuf buf;
    net::read_until(socket, buf, '\n', ec);
    std::string client_data{std::istreambuf_iterator<char>(&buf), std::istreambuf_iterator<char>()};

    if (ec)
    {
        std::cout << "Error reading data "sv << ec.message() << std::endl;
        return 2;
    }

    std::cout << "Client said: "sv << client_data << std::endl;

    socket.write_some(net::buffer("Hello, I'm server!\n"sv), ec);


    if (ec)
    {
        std::cout << "Error write to client data "sv << ec.message() << std::endl;
        return 3;
    }
    
}
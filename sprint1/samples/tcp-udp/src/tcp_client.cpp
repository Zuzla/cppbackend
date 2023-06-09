#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace net = boost::asio;
using net::ip::tcp;

using namespace std::literals;

int main(int argc, char** argv)
{
    net::io_context io;

    static const int port = 3333;

    if (argc != 2)
    {
        std::cout << "Error with argumenst" << std::endl;
    }

    boost::system::error_code ec;

    auto end_point = tcp::endpoint(net::ip::make_address(argv[1], ec), port);

    if (ec)
    {
        std::cout << "Error in endpoint " << ec.message() << std::endl;
        return 1;
    }

    tcp::socket socket{io};
    socket.connect(end_point, ec);

    if (ec)
    {
        std::cout << "Error in socket connect " << ec.message() << std::endl;
        return 2;
    }

    socket.write_some(net::buffer("Hi, I'm client!\n"sv), ec);

    if (ec)
    {
        std::cout << "Error in write " << ec.message() << std::endl;
        return 3;
    }

    net::streambuf buf;
    net::read_until(socket, buf, '\n', ec);
    std::string server_data{std::istreambuf_iterator<char>(&buf), std::istreambuf_iterator<char>()};

    if (ec)
    {
        std::cout << "Error in read " << ec.message() << std::endl;
        return 4;
    }

    std::cout << "server responded: "sv << server_data << std::endl;
}
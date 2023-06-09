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

    void StartGame(tcp::socket& socket, bool my_initiative) 
    {
        const size_t size_send = 2;
        const size_t size_recv = 1;

        while(!IsGameEnded())
        {
            PrintFields();

            // My step
            if (my_initiative)
            {
                std::cout << std::endl << "Enter your step: ";
                std::string step;
                std::cin >> step;

                auto error = WriteExact(socket, step);
                
                auto str = ReadExact<size_recv>(socket).value();

                auto data = std::stoi(str);
                
                auto point = ParseMove(step).value();

                if (data == SeabattleField::ShotResult::KILL)
                {
                    other_field_.MarkKill(point.second, point.first);
                    std::cout << "Your kill: " << step << std::endl;
                }
                else if (data == SeabattleField::ShotResult::HIT)
                {
                    other_field_.MarkHit(point.second, point.first);
                    std::cout << "Your Hit: " << step << std::endl;
                }
                else if (data == SeabattleField::ShotResult::MISS)
                {
                    other_field_.MarkMiss(point.second, point.first);
                    std::cout << "Your Miss: " << step << std::endl;
                    my_initiative = !my_initiative;
                }
            }
            // Enemy step
            else
            {
                auto data = ReadExact<size_send>(socket);
                auto point = ParseMove(data.value()).value();

                auto result = my_field_.Shoot(point.second, point.first);

                if (result == SeabattleField::ShotResult::HIT)
                {
                    WriteExact(socket, std::to_string(SeabattleField::ShotResult::HIT));
                    std::cout << "Hit: " << data.value() << std::endl;
                }
                else if (result == SeabattleField::ShotResult::KILL)
                {
                    WriteExact(socket, std::to_string(SeabattleField::ShotResult::KILL));
                    std::cout << "Kill: " << data.value() << std::endl;
                }
                else if (result == SeabattleField::ShotResult::MISS)
                {
                    WriteExact(socket, std::to_string(SeabattleField::ShotResult::MISS));
                    std::cout << "Miss: " << data.value() << std::endl;
                    my_initiative = !my_initiative;
                }

            }
        }
    }

private:
    static std::optional<std::pair<int, int>> ParseMove(const std::string_view& sv) {
        if (sv.size() != 2) return std::nullopt;

        int p1 = sv[0] - 'A', p2 = sv[1] - '1';

        if (p1 < 0 || p1 > 8) return std::nullopt;
        if (p2 < 0 || p2 > 8) return std::nullopt;

        return {{p1, p2}};
    }

    static std::string MoveToString(std::pair<int, int> move) {
        char buff[] = {static_cast<char>(move.first) + 'A', static_cast<char>(move.second) + '1'};
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

    net::io_context io;

    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), port));

    boost::system::error_code ec;

    tcp::socket socket{io};

    acceptor.accept(socket, ec);

    if (ec)
    {
        std::cout << "1 - Error code - " << ec.message() << std::endl;
        return;
    }

    agent.StartGame(socket, false);
};

void StartClient(const SeabattleField& field, const std::string& ip_str, unsigned short port) {
    SeabattleAgent agent(field);

    net::io_context io;
    boost::system::error_code ec;

    auto end_point = tcp::endpoint(net::ip::make_address(ip_str, ec), port);

    if (ec)
    {
        std::cout << "2 - Error code - " << ec.message() << std::endl;
        return;
    }

    tcp::socket socket{io};

    socket.connect(end_point, ec);

    if (ec)
    {
        std::cout << "3 - Error code - " << ec.message() << std::endl;
        return;
    }

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

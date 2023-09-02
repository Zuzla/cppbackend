#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "logging_request_handler.h"
#include "boost/beast.hpp"

namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
namespace http = boost::beast::http;

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
        std::cerr << "Usage: game_server <game-config-json>"sv << std::endl;
        return EXIT_FAILURE;
    }
    try {
        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(argv[1]);
        app::Players players;

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        // Подписываемся на сигналы и при их получении завершаем работу сервера
        net::signal_set signals(ioc, SIGINT, SIGTERM);

        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                json::value custom_data_stop{{"code"s, 0}};
                BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, custom_data_stop) << "server exited"sv;
                ioc.stop();
            }        
        });

        // strand для выполнения запросов к API
        auto api_strand = net::make_strand(ioc);

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto handler = std::make_shared<http_handler::RequestHandler>(game, players, std::move(argv[2]), api_strand);

        // Оборачиваем его в логирующий декоратор
        server_logging::LoggingRequestHandler logger_handler{
            [handler](auto&& req, auto&& send) 
            {
                (*handler)
                (std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
            }
        };


        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, logger_handler);
        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        
        json::value custom_data_start{{"port"s, port}, {"address"s, address.to_string()}};
        BOOST_LOG_TRIVIAL(info) << boost::log::add_value(additional_data, custom_data_start) << "server started"sv;

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        json::value custom_data_stop{{"code"s, EXIT_FAILURE}, {"exception"s, ex.what()}};
        BOOST_LOG_TRIVIAL(error) << boost::log::add_value(additional_data, custom_data_stop) << "server exited"sv;
        return EXIT_FAILURE;
    }
}

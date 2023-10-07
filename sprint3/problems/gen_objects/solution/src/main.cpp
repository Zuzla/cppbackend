#include "sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "logging_request_handler.h"
#include "boost/beast.hpp"
#include "ticker.h"
#include "loot_generator.h"
#include "loots.h"

namespace net = boost::asio;
using namespace std::literals;
namespace sys = boost::system;
namespace http = boost::beast::http;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned thread, const Fn& fn) {
    thread = std::max(1u, thread);
    std::vector<std::jthread> workers;
    workers.reserve(thread - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--thread) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

namespace 
{    
    std::tuple<model::Game, add_data::GameLoots>
    CreateNewGame(const bool is_debug, const bool default_spawn_,
                  const std::filesystem::path &json_path)
    {
        model::Game game{is_debug, default_spawn_};
        add_data::GameLoots game_loots{};
        json_loader::LoadGameData(std::move(game), std::move(game_loots), json_path);

        return std::make_tuple(game, game_loots);
    }
}  // namespace

struct Args {
    uint32_t tick_period;
    std::filesystem::path config_file;
    std::filesystem::path www_root;
    bool randomize_spawn_points = true;
}; 

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};

    Args args;

    auto add = desc.add_options();
    add("help,h", "Show help");
    add("tick-period,t", po::value<uint32_t>(&args.tick_period), "Set tick period");
    add("config-file,c", po::value<std::filesystem::path>(&args.config_file), "Set config file path");
    add("www-root,w", po::value<std::filesystem::path>(&args.www_root), "Set static files root");

    // variables_map хранит значения опций после разбора
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) 
    {
        std::cout << desc;
        return std::nullopt;
    }

    if (!vm.contains("config-file"s)) 
    {
        throw std::runtime_error("Configuration file not specified"s);
    }

    if (!vm.contains("www-root"s)) 
    {
        throw std::runtime_error("The path to static files is not set"s);
    }

    if (!vm.contains("tick-period"s)) 
    {
        args.tick_period = 0;
    }

    if (!vm.contains("randomize-spawn-points"s)) 
    {
        args.randomize_spawn_points = false;
    }

    // С опциями программы всё в порядке, возвращаем структуру args
    return args;
}


int main(int argc, const char* argv[]) {
        
    try {

        std::optional<Args> args = ParseCommandLine(argc, argv);
        if (args == std::nullopt) 
        {       
            return EXIT_FAILURE;
        }

        // 1. Загружаем карту из файла и построить модель игры
        //model::Game game = json_loader::LoadGame(argv[1]);
        auto [game, game_loots] =
            CreateNewGame(args->tick_period == 0, args->randomize_spawn_points,
                          args->config_file);
        //model::Game game = CreateNewGame(true, false, args->config_file);

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
        auto handler = std::make_shared<http_handler::RequestHandler>(game, players, game_loots, args->www_root, api_strand);

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

       // Настраиваем вызов метода app::Tick каждые 50 миллисекунд внутри strand
        auto ticker = std::make_shared<app::Ticker>(
            api_strand, 50ms, [&game, &game_loots](std::chrono::milliseconds delta)
            {
                if (!game.IsDebug())
                {
                    game.Tick(delta, game_loots);
                }

            });
        ticker->Start();

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

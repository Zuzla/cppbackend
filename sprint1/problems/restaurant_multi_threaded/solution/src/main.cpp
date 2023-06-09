#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <syncstream>
#include <thread>

namespace net = boost::asio;
namespace sys = boost::system;
namespace ph = std::placeholders;
using namespace std::chrono;
using namespace std::literals;
using Timer = net::steady_timer;

namespace {

class Hamburger {
public:
    [[nodiscard]] bool IsCutletRoasted() const {
        return cutlet_roasted_;
    }
    void SetCutletRoasted() {
        if (IsCutletRoasted()) {  // Котлету можно жарить только один раз
            throw std::logic_error("Cutlet has been roasted already"s);
        }
        cutlet_roasted_ = true;
    }

    [[nodiscard]] bool HasOnion() const {
        return has_onion_;
    }
    // Добавляем лук
    void AddOnion() {
        if (IsPacked()) {  // Если гамбургер упакован, класть лук в него нельзя
            throw std::logic_error("Hamburger has been packed already"s);
        }
        AssureCutletRoasted();  // Лук разрешается класть лишь после прожаривания котлеты
        has_onion_ = true;
    }

    [[nodiscard]] bool IsPacked() const {
        return is_packed_;
    }
    void Pack() {
        AssureCutletRoasted();  // Нельзя упаковывать гамбургер, если котлета не прожарена
        is_packed_ = true;
    }

private:
    // Убеждаемся, что котлета прожарена
    void AssureCutletRoasted() const {
        if (!cutlet_roasted_) {
            throw std::logic_error("Bread has not been roasted yet"s);
        }
    }

    bool cutlet_roasted_ = false;  // Обжарена ли котлета?
    bool has_onion_ = false;       // Есть ли лук?
    bool is_packed_ = false;       // Упакован ли гамбургер?
};

std::ostream& operator<<(std::ostream& os, const Hamburger& h) {
    return os << "Hamburger: "sv << (h.IsCutletRoasted() ? "roasted cutlet"sv : " raw cutlet"sv)
              << (h.HasOnion() ? ", onion"sv : ""sv)
              << (h.IsPacked() ? ", packed"sv : ", not packed"sv);
}

class Logger {
public:
    explicit Logger(std::string id)
        : id_(std::move(id)) {
    }

    void LogMessage(std::string_view message) const {
        std::osyncstream os{std::cout};
        os << id_ << "> ["sv << duration<double>(steady_clock::now() - start_time_).count()
           << "s] "sv << message << std::endl;
    }

private:
    std::string id_;
    steady_clock::time_point start_time_{steady_clock::now()};
};

class ThreadChecker {
public:
    explicit ThreadChecker(std::atomic_int& counter)
        : counter_{counter} {
    }

    ThreadChecker(const ThreadChecker&) = delete;
    ThreadChecker& operator=(const ThreadChecker&) = delete;

    ~ThreadChecker() {
        // assert выстрелит, если между вызовом конструктора и деструктора
        // значение expected_counter_ изменится
        assert(expected_counter_ == counter_);
    }

private:
    std::atomic_int& counter_;
    int expected_counter_ = ++counter_;
};

// Функция, которая будет вызвана по окончании обработки заказа
using OrderHandler = std::function<void(sys::error_code ec, int id, Hamburger* hamburger)>;



class Order : public std::enable_shared_from_this<Order>
{
public:
    Order(net::io_context& io, const int order_id, bool with_onion, OrderHandler handler) : 
        _io{io}, 
        _order_id{order_id}, 
        _with_onion{with_onion}, 
        _handler{std::move(handler)} 
    {

    }

    void Execute()
    {
        logger.LogMessage("Order has been started!"sv);
        RoastCutlet();
        if (_with_onion)
            MarinadeOnion();

    }

private:
    void RoastCutlet()
    {
        logger.LogMessage("Roast Cutlet!"sv);
        _roast_timer.async_wait([self = shared_from_this()](sys::error_code ec)
        {
            self->OnRoasted(ec);
        });

    }

    void OnRoasted(sys::error_code ec)
    {
        ThreadChecker checker{_counter};
        logger.LogMessage("On Roasted!"sv);
        if (ec) {
            logger.LogMessage("Roast error : "s + ec.what());
        } else {
            logger.LogMessage("Cutlet has been roasted."sv);
            _hamburger.SetCutletRoasted();
        }
        CheckReadiness(ec);
    }

    void MarinadeOnion()
    {
        logger.LogMessage("Marinade Onion!"sv);
        _marinade_timer.async_wait([self = shared_from_this()](sys::error_code ec)
        {
            self->OnMarinade(ec);
        });
    };

    void OnMarinade(sys::error_code ec)
    {
        ThreadChecker checker{_counter};
        logger.LogMessage("On Marinade!"sv);
        if (ec) {
            logger.LogMessage("Marinade onion error: "s + ec.what());
        } else {
            logger.LogMessage("Onion has been marinaded."sv);
            _onion_marinaded = true;
        }
        CheckReadiness(ec);
    }

    void CheckReadiness(sys::error_code ec)
    {
        if (_delivered) {
            // Выходим, если заказ уже доставлен либо клиента уведомили об ошибке
            return;
        }
        if (ec) {
            // В случае ошибки уведомляем клиента о невозможности выполнить заказ
            return Deliver(ec);
        }

        // Самое время добавить лук
        if (CanAddOnion()) {
            logger.LogMessage("Add onion"sv);
            _hamburger.AddOnion();
        }

        // Если все компоненты гамбургера готовы, упаковываем его
        if (IsReadyToPack()) {
            Pack();
        }
    }

    void Deliver(sys::error_code ec) {
        // Защита заказа от повторной доставки
        _delivered = true;
        // Доставляем гамбургер в случае успеха либо nullptr, если возникла ошибка
        _handler(ec, _order_id, ec ? nullptr : &_hamburger);
    }

    [[nodiscard]] bool CanAddOnion() const {
        // Лук можно добавить, если котлета обжарена, лук замаринован, но пока не добавлен
        return _hamburger.IsCutletRoasted() && _onion_marinaded && !_hamburger.HasOnion();
    }

    [[nodiscard]] bool IsReadyToPack() const {
        // Если котлета обжарена и лук добавлен, как просили, гамбургер можно упаковывать
        return _hamburger.IsCutletRoasted() && (!_with_onion || _hamburger.HasOnion());
    }

    void Pack() {
        logger.LogMessage("Packing"sv);

        // Просто потребляем ресурсы процессора в течение 0,5 с.
        auto start = steady_clock::now();
        while (steady_clock::now() - start < 500ms) {
        }

        _hamburger.Pack();
        logger.LogMessage("Packed"sv);

        Deliver({});
    }

    bool _onion_marinaded = false;
    bool _delivered = false;

    net::io_context& _io;
    const int _order_id;
    bool _with_onion;
    OrderHandler _handler;
    Logger logger{std::to_string(_order_id)};
    net::steady_timer _roast_timer{_io, 1s};
    net::steady_timer _marinade_timer{_io, 2s};
    Hamburger _hamburger;
    std::atomic_int _counter{0};
};

class Restaurant {
public:
    explicit Restaurant(net::io_context& io)
        : io_(io) {
    }

    int MakeHamburger(bool with_onion, OrderHandler handler) {
        const int order_id = ++next_order_id_;
        std::make_shared<Order>(io_, order_id, with_onion, std::move(handler))->Execute();
        return order_id;
    }

private:
    net::io_context& io_;
    int next_order_id_ = 0;
};

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

int main() {
    const unsigned num_workers = 4;
    net::io_context io(num_workers);

    Restaurant restaurant{io};

    Logger logger{"main"s};

    struct OrderResult {
        sys::error_code ec;
        Hamburger hamburger;
    };

    std::unordered_map<int, OrderResult> orders;

    // Обработчик заказа может быть вызван в любом из потоков, вызывающих io.run().
    // Чтобы избежать состояния гонки при обращении к orders, выполняем обращения к orders через
    // strand, используя функцию dispatch.
    auto handle_result
        = [strand = net::make_strand(io), &orders](sys::error_code ec, int id, Hamburger* h) {
              net::dispatch(strand, [&orders, id, res = OrderResult{ec, ec ? Hamburger{} : *h}] {
                  orders.emplace(id, res);
              });
          };

    const int num_orders = 16;
    for (int i = 0; i < num_orders; ++i) {
        restaurant.MakeHamburger(i % 2 == 0, handle_result);
    }

    assert(orders.empty());
    RunWorkers(num_workers, [&io] {
        io.run();
    });
    assert(orders.size() == num_orders);

    for (const auto& [id, order] : orders) {
        assert(!order.ec);
        assert(order.hamburger.IsCutletRoasted());
        assert(order.hamburger.IsPacked());
        assert(order.hamburger.HasOnion() == (id % 2 != 0));
    }
}

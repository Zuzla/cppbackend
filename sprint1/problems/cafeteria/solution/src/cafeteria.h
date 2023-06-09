#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <thread>

#include <syncstream>
#include <iostream>
#include <sstream>
#include <syncstream>
#include <thread>

#include "hotdog.h"
#include "result.h"
#include "ingredients.h"

namespace net = boost::asio;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;


class Logger {
public:
    explicit Logger(int id)
        : id_(std::move(id)) {
    }

    void LogMessage(std::string_view message) const {
        std::osyncstream os{std::cout};
        os << id_ << "> [" << std::chrono::duration<double>(std::chrono::steady_clock::now() - start_time_).count()
           << "s] " << message << std::endl;
    }

private:
    int id_;
    std::chrono::steady_clock::time_point start_time_{std::chrono::steady_clock::now()};
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

class Order : public std::enable_shared_from_this<Order>
{
public:

    Order(net::io_context& io, int id, std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread, GasCooker& gas_cooker, HotDogHandler handler)
    : _io{io},
        _id{id},
        _sausage{sausage},
        _bread{bread},
        _gas_cooker{gas_cooker},
        _handler(std::move(handler))
    {
        
    }

    void Cook()
    {     
        _sausage->StartFry(_gas_cooker, [self = shared_from_this()]
        {
            self->_sausage_timer.expires_after(std::chrono::milliseconds(1500));

            self->_sausage_timer.async_wait([self](sys::error_code ec)
            {
                self->_sausage->StopFry();
                self->OrderComplete();
            });

        });

        _bread->StartBake(_gas_cooker, [self = shared_from_this()]
        {
            self->_bread_timer.expires_after(std::chrono::milliseconds(1000));

            self->_bread_timer.async_wait([self](sys::error_code ec)
            {
                self->_bread->StopBaking();
                self->OrderComplete();
            });

        });
        
    }

private:
  

    void OrderComplete()
    {
        std::lock_guard lk{_mutex};
        if (_sausage->IsCooked() && _bread->IsCooked())
        {
            _handler(HotDog{_id, _sausage, _bread});
        }
    }

    HotDogHandler _handler;

    net::io_context& _io;

    int _id;

    GasCooker& _gas_cooker;
    std::shared_ptr<Sausage> _sausage;
    std::shared_ptr<Bread> _bread;

    net::strand<net::io_context::executor_type> _strand{net::make_strand(_io)};

    net::steady_timer _sausage_timer{_strand};
    net::steady_timer _bread_timer{_strand};

    Logger _logger{_id};
    std::mutex _mutex;
};

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria 
{
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) 
    {
        net::dispatch(_strand, [this, handler = std::move(handler)]() mutable
        {
            //assert(_strand.running_in_this_thread());
            auto r = std::make_shared<Order>(io_, ++_order_id, store_.GetSausage(), store_.GetBread(), *gas_cooker_, std::move(handler));
            r->Cook();  
        });  
    }


private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
    int _order_id = 0;    
    net::steady_timer _timer{io_};
    net::strand<net::io_context::executor_type> _strand{net::make_strand(io_)};

};

#pragma once
#include <iostream>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

template <typename T, typename = typename std::enable_if<!std::is_same<T, std::string>::value, typename T::iterator>::type>
std::ofstream& operator << (std::ofstream& os, T const& t)
{
    for(auto it = std::begin(t); it != std::end(t); it++){
            os << *it;
    }
    return os;
}

class Logger
{
    auto GetTime() const
    {
        if (manual_ts_)
        {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const
    {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const
    {
        const auto t_c = std::chrono::system_clock::to_time_t(GetTime());
        std::stringstream ss;
        ss << std::put_time(std::localtime(&t_c), "%Y_%m_%d");
        return ss.str();
    }

    Logger() = default;
    Logger(const Logger &) = delete;

public:
    static Logger &GetInstance()
    {
        static Logger obj;
        return obj;
    }

    template <typename T>
    int log_helper(T &&t)
    {
        this->file_log_ << t;
        return 0;
    }

    // Выведите в поток все аргументы.
    template <typename... Ts>
    void Log(const Ts &...args)
    {
        auto new_file_log_name_  = "/var/log/sample_log_"s + GetFileTimeStamp() + ".log";     

        std::lock_guard lk{mutex_};

        if (new_file_log_name_ != file_log_name_)
        {
            if (file_log_.is_open())
                file_log_.close();
                        
            file_log_.open(new_file_log_name_, std::ios::out);
            file_log_name_ = new_file_log_name_;
        }


        file_log_ << GetTimeStamp() << ": ";
        auto y = {log_helper(args)...};
        file_log_ << "\n";
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts)
    {
        std::lock_guard lk{mutex_};
        manual_ts_ = std::move(ts);
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::mutex mutex_;
    std::string file_log_name_;
    std::ofstream file_log_;
};


#include <iostream>
#include <chrono>

namespace timer
{
    class DurationMeasure {
    public:
        DurationMeasure() = default;
        ~DurationMeasure() = default;
        std::chrono::_V2::system_clock::rep GetTime()
        {
            end_ts_ = std::chrono::system_clock::now();
            return (end_ts_ - start_ts_).count();
        }

    private:
        std::chrono::system_clock::time_point start_ts_ = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point end_ts_;
    }; 
}
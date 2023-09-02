#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильт
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/date_time.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <string_view>

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "Time", boost::posix_time::ptime)

BOOST_LOG_ATTRIBUTE_KEYWORD(file, "File", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(line, "Line", int) 

using namespace std::literals;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;

void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    // выводим атрибут LineID
    strm << logging::extract<unsigned int>("LineID", rec) << ": ";

    // выводим уровень, заключая его в угловые скобки
    strm << "<" << rec[logging::trivial::severity] << "> ";

    // выводим само сообщение
    strm << rec[logging::expressions::smessage];
} 

void InitBoostLogFilter() {
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info
    );
    
} 

int main() {
    //InitBoostLogFilter();
    int log_num = 1;
    logging::add_file_log(
        logging::keywords::file_name = "sample.log",
        logging::keywords::format = &MyFormatter,
        logging::keywords::open_mode = std::ios_base::app | std::ios_base::out,
        // ротируем по достижению размера 10 мегабайт
        logging::keywords::rotation_size = 10 * 1024 * 1024,
        // ротируем ежедневно в полдень
        logging::keywords::time_based_rotation = sinks::file::rotation_at_time_point(12, 0, 0)
    );



    BOOST_LOG_TRIVIAL(trace) << "Сообщение уровня trace"sv;
    BOOST_LOG_TRIVIAL(debug) << "Сообщение уровня debug"sv;
    BOOST_LOG_TRIVIAL(info) << logging::add_value(file, __FILE__) << logging::add_value(line, __LINE__)  << "Сообщение уровня info"sv;
    BOOST_LOG_TRIVIAL(warning) << "Сообщение уровня warning"sv;
    BOOST_LOG_TRIVIAL(error) << "Сообщение уровня error"sv;
    BOOST_LOG_TRIVIAL(fatal) << "Сообщение уровня fatal"sv;
} 
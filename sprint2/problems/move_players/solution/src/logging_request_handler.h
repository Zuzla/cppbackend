#include "request_handler.h"
#include "time.h"

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/date_time.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/beast.hpp>

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)

namespace server_logging
{
    namespace http = boost::beast::http;
    using namespace std::literals;
    namespace logging = boost::log;
    namespace net = boost::asio;
    using tcp = net::ip::tcp;

    template<class SomeRequestHandler>
    class LoggingRequestHandler
    {
        // Запрос, тело которого представлено в виде строки
        using StringRequest = http::request<http::string_body>;
        // Ответ, тело которого представлено в виде строки
        using StringResponse = http::response<http::string_body>;
        // Ответ, тело которого представлено в виде файла
        using FileResponse = http::response<http::file_body>;
        // Ответ, тело которого представлено в виде файла
        using EmptyResponse = http::response<http::empty_body>;
        //
        using FileRequestResult = std::variant<EmptyResponse, StringResponse, FileResponse>;

    public:

        LoggingRequestHandler(SomeRequestHandler decorated) : decorated_(decorated)
        {      
            
            logging::add_console_log
            (
                //logging::keywords::file_name = "game_server.log", 
                std::cout,
                logging::keywords::format = &MyFormatter,
                logging::keywords::auto_flush = true
            );
                        
        }

        template <typename Body, typename Allocator, typename Send>
        auto operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) 
        {
            start_ts_ = std::chrono::system_clock::now();
            LogRequest(req);
            decorated_(std::move(req), [this, &send](auto&& res)
            {
                auto end_ts_ = std::chrono::system_clock::now();
                this->LogResponse(res, (end_ts_ - this->start_ts_).count());
                send(res);
            });
            int u = 0;
        }

    private:
        SomeRequestHandler decorated_;
        std::chrono::system_clock::time_point start_ts_;

        static std::string GetFileTimeStamp()
        {
            const auto t_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::stringstream ss;
            ss << std::put_time(std::localtime(&t_c), "%Y-%m-%dT%H:%M:%S");
            return ss.str();
        }

        static void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm)
        {           
            auto message = rec[logging::expressions::smessage].get().c_str();
            auto custom_data = logging::extract<boost::json::value>("AdditionalData", rec);
            auto data = custom_data.get().as_object();
            auto time = GetFileTimeStamp();
            boost::json::value log
            {
                {"timestamp"s, time},
                {"data"s, data},
                {"message"s, message}
            };

            strm << log << std::endl;
        }

        void LogRequest(const StringRequest& r)
        {
            
            boost::json::value custom_data
            {
                {"ip"s, boost::asio::ip::tcp::endpoint().address().to_string()}, 
                {"uri"s, r.target().data()}, 
                {"method"s, boost::beast::http::to_string(r.base().method())}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data) << "request received"sv;

        }

        void LogResponse(const FileResponse& r, const std::chrono::_V2::system_clock::rep& time)
        {            
            boost::json::value custom_data
            {
                {"response_time"s, time}, 
                {"code"s, r.version()}, 
                {"content_type"s, "file"}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data) << "response sent"sv;   
        }

        void LogResponse(const EmptyResponse& r, const std::chrono::_V2::system_clock::rep& time)
        {            
            boost::json::value custom_data
            {
                {"response_time"s, "end_time.total_milliseconds()"}, 
                {"code"s, "EmptyBody"}, 
                {"content_type"s, "EmptyBody"}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data) << "response sent"sv;   
        }

        void LogResponse(const StringResponse& r,  const std::chrono::_V2::system_clock::rep& time)
        {            
            boost::json::value custom_data
            {
                {"response_time"s, "end_time.total_milliseconds()"}, 
                {"code"s, r.version()}, 
                {"content_type"s, r.body()}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data) << "response sent"sv;            
        }
    };
}
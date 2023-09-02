#include "request_handler.h"

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

using namespace std::literals;
namespace logging = boost::log;

namespace http_handler
{

    template<class SomeRequestHandler>
    class LoggingRequestHandler
    {
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
            auto custom_data = logging::extract<json::value>("AdditionalData", rec);
            auto data = custom_data.get().as_object();
            auto time = GetFileTimeStamp();
            boost::json::value log
            {
                {"timestamp"s, time},
                {"data"s, data},
                {"message"s, message}
            };

            strm << log;
        }

        void LogRequest(const StringRequest& r)
        {
            json::value custom_data
            {
                {"ip"s, boost::asio::ip::tcp::endpoint().address().to_string()}, 
                {"URI"s, r.target().data()}, 
                {"method"s, boost::beast::http::to_string(r.base().method())}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data) << "request received"sv;

        }

        void LogResponse(const FileResponse& r)
        {
            auto end_time = start_time - boost::posix_time::second_clock::local_time();
            
            std::string_view content_type = "";

            for (const auto& h : r.base()) 
            {
                const boost::beast::string_view name = h.name_string(); 

                if (name == "Content-Type")
                {
                    content_type = h.value();
                    break;
                }
            }

            json::value custom_data
            {
                {"response_time"s, end_time.total_milliseconds()}, 
                {"code"s, (uint32_t)r.base().result()}, 
                {"content_type"s, content_type}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data) << "response sent"sv;   
        }

        void LogResponse(const StringResponse& r)
        {
            auto end_time = start_time - boost::posix_time::second_clock::local_time();          

            std::string_view content_type = "";

            for (const auto& h : r.base()) 
            {
                const boost::beast::string_view name = h.name_string(); 

                if (name == "Content-Type")
                {
                    content_type = h.value();
                    break;
                }
            }

            json::value custom_data
            {
                {"response_time"s, end_time.total_milliseconds()}, 
                {"code"s, (uint32_t)r.base().result()}, 
                {"content_type"s, content_type}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data) << "response sent"sv;            
        }


    public:

        LoggingRequestHandler(SomeRequestHandler& decorated) : decorated_(decorated)
        {            
            logging::add_console_log(
                std::cout, 
                logging::keywords::format = &MyFormatter,
                logging::keywords::auto_flush = true
                ); 
        }

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) 
        {
            LogRequest(req);
            start_time = boost::posix_time::second_clock::local_time();
            // Не знаю как сделать иначе, потрачено много времени, иду дальше. Переделать!
            decorated_(std::forward<decltype(req)>(req), [this, &send](auto&& response)
            {          
                this->LogResponse(std::move(response));      
                send(std::move(response));
            });
        }

    private:
        boost::posix_time::ptime start_time;
        SomeRequestHandler& decorated_;
    };
}
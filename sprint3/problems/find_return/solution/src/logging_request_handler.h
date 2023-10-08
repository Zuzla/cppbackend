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
                std::cout,
                logging::keywords::format = &MyFormatter,
                logging::keywords::auto_flush = true
            );
                        
        }

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) 
        {
            auto start_ts_ = std::chrono::system_clock::now();
            
            LogRequest(req);

            decorated_(std::move(req), [this, start_ts_, &send](auto&& res)
            {
                auto end_ts_ = std::chrono::system_clock::now();
                auto diff = duration_cast<std::chrono::milliseconds>(end_ts_ - start_ts_);
                this->LogResponse(res, diff.count());
                send(res);
            });

        }

    private:
        SomeRequestHandler decorated_;

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
                {"URI"s, r.target().data()}, 
                {"method"s, boost::beast::http::to_string(r.base().method())}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data) << "request received"sv;

        }

        void LogResponse(const FileResponse& r, const int64_t& time)
        {            
           std::string_view content_type = "";

            for (const auto& h : r.base()) 
            {
                const boost::beast::string_view name = h.name_string(); 

                if (name == "Content-Type")
                {
                    content_type = h.value().data();
                    break;
                }
            }

            boost::json::value custom_data
            {
                {"response_time"s, time}, 
                {"code"s, static_cast<uint32_t>(r.base().result())}, 
                {"content_type"s, content_type}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data) << "response sent"sv;   
        }

        void LogResponse(const EmptyResponse& r, const int64_t& time)
        {            
            boost::json::value custom_data
            {
                {"response_time"s, time}, 
                {"code"s, "EmptyBody"}, 
                {"content_type"s, "EmptyBody"}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data) << "response sent"sv;   
        }

        void LogResponse(const StringResponse& r, const int64_t& time)
        {
           std::string_view content_type = "";

            for (const auto& h : r.base()) 
            {
                const boost::beast::string_view name = h.name_string(); 

                if (name == "Content-Type")
                {
                    content_type = h.value().data();
                    break;
                }
            }

            boost::json::value custom_data
            {
                {"response_time"s, time}, 
                {"code"s, static_cast<uint32_t>(r.base().result())}, 
                {"content_type"s, content_type}
            };
            BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, custom_data) << "response sent"sv;            
        }
    };
}
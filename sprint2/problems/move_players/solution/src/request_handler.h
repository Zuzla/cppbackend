#pragma once
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "player.h"
#include "responce_api.h"
#include "http_server.h"
#include "model.h"

#include <boost/json.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>
#include <boost/asio.hpp>

#include <string>
#include <filesystem>
#include <map>
#include <variant>

namespace http_handler 
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;
    namespace fs = std::filesystem;
    namespace sys = boost::system;
    namespace net = boost::asio;

    using namespace std::literals;
    
    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view TEXT_HTML = "text/html";
        constexpr static std::string_view TEXT_CSS = "text/css";
        constexpr static std::string_view TEXT_TXT = "text/plain";
        constexpr static std::string_view TEXT_JS = "text/javascript";
        constexpr static std::string_view TEXT_JSON = "application/json";
        constexpr static std::string_view TEXT_XML = "application/xml";
        constexpr static std::string_view TEXT_PNG = "image/png";
        constexpr static std::string_view TEXT_JPG = "image/jpeg";
        constexpr static std::string_view TEXT_GIF = "image/gif";
        constexpr static std::string_view TEXT_BMP = "image/bmp";
        constexpr static std::string_view TEXT_ICO = "image/vnd.microsoft.icon";
        constexpr static std::string_view TEXT_TIF = "image/tiff";
        constexpr static std::string_view TEXT_SVG = "image/svg+xml";
        constexpr static std::string_view TEXT_MP3 = "audio/mpeg";
        constexpr static std::string_view TEXT_UNKNOWN = "application/octet-stream";
    };

    class RequestHandler : public std::enable_shared_from_this<RequestHandler>
    {
    public:
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
        //
        using Strand = net::strand<net::io_context::executor_type>;

        RequestHandler(model::Game& game, app::Players& players, std::string game_file_path, Strand api_strand)
            : game_{game}, players_{players}, game_file_path_(game_file_path), api_strand_{api_strand}
            {                }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) 
        {           
            auto version = req.version();
            auto keep_alive = req.keep_alive();

                auto url = req.target();
                url.remove_prefix(1);

                if (url.empty() || url == "")
                {
                    url = "index.html"sv;
                }


                std::string str = {url.begin(), url.end()};

                if (std::strstr(url.data(), "api"))
                {
                    auto handle = [self = shared_from_this(), send, req = std::forward<decltype(req)>(req), str, api = std::make_shared<ResponseApi>(game_, players_)] 
                    {             
                        auto res = api->Request(std::move(req), str);       
                        return send(res);
                    };
                    return net::dispatch(api_strand_, handle);
                    
                }

                // Возвращаем результат обработки запроса к файлу
                return std::visit(
                    [&send](auto&& result) {
                        send(std::forward<decltype(result)>(result));
                }, RequestFile(req, str));     
        }


    private:

        FileRequestResult RequestFile(const StringRequest& req, fs::path path);
        StringResponse ReportServerError(const StringRequest& req);

        // возвращает Json с ошибкой
        std::string Error(std::string code, std::string msg);
        // Создаёт StringResponse с заданными параметрами
        StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, std::string_view content_type);    
        // Создаёт FileResponse с заданными параметрами
        FileResponse MakeFileResponse(http::status status, http::file_body::value_type body, unsigned http_version, bool keep_alive, std::string_view content_type); 
        std::string_view GetContenType(fs::path&& path);
        bool IsSubPath(fs::path&& path, fs::path base);

        model::Game& game_;
        app::Players& players_;
        const fs::path game_file_path_;
        Strand api_strand_;

        const std::map<std::string, std::string_view> map_extension = {
            { ".json", ContentType::TEXT_JSON },
            { ".htm",  ContentType::TEXT_HTML },
            { ".html",  ContentType::TEXT_HTML },
            { ".css",  ContentType::TEXT_CSS },
            { ".txt",  ContentType::TEXT_TXT },
            { ".js",  ContentType::TEXT_JS },
            { ".xml",  ContentType::TEXT_XML },
            { ".png",  ContentType::TEXT_PNG },
            { ".jpg",  ContentType::TEXT_JPG },
            { ".jpe",  ContentType::TEXT_JPG },
            { ".jpeg",  ContentType::TEXT_JPG },
            { ".gif",  ContentType::TEXT_GIF },
            { ".bmp",  ContentType::TEXT_BMP },
            { ".ico",  ContentType::TEXT_ICO },
            { ".tiff",  ContentType::TEXT_TIF },
            { ".tif",  ContentType::TEXT_TIF },
            { ".svg",  ContentType::TEXT_SVG },
            { ".svgz",  ContentType::TEXT_SVG },
            { ".mp3",  ContentType::TEXT_MP3 }
        };
    };
}  // namespace http_handler

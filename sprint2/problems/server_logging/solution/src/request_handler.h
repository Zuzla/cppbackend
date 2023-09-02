#pragma once
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "http_server.h"
#include "model.h"
#include <boost/json.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>
#include <string>
#include <filesystem>
#include <map>

namespace http_handler 
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;
    namespace fs = std::filesystem;
    namespace sys = boost::system;

    using namespace std::literals;

    // Запрос, тело которого представлено в виде строки
    using StringRequest = http::request<http::string_body>;
    // Ответ, тело которого представлено в виде строки
    using StringResponse = http::response<http::string_body>;
    // Ответ, тело которого представлено в виде файла
    using FileResponse = http::response<http::file_body>;


    class RequestHandler 
    {
    public:
        explicit RequestHandler(model::Game& game, std::string game_file_path)
            : game_{game}, game_file_path_(game_file_path)
            {                }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        auto operator()(StringRequest&& req, auto&& send) 
        {
            HandleRequest(std::forward<decltype(req)>(req), [&send](auto&& response)
            {                
                send(std::move(response));
            });
        }

    private:

        void RequestApi(StringRequest&& req, std::string_view path, auto&& res)
        {
            if (!std::strstr(path.data(), "api/v1/maps"))
            {
                res(MakeStringResponse(http::status::bad_request, Error("Bad Request", "Bad request"), req.version(), req.keep_alive(), ContentType::TEXT_JSON));
                return;
            } 

            std::string body;
            std::string target_file = path.substr(path.find_last_of('/') + 1, path.size()).data();

            if (target_file == "maps" || target_file == "")
            {
                GetAllMaps(std::move(body));
            }
            else 
            {
                if (GetMap(target_file, std::move(body)) == false)
                {
                    res(MakeStringResponse(http::status::not_found, body, req.version(), req.keep_alive(), ContentType::TEXT_JSON));
                    return;
                }
            }

            res(MakeStringResponse(http::status::ok, body, req.version(), req.keep_alive(), ContentType::TEXT_JSON));

        }

        // если вынести в .cpp файл, выдается ошибка "used but never defined"
        void RequestFile(StringRequest&& req, fs::path path, auto&& res)
        {
            if (!path.has_extension())
            {
                res(MakeStringResponse(http::status::bad_request, Error("Bad Request", "Bad request"), req.version(), req.keep_alive(), ContentType::TEXT_HTML));
                return;
            }      

            if (!IsSubPath(std::move(path), game_file_path_))
            {
                res(MakeStringResponse(http::status::not_found, "File not found!", req.version(), req.keep_alive(), ContentType::TEXT_TXT));
                return;
            }

            http::file_body::value_type file;

            if (sys::error_code ec; file.open(path.c_str(), beast::file_mode::read, ec), ec) 
            {
                res(MakeStringResponse(http::status::not_found, "File not found!", req.version(), req.keep_alive(), ContentType::TEXT_TXT));
                return;
            }        

            res(MakeFileResponse(http::status::ok, std::move(file), req.version(), req.keep_alive(), GetContenType(std::move(path))));
        }   

        // обрабатывает запрос
        // если вынести в .cpp файл, выдается ошибка "used but never defined"
        void HandleRequest(StringRequest&& req, auto&& response)
        {
            // <summary>
            // !!! Переделать !!!
            // Узнать как правильно обрабатывать ссылки
            // </summary>
            
            auto url = req.target();
            url.remove_prefix(1);

            if (url.empty())
            {
                url = "index.html"sv;
            }

            std::string str = {url.begin(), url.end()};

            if (std::strstr(url.data(), "api"))
            {
                RequestApi(std::move(req), str,[response](auto&& res)
                {
                    response(std::move(res));
                });

                return;
            }
            else
            {
                RequestFile(std::move(req), str, [response](auto&& res)
                {
                    response(std::move(res));
                });

                return;
            }
            
            response(MakeStringResponse(http::status::bad_request, Error("Bad Request", "Bad request"), req.version(), req.keep_alive(), ContentType::TEXT_HTML));
        }

        // возвращает Json с ошибкой
        std::string Error(std::string code, std::string msg);
        // в res передает Json с данными о карте по id_map
        bool GetMap(const std::string& id_map, std::string&& res);
        // в res передает id и name для всех карт в виде Json
        void GetAllMaps(std::string&& res); 
        // Создаёт StringResponse с заданными параметрами
        StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, std::string_view content_type);    
        // Создаёт FileResponse с заданными параметрами
        FileResponse MakeFileResponse(http::status status, http::file_body::value_type body, unsigned http_version, bool keep_alive, std::string_view content_type); 
        std::string_view GetContenType(fs::path&& path);
        bool IsSubPath(fs::path&& path, fs::path base);

        model::Game& game_;
        const fs::path game_file_path_;

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

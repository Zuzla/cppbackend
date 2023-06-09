#pragma once
#include "http_server.h"
#include "model.h"
#include <boost/json.hpp>
#include <string>

namespace http_handler 
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;

    using namespace std::literals;

    class RequestHandler 
    {
    public:
        explicit RequestHandler(model::Game& game)
            : game_{game} {
        }

        RequestHandler(const RequestHandler&) = delete;
        RequestHandler& operator=(const RequestHandler&) = delete;

        // Запрос, тело которого представлено в виде строки
        using StringRequest = http::request<http::string_body>;
        // Ответ, тело которого представлено в виде строки
        using StringResponse = http::response<http::string_body>;

        struct ContentType {
            ContentType() = delete;
            constexpr static std::string_view TEXT_HTML = "application/json";
        };

        // возвращает Json с ошибкой
        std::string ErrorJson(std::string code, std::string msg);
        // в res передает Json с данными о карте по id_map
        bool GetMapJson(const std::string id_map, std::string* res);
        // в res передает id и name для всех карт в виде Json
        bool GetAllMapsJson(std::string* res); 
        // Создаёт StringResponse с заданными параметрами
        StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, std::string_view content_type = ContentType::TEXT_HTML);
        // обрабатывает запрос
        StringResponse HandleRequest(StringRequest&& req);

        template <typename Body, typename Allocator, typename Send>
        void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
            send(HandleRequest(std::forward<decltype(req)>(req)));
        }

    private:
        model::Game& game_;
    };
}  // namespace http_handler

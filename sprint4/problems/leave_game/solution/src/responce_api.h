#pragma once
#define BOOST_BEAST_USE_STD_STRING_VIEW

#include "loots.h"
#include "model.h"
#include "player.h"
#include "application.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

namespace http_handler
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace json = boost::json;

    class ResponseApi
    {

        using StringResponse = http::response<http::string_body>;
        using StringRequest = http::request<http::string_body>;

    public:
        ResponseApi(model::Game &game,
                    app::Players &players,
                    add_data::GameLoots &game_loots,
                    app::Application &application)
            : game_{game},
              players_{players},
              game_loots_{game_loots},
              application_{application} {

              };

        StringResponse Request(const StringRequest &req, std::string path);

    private:
        model::Game &game_;
        app::Players &players_;
        add_data::GameLoots &game_loots_;
        app::Application &application_;

        std::shared_ptr<app::Player> Authorization(const StringRequest &req,
                                                   StringResponse &&res);

        // возвращает Json с ошибкой
        std::string Error(std::string code, std::string msg);
        // Создаёт StringResponse с заданными параметрами
        StringResponse MakeStringResponse(http::status status, std::string_view body,
                                          unsigned http_version, bool keep_alive,
                                          const std::string_view &content_type,
                                          const std::string_view &cache_control = "",
                                          const std::string_view &allow = "");
        StringResponse Tick(const StringRequest &req);
        StringResponse JoinGame(const StringRequest &req);
        StringResponse Player(const StringRequest &req);
        StringResponse Records(const StringRequest &req);
        StringResponse State(const StringRequest &req);
        StringResponse PlayerAction(const StringRequest &req);
        boost::json::array RoadsObject(const model::Map &data_map);
        boost::json::array BuildingsObject(const model::Map &data_map);
        boost::json::array OfficesObject(const model::Map &data_map);
        boost::json::array LootsObject(const std::string &map_name);

        // в res передает Json с данными о карте по id_map
        bool GetMap(const std::string id_map, std::string &&res);
        bool CheckMap(const std::string &id_map);
        // в res передает id и name для всех карт в виде Json
        void GetAllMaps(std::string &&res) const;
    };
} // namespace http_handler
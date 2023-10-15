#include "responce_api.h"
#include "request_handler.h"
#include "player.h"
#include "constants.h"

namespace http_handler
{
    using namespace std::literals;
    using namespace constant;

    std::string ResponseApi::Error(std::string code, std::string msg)
    {
        code.erase(std::remove_if(code.begin(), code.end(), ::isspace), code.end());
        code[0] = tolower(code[0]);

        boost::json::object error =
            {
                {kCode, code},
                {kMessage, msg}};

        return json::serialize(error);
    }

    ResponseApi::StringResponse ResponseApi::MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, const std::string_view &content_type, const std::string_view &cache_control, const std::string_view &allow)
    {
        ResponseApi::StringResponse res(status, http_version);
        res.set(http::field::content_type, content_type);

        if (!cache_control.empty())
            res.set(http::field::cache_control, cache_control);

        if (!allow.empty())
            res.set(http::field::allow, allow);

        res.body() = body;
        res.content_length(body.size());
        res.keep_alive(keep_alive);
        return res;
    }

    boost::json::array ResponseApi::RoadsObject(const model::Map &data_map)
    {
        auto roads = data_map.GetRoads();
        boost::json::array roads_odj;
        roads_odj.reserve(roads.size());

        for (const auto &road : roads)
        {
            boost::json::object road_obj =
                {
                    {kPosition_X0, road.GetStart().x},
                    {kPosition_Y0, road.GetStart().y},
                };

            if (road.IsHorizontal())
                road_obj.emplace(kPosition_X1, road.GetEnd().x);
            else
                road_obj.emplace(kPosition_Y1, road.GetEnd().y);

            roads_odj.push_back(road_obj);
        };
        return roads_odj;
    }

    boost::json::array ResponseApi::BuildingsObject(const model::Map &data_map)
    {
        auto buildings = data_map.GetBuildings();
        boost::json::array building_odj;
        building_odj.reserve(buildings.size());

        for (const auto &building : buildings)
        {
            auto position = building.GetBounds().position;
            auto size = building.GetBounds().size;

            boost::json::object obj =
                {
                    {kXPosition, position.x},
                    {kYPosition, position.y},
                    {kWidth, size.width},
                    {kHeight, size.height}};

            building_odj.push_back(obj);
        };

        return building_odj;
    }

    boost::json::array ResponseApi::OfficesObject(const model::Map &data_map)
    {
        auto offices = data_map.GetOffices();
        boost::json::array offices_odj;
        offices_odj.reserve(offices.size());

        for (const auto &office : offices)
        {
            boost::json::object obj =
                {
                    {kId, *office.GetId()},
                    {kXPosition, office.GetPosition().x},
                    {kYPosition, office.GetPosition().y},
                    {kOffsetX, office.GetOffset().dx},
                    {kOffsetY, office.GetOffset().dy}};

            offices_odj.push_back(obj);
        };

        return offices_odj;
    }

    boost::json::array ResponseApi::LootsObject(const std::string &map_name)
    {
        auto loots = game_loots_.GetLoot(map_name);
        boost::json::array loots_odj;
        loots_odj.reserve(loots.size());

        for (const auto &loot : loots)
        {
            boost::json::object loot_obj;

            loot_obj.emplace(kName, loot.name_);
            loot_obj.emplace(kFile, loot.file_);
            loot_obj.emplace(kType, loot.type_);

            if (loot.rotation_ != -1)
                loot_obj.emplace(kRotation, loot.rotation_);

            if (!loot.color_.empty())
                loot_obj.emplace(kColor, loot.color_);

            loot_obj.emplace(kScale, loot.scale_);
            loot_obj.emplace(kValue, loot.value_);

            loots_odj.push_back(loot_obj);
        };
        return loots_odj;
    }

    bool ResponseApi::CheckMap(const std::string &id_map)
    {
        if (game_.FindMap(util::Tagged<std::string, model::Map>(id_map)) == nullptr)
            return false;

        return true;
    }

    bool ResponseApi::GetMap(const std::string id_map, std::string &&res)
    {
        const model::Map *data_map = nullptr;

        if (id_map == "maps" || id_map.empty())
        {
            GetAllMaps(std::move(res));
            return true;
        }

        data_map = game_.FindMap(util::Tagged<std::string, model::Map>(id_map));

        if (data_map == nullptr)
        {
            res = Error("Map Not Found", "Map not found");
            return false;
        }

        // future json
        json::object obj;

        obj["bagCapacity"] = data_map->GetBagCapacity();
        obj["id"] = *data_map->GetId();
        obj["name"] = data_map->GetName();

        // add roads
        obj["roads"] = RoadsObject(*data_map);

        // add buildings
        obj["buildings"] = BuildingsObject(*data_map);

        // add offices
        obj["offices"] = OfficesObject(*data_map);

        // add offices
        obj["lootTypes"] = LootsObject(data_map->GetName());

        res = json::serialize(obj);
        return true;
    };

    void ResponseApi::GetAllMaps(std::string &&res) const
    {
        auto all_maps = game_.GetMaps();

        boost::json::array maps_array;
        for (const auto &map : all_maps)
        {
            boost::json::object map_obj =
                {
                    {kId, *map.GetId()},
                    {kName, map.GetName()},
                };

            maps_array.push_back(map_obj);
        }

        res = json::serialize(maps_array);
    }

    std::shared_ptr<app::Player> ResponseApi::Authorization(const StringRequest &req, StringResponse &&res)
    {
        try
        {
            std::string token = std::string();

            for (const auto &h : req.base())
            {
                const boost::beast::string_view name = h.name_string();

                if (name == "Authorization" || name == "authorization")
                {
                    if (!h.value().starts_with("Bearer"))
                    {
                        res = MakeStringResponse(http::status::unauthorized, Error("Invalid Token", "Authorization header is required"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
                        return nullptr;
                    }

                    token = h.value().substr(h.value().find(' ') + 1, h.value().size());
                }
            }

            if (token.empty() || token.size() != 32)
            {
                res = MakeStringResponse(http::status::unauthorized, Error("Invalid Token", "Authorization header is missing"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
                return nullptr;
            }

            auto player = players_.FindByToken(token);

            if (player == nullptr)
            {
                res = MakeStringResponse(http::status::unauthorized, Error("Unknown Token", "Player token has not been found"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
                return nullptr;
            }

            return player;
        }
        catch (...)
        {
            res = MakeStringResponse(http::status::unauthorized, Error("Unknown Token", "Player token has not been found"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
            return nullptr;
        }
    }

    ResponseApi::StringResponse ResponseApi::Tick(const StringRequest &req)
    {
        if (req.method() != boost::beast::http::verb::post)
        {
            return MakeStringResponse(http::status::method_not_allowed, Error("Invalid Method", "Only POST method is expected"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv, "post"sv);
        }

        uint64_t time = 0;

        try
        {
            boost::json::value value = json::parse(req.body());
            time = json::value_to<uint64_t>(value.at("timeDelta"));
        }
        catch (...)
        {
            return (MakeStringResponse(http::status::bad_request, Error("Invalid Argument", "Invalid Argument"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));
        }

        if (time == 0)
            return (MakeStringResponse(http::status::bad_request, Error("Invalid Argument", "Invalid Argument"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));

        game_.Tick(std::chrono::milliseconds(time), game_loots_);

        json::object obj = {};

        return MakeStringResponse(http::status::ok, json::serialize(obj), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
    }

    ResponseApi::StringResponse ResponseApi::PlayerAction(const StringRequest &req)
    {
        if (req.method() != boost::beast::http::verb::post)
        {
            return MakeStringResponse(http::status::method_not_allowed, Error("Invalid Method", "Only POST method is expected"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv, "post"sv);
        }

        StringResponse res;

        auto player_ = Authorization(req, std::move(res));

        if (player_ == nullptr)
            return res;

        std::string direction = std::string();

        try
        {
            boost::json::value value = json::parse(req.body());
            direction = json::value_to<std::string>(value.at("move"));
        }
        catch (...)
        {
            return (MakeStringResponse(http::status::bad_request, Error("Invalid Argument", "Failed to parse action"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));
        }

        auto game_session_id_ = player_->GetGameSessionId();
        auto session_ = game_.FindGameSession(game_session_id_);
        auto dog_ = session_->FindDog(player_->GetDogId());

        if (dog_ == nullptr)
            return (MakeStringResponse(http::status::not_found, Error("Invalid Argument", "Not found dog in Game Session"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));

        dog_->SetDirection(direction);

        json::object obj = {};

        return MakeStringResponse(http::status::ok, json::serialize(obj), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
    }

    ResponseApi::StringResponse ResponseApi::State(const StringRequest &req)
    {
        if (req.method() != boost::beast::http::verb::get && req.method() != boost::beast::http::verb::head)
        {
            return MakeStringResponse(http::status::method_not_allowed, Error("Invalid Method", "Only GET and HEAD methods are expected"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv, "GET, HEAD"sv);
        }

        StringResponse res;

        auto player_ = Authorization(req, std::move(res));

        if (player_ == nullptr)
            return res;

        auto allPlayers = players_.GetPlayers();

        try
        {
            json::object players_array_;

            for (const auto &current_player : allPlayers)
            {
                auto game_session_id_ = current_player.second.get()->GetGameSessionId();
                auto dog_ = game_.FindGameSession(game_session_id_)->FindDog(current_player.second.get()->GetDogId());

                if (dog_ == nullptr)
                    return (MakeStringResponse(http::status::not_found, Error("Invalid Argument", "Invalid Argument"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));

                json::array obj_bag_;

                auto bag_ = dog_->GetBag();
                for (const auto& item : bag_)
                {
                    json::object obj_item = 
                    {
                        {kId, item.id_},
                        {kType, item.type_}
                    };
                    obj_bag_.push_back(obj_item);
                }

                json::array dog_position_;

                dog_position_.push_back(std::round(dog_->GetPosition().x * 10000.0) / 10000.0);
                dog_position_.push_back(std::round(dog_->GetPosition().y * 10000.0) / 10000.0);

                json::array dog_speed_;
                dog_speed_.push_back(dog_->GetSpeed().x);
                dog_speed_.push_back(dog_->GetSpeed().y);

                json::object obj =
                    {
                        {kPos, dog_position_},
                        {kSpeed, dog_speed_},
                        {kDir, dog_->GetDirection()},
                        {kBag, obj_bag_},
                        {kScore, dog_->GetScore()},           
                    };

                players_array_.emplace(std::to_string(current_player.second->GetId()), obj);
            }

            // TO DO! Сессия должна быть одна                         Возможна ошибка `.at(0)`! Аккуратно!
            auto game_session = game_.GetGameSessions().at(0);
            auto all_loots_ = game_session->GetMap().GetLoots();

            json::object loots_array_;
            loots_array_.reserve(all_loots_.size());

            for (const auto &loot : all_loots_)
            {
                json::array pos;
                pos.push_back(loot.position_x_);
                pos.push_back(loot.position_y_);

                json::object obj =
                    {
                        {kType, loot.type_},
                        {kPos, pos},
                    };

                loots_array_.emplace(std::to_string(loot.id_), obj);
            }

            json::object players_obj_ =
                {
                    {kPlayers, players_array_},
                    {kLostObjects, loots_array_},
                };

            return MakeStringResponse(http::status::ok, json::serialize(players_obj_), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
        }
        catch (...)
        {
            return MakeStringResponse(http::status::ok, Error("Exception", "Exception in state responce"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
        }
    }

    ResponseApi::StringResponse ResponseApi::JoinGame(const StringRequest &req)
    {
        if (req.method() != boost::beast::http::verb::post)
            return (MakeStringResponse(http::status::method_not_allowed, Error("Invalid Method", "Only POST method is expected"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv, "POST"sv));

        json::string user_name;
        std::string map_id;
        try
        {
            boost::json::value value = json::parse(req.body());
            user_name = json::value_to<std::string>(value.at("userName"));
            map_id = json::value_to<std::string>(value.at("mapId"));
        }
        catch (...)
        {
            return (MakeStringResponse(http::status::bad_request, Error("Invalid Argument", "Invalid Argument"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv, "POST"sv));
        }

        if (user_name.empty())
        {
            return (MakeStringResponse(http::status::bad_request, Error("Invalid Argument", "Invalid Argument"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv, "POST"sv));
        }

        if (!CheckMap(map_id))
        {
            return (MakeStringResponse(http::status::not_found, Error("Map Not Found", "Map not found"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv, "POST"sv));
        }

        try
        {
            auto session_ = game_.ConnectToSession(map_id, user_name.data());

            if (session_ == nullptr)
                return MakeStringResponse(http::status::bad_request, Error("Bad Request", "Bad request"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);

            std::string token_new_player_ = players_.AddPlayer(std::move(user_name.data()), session_->GetId());

            if (token_new_player_.empty())
                return MakeStringResponse(http::status::bad_request, Error("Bad Request", "Bad request"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);

            json::object obj;
            obj["authToken"] = token_new_player_;
            obj["playerId"] = players_.FindByToken(token_new_player_)->GetId();

            return MakeStringResponse(http::status::ok, json::serialize(obj), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
        }
        catch (...)
        {
            return MakeStringResponse(http::status::bad_request, Error("Bad Request", "Bad request"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
        }
    }

    ResponseApi::StringResponse ResponseApi::Player(const StringRequest &req)
    {
        if (req.method() != boost::beast::http::verb::get && req.method() != boost::beast::http::verb::head)
            return (MakeStringResponse(http::status::method_not_allowed, Error("Invalid Method", "Only POST method is expected"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv, "GET, HEAD"sv));

        StringResponse res;

        auto player_ = Authorization(req, std::move(res));

        if (player_ == nullptr)
            return res;

        auto game_session_id_ = player_->GetGameSessionId();
        auto all_dogs_ = game_.FindGameSession(game_session_id_)->GetDogs();

        json::array obj{all_dogs_.size()};

        for (const auto &dog : all_dogs_)
        {
            json::object name;
            name["name"] = *dog->GetId();

            obj.push_back(name);
        }

        return MakeStringResponse(http::status::ok, json::serialize(obj), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
    }

    ResponseApi::StringResponse ResponseApi::Request(const StringRequest &req, std::string path)
    {

        if (std::strstr(path.data(), "api/v1/maps"))
        {
            std::string body;
            std::string target_file = path.substr(path.find_last_of('/') + 1, path.size()).data();

            if (target_file == "maps" || target_file.empty())
            {
                GetAllMaps(std::move(body));
            }
            else
            {
                if (req.method() != boost::beast::http::verb::get && req.method() != boost::beast::http::verb::head)
                    return (MakeStringResponse(http::status::method_not_allowed, Error("Invalid Method", "Only POST method is expected"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv, "GET, HEAD"sv));

                if (GetMap(target_file, std::move(body)) == false)
                {
                    return (MakeStringResponse(http::status::not_found, body, req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv, "GET, HEAD"sv));
                }
            }

            return (MakeStringResponse(http::status::ok, body, req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv, "GET, HEAD"sv));
        }
        else if (std::strstr(path.data(), "api/v1/game"))
        {
            std::string target = path.substr(path.find_last_of('/') + 1, path.size()).data();

            if (target == "join")
            {
                return JoinGame(std::move(req));
            }

            if (target == "players")
            {
                return Player(std::move(req));
            }

            if (target == "state")
            {
                return State(std::move(req));
            }

            if (target == "action")
            {
                return PlayerAction(std::move(req));
            }

            if (target == "tick" && game_.IsDebug())
            {
                return Tick(std::move(req));
            }
        }

        return MakeStringResponse(http::status::bad_request, Error("Bad Request", "Bad request main"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
    }
}

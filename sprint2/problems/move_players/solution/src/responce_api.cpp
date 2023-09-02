#include "responce_api.h"
#include "request_handler.h"
#include "player.h"

namespace http_handler
{  
    using namespace std::literals;

    std::string ResponseApi::Error(std::string code, std::string msg)
    {
        code.erase(std::remove_if(code.begin(), code.end(), ::isspace), code.end());
        code[0] = tolower(code[0]);
        
        boost::json::object error = 
        {
            {"code", code},
            {"message", msg}
        };

        return json::serialize(error);
    }    

    ResponseApi::StringResponse ResponseApi::MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, const std::string_view& content_type, const std::string_view& cache_control, const std::string_view& allow) 
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

    boost::json::array ResponseApi::RoadsObject(const model::Map *data_map)
    {
        auto roads = data_map->GetRoads();
        boost::json::array roads_odj;

        for (int i = 0; i < roads.size(); i++)
        {
            boost::json::object road = 
            {
                {"x0", roads.at(i).GetStart().x},
                {"y0", roads.at(i).GetStart().y},
            };

            if (roads.at(i).IsHorizontal())
                road.emplace("x1", roads.at(i).GetEnd().x);
            else
                road.emplace("y1", roads.at(i).GetEnd().y);

            roads_odj.push_back(road);
        };
        return roads_odj;
    }

    boost::json::array ResponseApi::BuildingsObject(const model::Map *data_map)
    {
        auto buildings = data_map->GetBuildings();
        boost::json::array building_odj;

        for (const auto& building : buildings)
        {
            auto position = building.GetBounds().position;
            auto size = building.GetBounds().size;

            boost::json::object obj = 
            {
                {"x", position.x},
                {"y", position.y},
                {"w", size.width},
                {"h", size.height}
            };

            building_odj.push_back(obj);
        };

        return building_odj;
    }

    boost::json::array ResponseApi::OfficesObject(const model::Map *data_map)
    {
        auto offices = data_map->GetOffices();
        boost::json::array offices_odj;

        for (int i = 0; i < offices.size(); i++)
        {
            boost::json::object office = 
            {
                {"id", *offices.at(i).GetId()},
                {"x", offices.at(i).GetPosition().x},
                {"y", offices.at(i).GetPosition().y},
                {"offsetX", offices.at(i).GetOffset().dx},
                {"offsetY", offices.at(i).GetOffset().dy}
            };

            offices_odj.push_back(office);
        };

        return offices_odj; 
    }

    bool ResponseApi::CheckMap(const std::string& id_map)
    {
        if (game_.FindMap(util::Tagged<std::string, model::Map>(id_map)) == nullptr)
            return false;
        
        return true;
    }

    bool ResponseApi::GetMap(const std::string id_map, std::string&& res)
    {
        const model::Map *data_map = nullptr;

        if (id_map == "maps" || id_map == "")
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

        obj["id"] = *data_map->GetId();
        obj["name"] = data_map->GetName();

        // add roads        
        obj["roads"] = RoadsObject(data_map);
        
        // add buildings        
        obj["buildings"] = BuildingsObject(data_map);

        // add offices        
        obj["offices"] = OfficesObject(data_map);

        res = json::serialize(obj);
        return true;
    };

    void ResponseApi::GetAllMaps(std::string&& res) const
    {
        auto all_maps = game_.GetMaps();

        boost::json::array maps_array;
        for (const auto &map : all_maps)
        {
            boost::json::object map_obj = 
            {
                {"id", *map.GetId()},
                {"name", map.GetName()},
            };

            maps_array.push_back(map_obj);
        }
        
        res = json::serialize(maps_array);
    }

    const app::Player* ResponseApi::Authorization(const StringRequest& req, StringResponse&& res)
    {
        std::string token = "";

        for (const auto& h : req.base()) 
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

    ResponseApi::StringResponse ResponseApi::PlayerAction(const StringRequest& req)
    {
        if (req.method() != boost::beast::http::verb::post)
        {
            return MakeStringResponse(http::status::method_not_allowed, Error("Invalid Method", "Only POST method is expected"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv, "post"sv);
        }

        StringResponse res;

        auto player_ = Authorization(req, std::move(res));

        if (player_ == nullptr)
            return res;
        
        std::string direction = "";

        try
        {
            auto body = req.body();
            boost::json::value value = json::parse(body);
            auto arr = value.as_object();
            direction = json::value_to<std::string>(value.at("move"));
        }
        catch(...)
        {
            return (MakeStringResponse(http::status::bad_request, Error("Invalid Argument", "Failed to parse action"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));
        }

        if (direction.empty())
            return (MakeStringResponse(http::status::bad_request, Error("Invalid Argument", "Invalid Argument"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));

        try
        {
            
            auto dog_id_ = player_->GetDogId();
            auto dog_ = player_->GetGameSession().FindDog(dog_id_);

            if (dog_ == nullptr)
                return (MakeStringResponse(http::status::not_found, Error("Invalid Argument", "Not found dog in Game Session"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));

            dog_->SetDirection(direction);

            
        }catch(...)
        {
            return MakeStringResponse(http::status::bad_request, Error("Bad Request", "Bad request"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv); 
        }

        json::object obj;

        return MakeStringResponse(http::status::ok, json::serialize(obj), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
    }

    ResponseApi::StringResponse ResponseApi::State(const StringRequest& req)
    {
        if (req.method() != boost::beast::http::verb::get && req.method() != boost::beast::http::verb::head)
        {
            return MakeStringResponse(http::status::method_not_allowed, Error("Invalid Method", "Only GET and HEAD methods are expected"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
        }

        StringResponse res;

        auto player_ = Authorization(req, std::move(res));

        if (player_ == nullptr)
            return res;

        auto allPlayers = players_.GetPlayers();                              

        json::object players_array_{allPlayers.size()};

        for (const auto& player : allPlayers)
        {
            auto dog_id_ = player.second.get()->GetDogId();
            auto dog_ = player.second.get()->GetGameSession().FindDog(dog_id_);

            if (dog_ == nullptr)
                return (MakeStringResponse(http::status::not_found, Error("Invalid Argument", "Invalid Argument"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));

            json::array dog_position_;
            dog_position_.push_back(dog_->GetPosition().x);
            dog_position_.push_back(dog_->GetPosition().y);

            json::array dog_speed_;
            dog_speed_.push_back(dog_->GetSpeed().x);
            dog_speed_.push_back(dog_->GetSpeed().y);

            json::object obj = 
            {
                {"pos", dog_position_},
                {"speed", dog_speed_},
                {"dir", dog_->GetDirection()}
                
            };

            players_array_.emplace(std::to_string(player.second.get()->GetId()), obj);
        }
        
        json::object players_obj_ = 
        {
            {"players", players_array_}
        }; 

        return MakeStringResponse(http::status::ok, json::serialize(players_obj_), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
    }

    ResponseApi::StringResponse ResponseApi::JoinGame(const StringRequest& req)
    {
        if (req.method() != boost::beast::http::verb::post)
            return (MakeStringResponse(http::status::method_not_allowed, Error("Invalid Method", "Only POST method is expected"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));

        // TO DO. Remake!!! Exception here broke program
        std::string user_name;
        std::string map_id;
        try
        {
            boost::json::value value = json::parse(req.body());
            user_name = json::value_to<std::string>(value.at("userName"));
            map_id = json::value_to<std::string>(value.at("mapId"));
        }
        catch(...)
        {
            return (MakeStringResponse(http::status::not_found, Error("Invalid Argument", "Invalid Argument"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));
        }

        if (user_name.empty())
        {
            return (MakeStringResponse(http::status::not_found, Error("Invalid Argument", "Invalid Argument"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));
        }

        if (!CheckMap(map_id))
        {
            return (MakeStringResponse(http::status::not_found, Error("Map Not Found", "Map not found"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv));
        }

        std::string token_new_player_;

        try
        {        
            auto session_ = game_.ConnectToSession(map_id, user_name);
            token_new_player_ = players_.AddPlayer(std::move(user_name), std::make_shared<model::GameSession>(session_));
        } 
        catch(...)
        {
            return MakeStringResponse(http::status::bad_request, Error("Bad Request", "AddPlayer Error"), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
        }

        json::object obj;
        obj["authToken"] = token_new_player_;
        obj["playerId"] = players_.FindByToken(token_new_player_)->GetId();

        return MakeStringResponse(http::status::ok, json::serialize(obj), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
    }
    
    ResponseApi::StringResponse ResponseApi::Player(const StringRequest& req)
    {
        StringResponse res;

        auto player_ = Authorization(req, std::move(res));

        if (player_ == nullptr)
            return res;    
        
        json::object name;
        name["name"] = *player_->GetDogId();       
             
        boost::json::object obj = 
        {
            {std::to_string(player_->GetId()), name}
        };        
        
        return MakeStringResponse(http::status::ok, json::serialize(obj), req.version(), req.keep_alive(), ContentType::TEXT_JSON, "no-cache"sv);
    }

    ResponseApi::StringResponse ResponseApi::Request(const StringRequest& req, std::string path)
    {

        if (std::strstr(path.data(), "api/v1/maps"))
        {    
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
                    return (MakeStringResponse(http::status::not_found, body, req.version(), req.keep_alive(), ContentType::TEXT_JSON));
                }
            }

            return (MakeStringResponse(http::status::ok, body, req.version(), req.keep_alive(), ContentType::TEXT_JSON));
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
        }
        
        return MakeStringResponse(http::status::bad_request, Error("Bad Request", "Bad request main"), req.version(), req.keep_alive(), ContentType::TEXT_JSON);
    }
}

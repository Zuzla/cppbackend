#include "request_handler.h"

namespace http_handler 
{
    std::string RequestHandler::Error(std::string code, const std::string& msg)
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

    boost::json::array RequestHandler::RoadsObject(const model::Map& data_map)
    {
        auto roads = data_map.GetRoads();
        boost::json::array roads_odj{roads.size()};
        //roads_odj.reserve(roads.size());

        for (const auto& road : roads)
        {
            boost::json::object road_obj = 
            {
                {"x0", road.GetStart().x},
                {"y0", road.GetStart().y},
            };

            if (road.IsHorizontal())
                road_obj.emplace("x1", road.GetEnd().x);
            else
                road_obj.emplace("y1", road.GetEnd().y);

            roads_odj.push_back(road_obj);
        };
        return roads_odj;
    }

    boost::json::array RequestHandler::BuildingsObject(const model::Map& data_map)
    {
        auto buildings = data_map.GetBuildings();
        boost::json::array building_odj{buildings.size()};
        //building_odj.reserve(buildings.size());

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

    boost::json::array RequestHandler::OfficesObject(const model::Map& data_map)
    {
        auto offices = data_map.GetOffices();
        boost::json::array offices_odj{offices.size()};
        //offices_odj.reserve(offices.size());

        for (const auto& office : offices)
        {
            boost::json::object obj = 
            {
                {"id", *office.GetId()},
                {"x", office.GetPosition().x},
                {"y", office.GetPosition().y},
                {"offsetX", office.GetOffset().dx},
                {"offsetY", office.GetOffset().dy}
            };

            offices_odj.push_back(obj);
        };

        return offices_odj; 
    }

    bool RequestHandler::GetMap(const std::string id_map, std::string&& res)
    {
        const model::Map *data_map = nullptr;

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
        obj["roads"] = RoadsObject(*data_map);
        
        // add buildings        
        obj["buildings"] = BuildingsObject(*data_map);

        // add offices        
        obj["offices"] = OfficesObject(*data_map);

        res = json::serialize(obj);
        return true;
    };

    void RequestHandler::GetAllMaps(std::string&& res) const
    {
        auto all_maps = game_.GetMaps();

        boost::json::array maps_array;
        maps_array.reserve(all_maps.size());
        
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

    RequestHandler::StringResponse RequestHandler::MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, 
                                                                        std::string_view content_type) 
    {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);        
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    RequestHandler::StringResponse RequestHandler::HandleRequest(StringRequest&& req) 
    {
        // <summary>
        // !!! Переделать !!!
        // Узнать как правильно обрабатывать ссылки
        // </summary>

        std::string body;

        std::string url = req.target().data();

        if (std::strstr(url.c_str(), "/api/v1/maps"))
        {
            std::string target_file = url.substr(url.find_last_of('/') + 1, url.size()).data();

            if (target_file == "maps" || target_file.empty())
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

        body = Error("Bad Request", "Bad request");
        return MakeStringResponse(http::status::bad_request, body, req.version(), req.keep_alive());
    }

}  // namespace http_handler

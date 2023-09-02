#include "request_handler.h"

namespace http_handler 
{
    using namespace std::literals;

    std::string RequestHandler::Error(std::string code, std::string msg)
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


    boost::json::array RoadsObject(const model::Map *data_map)
    {
        auto roads = data_map->GetRoads();
        boost::json::array roads_odj;
        for (const auto& road : roads)
        {
            boost::json::object obj = 
            {
                {"x0", road.GetStart().x},
                {"y0", road.GetStart().y},
            };

            if (road.IsHorizontal())
                obj.emplace("x1", road.GetEnd().x);
            else
                obj.emplace("y1", road.GetEnd().y);

            roads_odj.push_back(obj);
        };
        return roads_odj;
    }

    boost::json::array BuildingsObject(const model::Map *data_map)
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

    boost::json::array OfficesObject(const model::Map *data_map)
    {
        auto offices = data_map->GetOffices();
        boost::json::array offices_odj;
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

    bool RequestHandler::GetMap(const std::string& id_map, std::string&& res)
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

    void RequestHandler::GetAllMaps(std::string&& res)
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

    StringResponse RequestHandler::MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, std::string_view content_type) 
    {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);        
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    FileResponse RequestHandler::MakeFileResponse(http::status status, http::file_body::value_type body, unsigned http_version, bool keep_alive, std::string_view content_type) 
    {
        FileResponse response;
        response.version(http_version);
        response.result(status);
        response.insert(http::field::content_type, content_type);        
        response.body() = std::move(body);
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        response.prepare_payload();
        return response;
    }

    std::string_view RequestHandler::GetContenType(fs::path&& path)
    {
        try
        {
            return map_extension.at(path.extension().string());
        }
        catch(const std::exception& e)
        {
            return ContentType::TEXT_HTML;
        }   
    }    

    bool RequestHandler::IsSubPath(fs::path&& path, fs::path base) 
    {
        
        // Приводим оба пути к каноничному виду (без . и ..)
        path = fs::weakly_canonical(game_file_path_ / path);
        base = fs::weakly_canonical(base);

        // Проверяем, что все компоненты base содержатся внутри path
        for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
            if (p == path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }

}  // namespace http_handler

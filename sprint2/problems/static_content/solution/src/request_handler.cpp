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

    boost::json::array BuildingsObject(const model::Map *data_map)
    {
        auto buildings = data_map->GetBuildings();
        boost::json::array building_odj;
        for (int i = 0; i < buildings.size(); i++)
        {
            auto position = buildings.at(i).GetBounds().position;
            auto size = buildings.at(i).GetBounds().size;

            boost::json::object building = 
            {
                {"x", position.x},
                {"y", position.y},
                {"w", size.width},
                {"h", size.height}
            };

            building_odj.push_back(building);
        };

        return building_odj;
    }

    boost::json::array OfficesObject(const model::Map *data_map)
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

    RequestHandler::StringResponse RequestHandler::MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, std::string_view content_type) 
    {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);        
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    RequestHandler::FileResponse RequestHandler::MakeFileResponse(http::status status, http::file_body::value_type body, unsigned http_version, bool keep_alive, std::string_view content_type) 
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

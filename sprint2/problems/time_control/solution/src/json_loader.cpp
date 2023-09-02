#include "json_loader.h"

#include <fstream>

namespace json_loader {


model::Game LoadGame(const std::filesystem::path& json_path) 
{
    model::Game game;
    std::stringstream data_file;

    std::ifstream file;
    file.open(json_path.c_str()); 

    if (!file.is_open())
    {
        throw std::invalid_argument("Could not open file.");
    }

    data_file << file.rdbuf();
    file.close();

    auto value = json::parse(data_file.str());

    int default_dog_speed_ = json::value_to<int>(value.at("defaultDogSpeed"));

    auto maps = value.at("maps");

    for (const auto& map : maps.as_array())
    {
        int map_dog_speed_ = 0;

        if (map.as_object().if_contains("dogSpeed"))
        {
            map_dog_speed_ = json::value_to<int>(map.at("dogSpeed"));
        }
        else
        {
            map_dog_speed_ = default_dog_speed_;
        }
        

        std::string id = json::value_to<std::string>(map.at("id"));
        std::string name = json::value_to<std::string>(map.at("name"));

        model::Map new_map(std::move(util::Tagged<std::string, model::Map>(id)), std::move(name));

        // Get all roads
        auto roads = json::value_to<boost::json::value>(map.at("roads")); 

        for (const auto& road : roads.as_array())
        {
            int x0 = json::value_to<int>(road.at("x0"));
            int y0 = json::value_to<int>(road.at("y0"));
            

            try
            {
                int x1 = json::value_to<int>(road.at("x1"));
                model::Road new_road(model::Road::HORIZONTAL, model::Point(x0,y0), x1);
                new_map.AddRoad(new_road);
            }catch(std::exception& e)
            {
                int y1 = json::value_to<int>(road.at("y1"));
                model::Road new_road(model::Road::VERTICAL, model::Point(x0,y0), y1);
                new_map.AddRoad(new_road);
            }

        }

        // Get all buildings
        auto buildings = json::value_to<boost::json::value>(map.at("buildings")); 

        for (const auto& building : buildings.as_array())
        {
            int x = json::value_to<int>(building.at("x"));
            int y = json::value_to<int>(building.at("y"));
            int w = json::value_to<int>(building.at("w"));
            int h = json::value_to<int>(building.at("h"));

            model::Building new_buildings(model::Rectangle(model::Point(x,y), model::Size(w,h)));
            new_map.AddBuilding(new_buildings);
        }

        // Get all offices
        auto offices = json::value_to<boost::json::value>(map.at("offices")); 

        for (const auto& office : offices.as_array())
        {
            std::string id = json::value_to<std::string>(office.at("id"));
            int x = json::value_to<int>(office.at("x"));
            int y = json::value_to<int>(office.at("y"));
            int offsetX = json::value_to<int>(office.at("offsetX"));
            int offsetY = json::value_to<int>(office.at("offsetY"));

            model::Office new_office(std::move(util::Tagged<std::string, model::Office>(id)), model::Point(x,y), model::Offset(offsetX, offsetY));
            new_map.AddOffice(std::move(new_office));
        }

        new_map.SetDogSpeed(map_dog_speed_);            

        game.AddMap(std::move(new_map));
    }

    return game;
}

}  // namespace json_loader
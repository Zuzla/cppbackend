#include "json_loader.h"

#include <fstream>

namespace json_loader {


model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;
    std::stringstream data_file;

    std::ifstream file;
    file.open(json_path.c_str()); 

    if (!file.is_open())
    {
        std::cout << "Could not open file." << std::endl;
        return game;
    }

    data_file << file.rdbuf();
    file.close();

    auto value = json::parse(data_file.str());

    auto maps = value.at("maps");

    for (int i = 0; i < maps.as_array().size(); i++)
    {
        std::string id = json::value_to<std::string>(maps.at(i).at("id"));
        std::string name = json::value_to<std::string>(maps.at(i).at("name"));

        model::Map new_map(std::move(util::Tagged<std::string, model::Map>(id)), std::move(name));

        // Get all roads
        auto roads = json::value_to<boost::json::value>(maps.at(i).at("roads")); 

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
        auto buildings = json::value_to<boost::json::value>(maps.at(i).at("buildings")); 

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
        auto offices = json::value_to<boost::json::value>(maps.at(i).at("offices")); 

        for (const auto& office : offices.as_array())
        {
            std::string id = json::value_to<std::string>(office.at("id"));
            int x = json::value_to<int>(office.at("x"));
            int y = json::value_to<int>(office.at("y"));
            int offsetX = json::value_to<int>(office.at("offsetX"));
            int offsetY = json::value_to<int>(office.at("offsetY"));

            model::Office new_office(std::move(util::Tagged<std::string, model::Office>(id)), model::Point(x,y), model::Offset(offsetX, offsetY));
            new_map.AddOffice(new_office);
        }

        game.AddMap(new_map);
    }

    return game;
}

}  // namespace json_loader

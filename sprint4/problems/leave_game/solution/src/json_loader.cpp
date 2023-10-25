#include "json_loader.h"
#include "loots.h"
#include "constants.h"

#include <fstream>

namespace json_loader
{

    using namespace constant;

    void LoadBuildings(const boost::json::value &maps, model::Map &&new_map)
    {
        auto buildings = json::value_to<boost::json::value>(maps);

        for (const auto &building : buildings.as_array())
        {
            int x = json::value_to<int>(building.at(kXPosition));
            int y = json::value_to<int>(building.at(kYPosition));
            int w = json::value_to<int>(building.at(kWidth));
            int h = json::value_to<int>(building.at(kHeight));

            model::Building new_buildings(model::Rectangle(model::Point(x, y), model::Size(w, h)));
            new_map.AddBuilding(new_buildings);
        }
    }

    void LoadOffice(const boost::json::value &maps, model::Map &&new_map)
    {
        auto offices = json::value_to<boost::json::value>(maps);

        for (const auto &office : offices.as_array())
        {
            std::string id = json::value_to<std::string>(office.at(kId));
            int x = json::value_to<int>(office.at(kXPosition));
            int y = json::value_to<int>(office.at(kYPosition));
            int offsetX = json::value_to<int>(office.at(kOffsetX));
            int offsetY = json::value_to<int>(office.at(kOffsetY));

            model::Office new_office(std::move(util::Tagged<std::string, model::Office>(id)), model::Point(x, y), model::Offset(offsetX, offsetY));
            new_map.AddOffice(std::move(new_office));
        }
    }

    void LoadRoads(const boost::json::value &maps, model::Map &&new_map)
    {
        auto roads = json::value_to<boost::json::value>(maps);

        for (const auto &road : roads.as_array())
        {
            int x0 = json::value_to<int>(road.at(kPosition_X0));
            int y0 = json::value_to<int>(road.at(kPosition_Y0));

            try
            {
                int x1 = json::value_to<int>(road.at(kPosition_X1));
                model::Road new_road(model::Road::HORIZONTAL, model::Point(x0, y0), x1);
                new_map.AddRoad(new_road);
            }
            catch (std::exception &e)
            {
                int y1 = json::value_to<int>(road.at(kPosition_Y1));
                model::Road new_road(model::Road::VERTICAL, model::Point(x0, y0), y1);
                new_map.AddRoad(new_road);
            }
        }
    }

    void LoadLoots(const boost::json::value &maps, add_data::GameLoots &&game_loots_, std::string map_name_)
    {

        auto loots = json::value_to<boost::json::value>(maps);

        for (const auto &loot : loots.as_array())
        {
            std::string name = json::value_to<std::string>(loot.at(kName));
            std::string file = json::value_to<std::string>(loot.at(kFile));
            std::string type = json::value_to<std::string>(loot.at(kType));

            int32_t rotation;
            try
            {
                rotation = json::value_to<int32_t>(loot.at(kRotation));
            }
            catch (std::exception& ex)
            {
                rotation = -1;
            }

            std::string color;
            try
            {
                color = json::value_to<std::string>(loot.at(kColor));
            }
            catch (std::exception& ex)
            {
                color = "";
            }

            double scale = json::value_to<double>(loot.at(kScale));

            int32_t value = json::value_to<int32_t>(loot.at(kValue));

            add_data::Loot loot_{name, file, type, color, rotation, scale, value};
            game_loots_.AddLoot(map_name_, std::move(loot_));
        }
    }

    void LoadGameData(model::Game &&game, add_data::GameLoots &&game_loots, const std::filesystem::path &json_path)
    {
        std::stringstream data_file;

        std::cout << std::filesystem::absolute(json_path) << std::endl;
        
        std::ifstream file;
        file.open(json_path.c_str());

        if (!file.is_open())
        {
            throw std::invalid_argument("Could not open file.");
        }

        data_file << file.rdbuf();
        file.close();

        auto value = json::parse(data_file.str());

        int default_dog_speed_ = json::value_to<int>(value.at(kDefaultDogSpeed));
        int default_bag_capacity_ = json::value_to<int>(value.at(kDefaultBagCapacity));

        auto loot_generator_config = value.at(kLootGeneratorConfig);
        double period = json::value_to<double>(loot_generator_config.at(kPeriod));
        double probability = json::value_to<double>(loot_generator_config.at(kProbability));
        loot_gen::LootGenerator loot_generator_{std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(period)), probability};
        
        double retirement_time = json::value_to<double>(value.at(kDogRetirementTime));

        auto maps = value.at("maps");

        for (const auto &map : maps.as_array())
        {
            int map_dog_speed_ = 0;

            if (map.as_object().if_contains(kDogSpeed))
            {
                map_dog_speed_ = json::value_to<int>(map.at(kDogSpeed));
            }
            else
            {
                map_dog_speed_ = default_dog_speed_;
            }

            int map_bag_capacity_ = 0;

            if (map.as_object().if_contains(kBagCapacity))
            {
                map_bag_capacity_ = json::value_to<int>(map.at(kBagCapacity));
            }
            else
            {
                map_bag_capacity_ = default_bag_capacity_;
            }

            std::string id = json::value_to<std::string>(map.at(kId));
            std::string name = json::value_to<std::string>(map.at(kName));

            model::Map new_map(std::move(util::Tagged<std::string, model::Map>(id)), name);

            // Get all loot types
            LoadLoots(map.at("lootTypes"), std::move(game_loots), name);
            game_loots.MakeGenerator(loot_generator_);
            
            // Get all roads
            LoadRoads(map.at("roads"), std::move(new_map));

            // Get all buildings
            LoadBuildings(map.at("buildings"), std::move(new_map));

            // Get all offices
            LoadOffice(map.at("offices"), std::move(new_map));

            new_map.SetDogSpeed(map_dog_speed_);
            new_map.SetBagCapacity(map_bag_capacity_);
            new_map.SetRetirementTime(retirement_time);

            game.AddMap(std::move(new_map));
        }
    }

} // namespace json_loader
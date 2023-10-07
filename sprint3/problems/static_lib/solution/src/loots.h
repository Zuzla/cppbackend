#pragma once

#include <iostream>
#include <map>
#include <vector>

#include <boost/asio.hpp>

#include "loot_generator.h"

using namespace std::literals;

namespace add_data
{
    struct Loot
    {
    public:
        std::string name_;
        std::string file_;
        std::string type_;
        std::string color_;
        int32_t rotation_;
        double scale_;

        Loot(const std::string &name, const std::string &file, const std::string &type, const std::string &color,
             int32_t rotation, double scale)
            : name_(name), file_(file), type_(type), color_(color),
              rotation_(rotation), scale_(scale) {}
    };

    class GameLoots
    {
    public:
        GameLoots() = default;

        std::vector<Loot> GetLoot(const std::string &map_name);
        void AddLoot(const std::string &map_name, Loot&& new_loot)
        {
            auto it = loot_.find(map_name);
            if (it == loot_.end())
            {
                std::vector<Loot> vec;
                vec.push_back(new_loot);
                auto pair = std::pair<std::string, std::vector<Loot>>(map_name, vec);
                loot_.insert(pair);
            }
            else
            {
                it->second.push_back(new_loot);
            }
        }

        void MakeGenerator(loot_gen::LootGenerator& gen)
        {
            generator = std::make_shared<loot_gen::LootGenerator>(gen);
        }

        std::shared_ptr<loot_gen::LootGenerator>& GetGenerator()
        {
            return generator;
        }

    private:
        std::map<std::string, std::vector<Loot>> loot_;
        std::shared_ptr<loot_gen::LootGenerator> generator = nullptr;
    };

};
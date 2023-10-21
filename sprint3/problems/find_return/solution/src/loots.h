#pragma once

#include <iostream>
#include <map>
#include <vector>

#include <boost/asio.hpp>

#include "loot_generator.h"

namespace add_data
{
    using namespace std::literals;
    struct Loot
    {
    public:
        std::string name_;
        std::string file_;
        std::string type_;
        std::string color_;
        int32_t rotation_;
        double scale_;
        int32_t value_;

        Loot(const std::string &name, const std::string &file, const std::string &type, const std::string &color,
             int32_t rotation, double scale, int32_t value)
            : name_(name), file_(file), type_(type), color_(color),
              rotation_(rotation), scale_(scale), value_(value) {}
    };

    class GameLoots
    {
    public:
        GameLoots() = default;

        std::vector<Loot> GetLoot(const std::string &map_name);
        void AddLoot(const std::string &map_name, Loot &&new_loot);

        void MakeGenerator(loot_gen::LootGenerator &gen)
        {
            if (generator == nullptr)
                generator = std::make_shared<loot_gen::LootGenerator>(gen);
        }

        std::shared_ptr<loot_gen::LootGenerator> &GetGenerator() noexcept
        {
            return generator;
        }

    private:
        std::map<std::string, std::vector<Loot>> loot_;
        std::shared_ptr<loot_gen::LootGenerator> generator = nullptr;
    };

};
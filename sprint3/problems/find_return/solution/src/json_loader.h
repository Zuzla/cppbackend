#pragma once

#include <filesystem>
#include <iostream>
#include <boost/json.hpp>

#include "model.h"
#include "loot_generator.h"
#include "loots.h"

namespace json = boost::json;

namespace json_loader
{
    void LoadGameData(model::Game &&game, add_data::GameLoots &&game_loots, const std::filesystem::path &json_path);

} // namespace json_loader

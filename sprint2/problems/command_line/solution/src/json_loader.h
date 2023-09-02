#pragma once

#include <filesystem>
#include <iostream>
#include <boost/json.hpp>

#include "model.h"

namespace json = boost::json;

namespace json_loader {

void LoadGameData(model::Game&& game, const std::filesystem::path& json_path);

}  // namespace json_loader

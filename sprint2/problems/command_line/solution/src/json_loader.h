#pragma once

#include <filesystem>
#include <iostream>
#include <boost/json.hpp>

#include "model.h"

namespace json = boost::json;

namespace json_loader
{

    const std::string kId = "id";
    const std::string kName = "name";

    const std::string kXPosition = "x";
    const std::string kYPosition = "y";
    const std::string kWidth = "w";
    const std::string kHeight = "h";

    const std::string kPosition_X0 = "x0";
    const std::string kPosition_X1 = "x1";
    const std::string kPosition_Y0 = "y0";
    const std::string kPosition_Y1 = "y1";

    const std::string kDogSpeed = "dogSpeed";

    const std::string kOffsetX = "offsetX";
    const std::string kOffsetY = "offsetY";

    void LoadGameData(model::Game &&game, const std::filesystem::path &json_path);

} // namespace json_loader

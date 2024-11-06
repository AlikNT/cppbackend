#pragma once

#include <filesystem>

#include "model.h"

// Константы для ключей JSON
const std::string X = "x";
const std::string Y = "y";
const std::string BUILDING_WIDTH = "w";
const std::string BUILDING_HEIGHT = "h";
const std::string OFFICE_OFFSET_X = "offsetX";
const std::string OFFICE_OFFSET_Y = "offsetY";
const std::string OFFICE_ID = "id";
const std::string ROAD_BEGIN_X0 = "x0";
const std::string ROAD_BEGIN_Y0 = "y0";
const std::string ROAD_END_X1 = "x1";
const std::string ROAD_END_Y1 = "y1";
const std::string LOOT_TYPES = "lootTypes";

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader

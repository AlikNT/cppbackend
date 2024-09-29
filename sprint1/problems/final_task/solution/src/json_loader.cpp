#include "json_loader.h"

#include <boost/json.hpp>
#include <fstream>
#include <iostream>

namespace json = boost::json;

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    // Чтение содержимого файла в строку
    std::ifstream file(json_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file");
    }

    std::string json_str((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    // Парсинг строки как JSON
    json::value parsed_json = json::parse(json_str);
    json::object root = parsed_json.as_object();

    // Создаем объект Game
    model::Game game;

    // Извлекаем карты из JSON
    if (root.contains("maps")) {
        for (const auto& map_json : root["maps"].as_array()) {
            const json::object& map_obj = map_json.as_object();

            // Получаем id и name карты
            auto map_id = model::Map::Id(map_obj.at("id").as_string().c_str());
            std::string map_name = map_obj.at("name").as_string().c_str();

            // Создаем объект карты
            model::Map map(map_id, map_name);

            // Извлекаем дороги
            if (map_obj.contains("roads")) {
                for (const auto& road_json : map_obj.at("roads").as_array()) {
                    const json::object& road_obj = road_json.as_object();

                    model::Point start{static_cast<int>(road_obj.at("x0").as_int64()), static_cast<int>(road_obj.at("y0").as_int64())};

                    if (road_obj.contains("x1")) {  // Горизонтальная дорога
                        model::Coord end_x = static_cast<int>(road_obj.at("x1").as_int64());
                        map.AddRoad(model::Road(model::Road::HORIZONTAL, start, end_x));
                    } else if (road_obj.contains("y1")) {  // Вертикальная дорога
                        model::Coord end_y = static_cast<int>(road_obj.at("y1").as_int64());
                        map.AddRoad(model::Road(model::Road::VERTICAL, start, end_y));
                    }
                }
            }

            // Извлекаем здания
            if (map_obj.contains("buildings")) {
                for (const auto& building_json : map_obj.at("buildings").as_array()) {
                    const json::object& building_obj = building_json.as_object();
                    model::Rectangle rect{
                            model::Point{static_cast<int>(building_obj.at("x").as_int64()), static_cast<int>(building_obj.at("y").as_int64())},
                            model::Size{static_cast<int>(building_obj.at("w").as_int64()), static_cast<int>(building_obj.at("h").as_int64())}
                    };
                    map.AddBuilding(model::Building(rect));
                }
            }

            // Извлекаем офисы
            if (map_obj.contains("offices")) {
                for (const auto& office_json : map_obj.at("offices").as_array()) {
                    const json::object& office_obj = office_json.as_object();

                    auto office_id = model::Office::Id(office_obj.at("id").as_string().c_str());
                    model::Point position{static_cast<int>(office_obj.at("x").as_int64()), static_cast<int>(office_obj.at("y").as_int64())};
                    model::Offset offset{static_cast<int>(office_obj.at("offsetX").as_int64()), static_cast<int>(office_obj.at("offsetY").as_int64())};

                    map.AddOffice(model::Office(office_id, position, offset));
                }
            }

            // Добавляем карту в игру
            game.AddMap(std::move(map));
        }
    }
    return game;
}

}  // namespace json_loader

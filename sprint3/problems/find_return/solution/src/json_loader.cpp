#include <boost/json.hpp>
#include <fstream>
#include <iostream>

#include "json_loader.h"
#include "extra_data.h"
#include "loot_generator.h"

namespace json = boost::json;

namespace json_loader {
using namespace std::literals;

// Функция для парсинга дорог и возвращения карты с дорогами
model::Map ParseRoads(const json::object& map_obj, model::Map map) {
    if (map_obj.contains("roads"s)) {
        for (const auto& road_json : map_obj.at("roads"s).as_array()) {
            const json::object& road_obj = road_json.as_object();

            model::Point start{static_cast<int>(road_obj.at(ROAD_BEGIN_X0).as_int64()), static_cast<int>(road_obj.at(ROAD_BEGIN_Y0).as_int64())};

            if (road_obj.contains(ROAD_END_X1)) {  // Горизонтальная дорога
                const auto end_x = static_cast<int>(road_obj.at(ROAD_END_X1).as_int64());
                map.AddRoad(model::Road(model::Road::HORIZONTAL, start, end_x));
            } else if (road_obj.contains(ROAD_END_Y1)) {  // Вертикальная дорога
                const auto end_y = static_cast<int>(road_obj.at(ROAD_END_Y1).as_int64());
                map.AddRoad(model::Road(model::Road::VERTICAL, start, end_y));
            }
        }
    }
    return map;
}

// Функция для парсинга зданий и возвращения карты со зданиями
model::Map ParseBuildings(const json::object& map_obj, model::Map map) {
    if (map_obj.contains("buildings"s)) {
        for (const auto& building_json : map_obj.at("buildings").as_array()) {
            const json::object& building_obj = building_json.as_object();
            model::Rectangle rect{
                    model::Point{static_cast<int>(building_obj.at(X).as_int64()), static_cast<int>(building_obj.at(Y).as_int64())},
                    model::Size{static_cast<int>(building_obj.at(BUILDING_WIDTH).as_int64()), static_cast<int>(building_obj.at(BUILDING_HEIGHT).as_int64())}
            };
            map.AddBuilding(model::Building(rect));
        }
    }
    return map;
}

// Функция для парсинга офисов и возвращения карты с офисами
model::Map ParseOffices(const json::object& map_obj, model::Map map) {
    if (map_obj.contains("offices"s)) {
        for (const auto& office_json : map_obj.at("offices").as_array()) {
            const json::object& office_obj = office_json.as_object();

            auto office_id = model::Office::Id(office_obj.at(OFFICE_ID).as_string().c_str());
            model::Point position{static_cast<int>(office_obj.at(X).as_int64()), static_cast<int>(office_obj.at(Y).as_int64())};
            model::Offset offset{static_cast<int>(office_obj.at(OFFICE_OFFSET_X).as_int64()), static_cast<int>(office_obj.at(OFFICE_OFFSET_Y).as_int64())};

            map.AddOffice(model::Office(office_id, position, offset));
        }
    }
    return map;
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file"s);
    }

    std::string json_str((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    json::value parsed_json = json::parse(json_str);
    json::object root = parsed_json.as_object();

    model::Game game;
    constexpr double DEFAULT_DOG_SPEED = 1.0;
    double dog_speed = DEFAULT_DOG_SPEED;  // Значение по умолчанию
    if (root.contains("defaultDogSpeed"s)) {
        dog_speed = root["defaultDogSpeed"s].as_double();
    }

    constexpr size_t DEFAULT_BAG_CAPACITY = 3;
    unsigned bag_capacity = DEFAULT_BAG_CAPACITY;
    if (root.contains("defaultBagCapacity"s)) {
        bag_capacity = root["defaultBagCapacity"s].as_uint64();
    }

    std::chrono::milliseconds period(0);
    double probability = 0.0;
    if (root.contains("lootGeneratorConfig"s)) {
        constexpr int MS_IN_S = 1000;
        const auto& loot_config = root["lootGeneratorConfig"s].as_object();
        period = std::chrono::milliseconds(static_cast<int>(loot_config.at("period"s).as_double()) * MS_IN_S);
        probability = loot_config.at("probability"s).as_double();
    }
    // auto loot_generator_ptr = std::make_shared<loot_gen::LootGenerator>(period, probability, loot_gen::GenerateRandomBase);
    auto loot_generator_ptr = std::make_shared<loot_gen::LootGenerator>(period, probability);
    game.AddLootGenerator(loot_generator_ptr);

    if (root.contains("maps"s)) {
        extra_data::ExtraDataStorage extra_data;
        for (const auto& map_json : root["maps"s].as_array()) {
            const json::object& map_obj = map_json.as_object();

            auto map_id = model::Map::Id(map_obj.at(OFFICE_ID).as_string().c_str());
            std::string map_name = map_obj.at("name"s).as_string().c_str();

            // Получаем скорость для конкретной карты, если она указана
            double map_dog_speed = dog_speed;
            if (map_obj.contains("dogSpeed"s)) {
                map_dog_speed = map_obj.at("dogSpeed"s).as_double();
            }

            // Получаем вместимость рюкзака для конкретной карты, если она указана
            size_t map_bag_capacity = bag_capacity;
            if (map_obj.contains("bagCapacity"s)) {
                map_bag_capacity = map_obj.at("bagCapacity"s).as_uint64();
            }

            // Создаем базовую карту
            model::Map map(map_id, map_name, map_dog_speed, map_bag_capacity);

            if (map_obj.contains("lootTypes"s)) {
                auto loot_types = map_obj.at("lootTypes"s).as_array();
                map.SetLootTypesCount(loot_types.size());
                extra_data.AddLootTypesJson(std::move(loot_types));
            }
            map.AddExtraData(std::move(extra_data));

            // Парсим и добавляем дороги, здания и офисы
            map = ParseRoads(map_obj, std::move(map));
            map = ParseBuildings(map_obj, std::move(map));
            map = ParseOffices(map_obj, std::move(map));

            // Добавляем карту в игру
            game.AddMap(std::move(map));
        }
    }
    return game;
}

}  // namespace json_loader

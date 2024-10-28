#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>
#include <memory>
#include <cassert>
#include <chrono>

#include "tagged.h"
//#include "app.h"


namespace app {

struct DogSpeed {
    double sx = 0.0;
    double sy = 0.0;
};

using PlayerDogId = uint32_t;

struct DogPosition {
    double x = 0.0;
    double y = 0.0;
};


enum class Direction {
    NORTH,
    SOUTH,
    WEST,
    EAST
};


class Dog {
public:
    Dog(std::string dog_name, PlayerDogId dog_id);

    [[nodiscard]] const std::string& GetName() const noexcept;

    [[nodiscard]] PlayerDogId GetId() const;

    [[nodiscard]] DogPosition GetPosition() const;

    [[nodiscard]] Direction GetDirection() const;

    [[nodiscard]] DogSpeed GetDogSpeed() const;

    void SetDogSpeed(DogSpeed speed);

    void SetDogPosition(DogPosition dog_position);

    [[nodiscard]] DogPosition GetDogPosition() const;

    void SetDogDirection(Direction direction);

private:
    PlayerDogId dog_id_;
    std::string dog_name_;
    DogPosition dog_position_;
    DogSpeed dog_speed_;
    Direction dog_direction_ = Direction::NORTH;
};

} // namespace app

namespace model {


using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept;

    Road(VerticalTag, Point start, Coord end_y) noexcept;

    bool IsHorizontal() const noexcept;

    bool IsVertical() const noexcept;

    Point GetStart() const noexcept;

    Point GetEnd() const noexcept;

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept;

    const Rectangle& GetBounds() const noexcept;

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept;

    const Id& GetId() const noexcept;

    Point GetPosition() const noexcept;

    Offset GetOffset() const noexcept;

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name, double dog_speed) noexcept;

    const Id& GetId() const noexcept;

    const std::string& GetName() const noexcept;

    const Buildings& GetBuildings() const noexcept;

    const Roads& GetRoads() const noexcept;

    const Offices& GetOffices() const noexcept;

    void AddRoad(const Road& road);

    std::vector<size_t> GetRoadsByPosition(const app::DogPosition& pos) const;

    void AddBuilding(const Building& building);

    void AddOffice(Office office);

    double GetDogSpeed() const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    double dog_speed_;

    // Хеш-функция для unordered_map с ключом pair<int, int>
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            return std::hash<T1>{}(p.first) ^ (std::hash<T2>{}(p.second) << 1);
        }
    };
    std::unordered_map<std::pair<int, int>, std::vector<size_t>, pair_hash> cells_;  // Сетка ячеек

    // Преобразование позиции в индекс ячейки
    static std::pair<int, int> GetCellIndex(const app::DogPosition& pos) ;
};

class GameSession {
public:
    // Конструктор, который связывает сессию с картой
    explicit GameSession(const Map* map);

    // Добавление собаки/игрока в сессию
    std::shared_ptr<app::Dog> AddDog(const std::string& player_name);

    // Возвращает количество игроков в сессии
    [[nodiscard]] size_t GetPlayerCount() const noexcept;

    // Возвращает карту, с которой связана сессия
    [[nodiscard]] const Map* GetMap() const noexcept;

    std::vector<std::shared_ptr<app::Dog>> & GetDogs();
private:
    std::vector<std::shared_ptr<app::Dog>> dogs_;
    const Map* map_;
    std::unordered_map<uint32_t, size_t> dog_id_to_index_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept;

    const Map* FindMap(const Map::Id& id) const noexcept;

    std::shared_ptr<GameSession> AddSession(const Map::Id& map_id);

    std::shared_ptr<GameSession> FindSession(const Map::Id& map_id);

    void Update(std::chrono::milliseconds time_delta_ms);

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using MapIdToSessionIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using Sessions = std::vector<std::shared_ptr<GameSession>>;

    Maps maps_;
    MapIdToIndex map_id_to_index_;
    Sessions sessions_;
    MapIdToSessionIndex map_id_to_session_index_;
};

}  // namespace model

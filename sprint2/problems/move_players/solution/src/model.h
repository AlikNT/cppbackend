#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>
#include <memory>
#include <cassert>

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

private:
    PlayerDogId dog_id_;
    std::string dog_name_;
    DogPosition dog_position_;
    DogSpeed dog_speed_;
    Direction dog_direction_ = Direction::NORTH;
};

}

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

    void AddBuilding(const Building& building);

    void AddOffice(Office office);

    void SetDogSpeed(double speed);

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
};

class GameSession {
public:
    // Конструктор, который связывает сессию с картой
    explicit GameSession(const Map* map);

    // Добавление собаки/игрока в сессию
    app::Dog* AddDog(const std::string& player_name);

    // Возвращает количество игроков в сессии
    [[nodiscard]] size_t GetPlayerCount() const noexcept;

    // Возвращает карту, с которой связана сессия
    [[nodiscard]] const Map* GetMap() const noexcept;

    const std::vector<app::Dog> & GetPlayers() const;
private:
    std::vector<app::Dog> dogs_;
    const Map* map_;
    std::unordered_map<uint32_t, size_t> dog_id_to_index_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept;

    const Map* FindMap(const Map::Id& id) const noexcept;

    GameSession* AddSession(const Map::Id& map_id);

    GameSession* FindSession(const Map::Id& map_id);

    void SetDefaultDogSpeed(double speed);

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using MapIdToSessionIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using Sessions = std::vector<GameSession>;

    Maps maps_;
    MapIdToIndex map_id_to_index_;
    Sessions sessions_;
    MapIdToSessionIndex map_id_to_session_index_;
    double default_dog_speed_ = 1.0;
};

}  // namespace model

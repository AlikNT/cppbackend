#pragma once
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <random>
#include <iomanip>
#include <memory>
#include <chrono>
#include <variant>

#include "collision_detector.h"
#include "extra_data.h"
#include "loot_generator.h"
#include "tagged.h"


namespace app {

struct DogSpeed {
    double sx = 0.0;
    double sy = 0.0;
};

using PlayerDogId = uint32_t;

struct Position {
    double x = 0.0;
    double y = 0.0;
};

using DogPosition = Position;
using LootPosition = Position;

enum class Direction {
    NORTH,
    SOUTH,
    WEST,
    EAST
};

class Loot;

class Dog {
public:
    using LootId = size_t;
    using LootPtr = std::shared_ptr<Loot>;

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

    void AddLoot(const LootPtr& loot_ptr);

    [[nodiscard]] size_t GetLootsCountInBag() const;

    [[nodiscard]] std::vector<LootPtr> GetLootsInBag() const;

    void ClearLootsInBag();

    void AddScoreValue(unsigned score);

    [[nodiscard]] unsigned GetScore() const;

private:
    PlayerDogId dog_id_;
    std::string dog_name_;
    DogPosition dog_position_;
    DogSpeed dog_speed_;
    Direction dog_direction_ = Direction::NORTH;
    std::vector<LootPtr> loots_in_bag_;
    unsigned score_{0};
};

enum class LootStatus {
    BAG,
    ROAD,
    STATUS
};

class Loot {
public:
    Loot(const unsigned loot_type, const LootPosition &loot_position)
        : loot_type_id_(loot_type),
          loot_position_(loot_position),
          loot_status_(LootStatus::ROAD) {
    }

    [[nodiscard]] unsigned GetLootTypeId() const noexcept;

    [[nodiscard]] LootPosition GetLootPosition() const;

    void SetLootStatus(LootStatus loot_status);

    [[nodiscard]] LootStatus GetLootStatus() const;

private:
    unsigned loot_type_id_{};
    LootPosition loot_position_;
    LootStatus loot_status_;
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

    Map(Id id, std::string name, double dog_speed, size_t bag_capacity) noexcept;

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

    void AddExtraData(extra_data::ExtraDataStorage extra_data);

    [[nodiscard]] const extra_data::ExtraDataStorage & GetExtraData() const;

    [[nodiscard]] unsigned GetLootTypesCount() const;

    [[nodiscard]] unsigned GetBagCapacity() const;

    void AddLootValue(unsigned value) noexcept;

    unsigned GetLootValue(size_t index) const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    double dog_speed_;
    size_t bag_capacity_;
    extra_data::ExtraDataStorage extra_data_;
    std::vector<unsigned> loot_values_;

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
    using Dogs = std::vector<std::shared_ptr<app::Dog>>;
    using Loots = std::vector<std::shared_ptr<app::Loot>>;

    // Конструктор, который связывает сессию с картой
    explicit GameSession(const Map* map);

    // Добавление собаки/игрока в сессию
    std::shared_ptr<app::Dog> AddDog(const std::string& player_name);

    // Добавление трофея
    void AddLoots(size_t loots_count);

    // Возвращает количество игроков в сессии
    [[nodiscard]] size_t GetPlayerCount() const noexcept;

    // Возвращает карту, с которой связана сессия
    [[nodiscard]] const Map* GetMap() const noexcept;

    std::vector<std::shared_ptr<app::Dog>> & GetDogs();

    size_t GetLootsCount() const;

    Loots GetLoots() const;

    size_t GetDogIndexById(app::PlayerDogId id) const;

    std::shared_ptr<app::Dog> GetDogById(app::PlayerDogId id) const;

    size_t GetIndexByLootPtr(const std::shared_ptr<app::Loot> &loot_ptr) const;

private:
    Dogs dogs_;
    const Map* map_;
    std::unordered_map<app::PlayerDogId, size_t> dog_id_to_index_;
    Loots loots_;
    std::unordered_map<std::shared_ptr<app::Loot>, size_t> loots_to_id_;
};

class Game {
public:
    using LootGeneratorPtr = std::shared_ptr<loot_gen::LootGenerator>;
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept;

    const Map* FindMap(const Map::Id& id) const noexcept;

    std::shared_ptr<GameSession> AddSession(const Map::Id& map_id);

    std::shared_ptr<GameSession> FindSession(const Map::Id& map_id);

    void Tick(std::chrono::milliseconds time_delta_ms);

    void AddLootGenerator(const LootGeneratorPtr &generator_ptr);

    LootGeneratorPtr GetLootGenerator() const;

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using MapIdToSessionIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using Sessions = std::vector<std::shared_ptr<GameSession>>;

    Maps maps_;
    MapIdToIndex map_id_to_index_;
    Sessions sessions_;
    MapIdToSessionIndex map_id_to_session_index_;
    LootGeneratorPtr loot_generator_ptr_;
};

}  // namespace model

namespace app {

class ItemGathererProvider : public collision_detector::ItemGathererProvider {
public:
    using Items = std::vector<collision_detector::Item>;
    using Gatherers = std::vector<collision_detector::Gatherer>;
    using ItemIndex = size_t;
    using GathererIndex = size_t;
    using LootPtr = std::shared_ptr<Loot>;
    using MapObject = std::variant<LootPtr, model::Office::Id>;
    using MapObjectById = std::unordered_map<ItemIndex, MapObject>;
    using DogById = std::unordered_map<GathererIndex, PlayerDogId>;

    static constexpr double GATHERER_WIDTH = 0.6;

    template <typename Object>
    void AddItem(geom::Point2D position, double width, Object session_object);

    void AddGatherer(geom::Point2D start_pos, geom::Point2D end_pos, PlayerDogId dog_id);

    size_t ItemsCount() const override;

    collision_detector::Item GetItem(size_t idx) const override;

    MapObject GetMapObjectById(ItemIndex item_index) const;

    size_t GatherersCount() const override;

    collision_detector::Gatherer GetGatherer(size_t idx) const override;

    PlayerDogId GetDogById(GathererIndex gather_index) const;

    virtual ~ItemGathererProvider() = default;

private:
    Items items_;
    Gatherers gatherers_;
    MapObjectById map_object_by_id_;
    DogById dog_by_id_;
};

template<typename Object>
void ItemGathererProvider::AddItem(const geom::Point2D position, const double width, Object session_object) {
    items_.emplace_back(collision_detector::Item{position, width});
    map_object_by_id_[items_.size() - 1] = std::move(session_object);
}
} // namespace app

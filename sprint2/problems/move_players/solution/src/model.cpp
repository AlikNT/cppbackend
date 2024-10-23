#include "model.h"

#include <stdexcept>

namespace app {

Dog::Dog(std::string dog_name, PlayerDogId dog_id)
        : dog_name_(std::move(dog_name))
        , dog_id_(dog_id) {}

const std::string &Dog::GetName() const noexcept {
    return dog_name_;
}

PlayerDogId Dog::GetId() const {
    return dog_id_;
}

DogPosition Dog::GetPosition() const {
    return dog_position_;
}


Direction Dog::GetDirection() const {
    return dog_direction_;
}

DogSpeed Dog::GetDogSpeed() const {
   return dog_speed_;
}

void Dog::SetDogSpeed(DogSpeed speed) {
    dog_speed_ = speed;
}

}

namespace model {
using namespace std::literals;

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

Map::Map(Map::Id id, std::string name, double dog_speed) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , dog_speed_(dog_speed) {
}

const Map::Id &Map::GetId() const noexcept {
    return id_;
}

const std::string &Map::GetName() const noexcept {
    return name_;
}

const Map::Buildings &Map::GetBuildings() const noexcept {
    return buildings_;
}

const Map::Roads &Map::GetRoads() const noexcept {
    return roads_;
}

const Map::Offices &Map::GetOffices() const noexcept {
    return offices_;
}

void Map::AddRoad(const Road &road) {
    roads_.emplace_back(road);
}

void Map::AddBuilding(const Building &building) {
    buildings_.emplace_back(building);
}

void Map::SetDogSpeed(double speed) {
    dog_speed_ = speed;
}

double Map::GetDogSpeed() const {
    return dog_speed_;
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

const Game::Maps &Game::GetMaps() const noexcept {
    return maps_;
}

const Map *Game::FindMap(const Map::Id &id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

GameSession * Game::AddSession(const Map::Id &map_id) {
    const Map* map = FindMap(map_id);
    if (!map) {
        throw std::runtime_error("Map with the given ID does not exist");
    }

    // Проверяем, существует ли уже сессия для этой карты
    if (map_id_to_session_index_.find(map_id) != map_id_to_session_index_.end()) {
        throw std::runtime_error("Session for this map already exists");
    }

    // Добавляем сессию в вектор
    sessions_.emplace_back(map);

    // Связываем идентификатор карты с индексом новой сессии
    map_id_to_session_index_[map_id] = sessions_.size() - 1;

    return &sessions_.back();
}

GameSession *Game::FindSession(const Map::Id &map_id) {
    auto it = map_id_to_session_index_.find(map_id);
    if (it != map_id_to_session_index_.end()) {
        return &sessions_.at(it->second);
    }
    return nullptr;
}

void Game::SetDefaultDogSpeed(double speed) {
    default_dog_speed_ = speed;
}

Road::Road(Road::HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
}

Road::Road(Road::VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept {
    return end_;
}

Building::Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
}

const Rectangle &Building::GetBounds() const noexcept {
    return bounds_;
}

Office::Office(Office::Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
}

const Office::Id &Office::GetId() const noexcept {
    return id_;
}

Point Office::GetPosition() const noexcept {
    return position_;
}

Offset Office::GetOffset() const noexcept {
    return offset_;
}

GameSession::GameSession(const Map *map) : map_(map) {}

app::Dog * GameSession::AddDog(const std::string &player_name) {
    dogs_.emplace_back(player_name, dogs_.size());
    app::Dog * dog = &dogs_.back();
    dog_id_to_index_[dog->GetId()] = dogs_.size() - 1;
    return dog;
}

size_t GameSession::GetPlayerCount() const noexcept {
    return dogs_.size();
}

const Map *GameSession::GetMap() const noexcept {
    return map_;
}

const std::vector<app::Dog> & GameSession::GetPlayers() const {
    return dogs_;
}

}  // namespace model

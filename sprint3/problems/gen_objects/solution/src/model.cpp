#include "model.h"

#include <stdexcept>

namespace app {

Dog::Dog(std::string dog_name, PlayerDogId dog_id)
        : dog_id_(dog_id)
        , dog_name_(std::move(dog_name)) {}

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

DogPosition Dog::GetDogPosition() const {
    return dog_position_;
}

void Dog::SetDogPosition(DogPosition dog_position) {
    dog_position_ = dog_position;
}

void Dog::SetDogDirection(Direction direction) {
    dog_direction_ = direction;
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
    size_t index = roads_.size() - 1;
    int x1 = road.GetStart().x;
    int x2 = road.GetEnd().x;
    int y1 = road.GetStart().y;
    int y2 = road.GetEnd().y;
    if (road.IsHorizontal()) {
        for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) {
            cells_[{x, road.GetStart().y}].push_back(index);
        }
    } else {
        for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) {
            cells_[{road.GetStart().x, y}].push_back(index);
        }
    }
}

void Map::AddBuilding(const Building &building) {
    buildings_.emplace_back(building);
}

double Map::GetDogSpeed() const {
    return dog_speed_;
}

void Map::SetExtraData(extra_data::ExtraDataStorage extra_data) {
    extra_data_ = std::move(extra_data);
}

std::vector<size_t> Map::GetRoadsByPosition(const app::DogPosition &pos) const {
    auto [cell_x, cell_y] = GetCellIndex(pos);

    auto it = cells_.find({cell_x, cell_y});
    if (it != cells_.end()) {
        return it->second;
    }
    return {};
}

int CustomRound(double num) {
    constexpr double HALF_ROAD_WIDTH = 0.4;

    num = std::abs(num);
    int integerPart = static_cast<int>(num); // Берем целую часть числа
    double fractionalPart = std::abs(num - integerPart); // Вычисляем дробную часть числа

    // Округляем в зависимости от значения дробной части
    if (fractionalPart <= HALF_ROAD_WIDTH) {
        return integerPart;
    } else {
        return integerPart + 1;
    }
}

std::pair<int, int> Map::GetCellIndex(const app::DogPosition &pos) {
    int cell_x = CustomRound(pos.x);
    int cell_y = CustomRound(pos.y);
    return {cell_x, cell_y};
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

std::shared_ptr<GameSession> Game::AddSession(const Map::Id &map_id) {
    const Map* map = FindMap(map_id);
    if (!map) {
        throw std::runtime_error("Map with the given ID does not exist");
    }

    // Проверяем, существует ли уже сессия для этой карты
    if (map_id_to_session_index_.find(map_id) != map_id_to_session_index_.end()) {
        throw std::runtime_error("Session for this map already exists");
    }

    // Добавляем сессию в вектор
    sessions_.emplace_back(std::make_shared<GameSession>(map));

    // Связываем идентификатор карты с индексом новой сессии
    map_id_to_session_index_[map_id] = sessions_.size() - 1;

    return sessions_.back();
}

std::shared_ptr<GameSession> Game::FindSession(const Map::Id &map_id) {
    auto it = map_id_to_session_index_.find(map_id);
    if (it != map_id_to_session_index_.end()) {
        return sessions_.at(it->second);
    }
    return nullptr;
}

struct Constrains {
    int x_min;
    int x_max;
    int y_min;
    int y_max;
};

bool CorrectDogPosition(const Constrains &constrains, app::DogPosition &new_p, const app::Direction &dog_dir) {
    using Direction = app::Direction;
    constexpr double HALF_ROAD_WIDTH = 0.4;
    bool res = false;

    switch (dog_dir) {
        case Direction::NORTH:
            if (new_p.y < constrains.y_min - HALF_ROAD_WIDTH) {
                new_p.y = constrains.y_min - HALF_ROAD_WIDTH;
                res = true;
            }
            break;
        case Direction::SOUTH:
            if (new_p.y > constrains.y_max + HALF_ROAD_WIDTH) {
                new_p.y = constrains.y_max + HALF_ROAD_WIDTH;
                res = true;
            }
            break;
        case Direction::WEST:
            if (new_p.x < constrains.x_min - HALF_ROAD_WIDTH) {
                new_p.x = constrains.x_min - HALF_ROAD_WIDTH;
                res = true;
            }
            break;
        case Direction::EAST:
            if (new_p.x > constrains.x_max + HALF_ROAD_WIDTH) {
                new_p.x = constrains.x_max + HALF_ROAD_WIDTH;
                res = true;
            }
            break;
    }
    return res;
}

void Game::Update(std::chrono::milliseconds time_delta_ms) {
    constexpr double MS_IN_S = 1000;
    auto time_delta = time_delta_ms.count();
    for (auto& session : sessions_) {
        for (auto& dog : session->GetDogs()) {
            app::DogPosition pos = dog->GetDogPosition();
            app::DogSpeed speed = dog->GetDogSpeed();

            // Предварительно рассчитываем новую позицию
            app::DogPosition new_pos = pos;
            new_pos.x += speed.sx * static_cast<double>(time_delta) / MS_IN_S;
            new_pos.y += speed.sy * static_cast<double>(time_delta) / MS_IN_S;

            // Получаем список дорог на текущей позиции
            std::vector<size_t> on_roads_indexes = session->GetMap()->GetRoadsByPosition(pos);
            Constrains constrains{0, 0, 0, 0};
            for (auto it = on_roads_indexes.begin(); it != on_roads_indexes.end(); ++it) {
                const Road& road = session->GetMap()->GetRoads()[*it];
                // Инициализируем ограничения по первой дороге
                if (it == on_roads_indexes.begin()) {
                    constrains.x_min = std::min(road.GetStart().x, road.GetEnd().x);
                    constrains.x_max = std::max(road.GetStart().x, road.GetEnd().x);
                    constrains.y_min = std::min(road.GetStart().y, road.GetEnd().y);
                    constrains.y_max = std::max(road.GetStart().y, road.GetEnd().y);
                } else {
                    constrains.x_min = std::min(std::min(road.GetStart().x, road.GetEnd().x), constrains.x_min);
                    constrains.x_max = std::max(std::max(road.GetStart().x, road.GetEnd().x), constrains.x_max);
                    constrains.y_min = std::min(std::min(road.GetStart().y, road.GetEnd().y), constrains.y_min);
                    constrains.y_max = std::max(std::max(road.GetStart().y, road.GetEnd().y), constrains.y_max);
                }
            }
            if (CorrectDogPosition(constrains, new_pos, dog->GetDirection())) {
                dog->SetDogSpeed({0.0, 0.0});
            }
            dog->SetDogPosition(new_pos);
        }
    }
}

void Game::AddLootGenerator(loot_gen::LootGenerator generator) {
    generator = std::move(generator);
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

std::shared_ptr<app::Dog> GameSession::AddDog(const std::string &player_name) {
    dogs_.push_back(std::make_shared<app::Dog>(player_name, dogs_.size()));
    auto dog = dogs_.back();
    dog_id_to_index_[dog->GetId()] = dogs_.size() - 1;

    // Устанавливаем начальную позицию на первой дороге карты
    if (!map_->GetRoads().empty()) {
        const Road& first_road = map_->GetRoads().front();
        app::DogPosition start_pos = { static_cast<double>(first_road.GetStart().x), static_cast<double>(first_road.GetStart().y) };
        dog->SetDogPosition(start_pos);
    }
    return dog;
}

size_t GameSession::GetPlayerCount() const noexcept {
    return dogs_.size();
}

const Map *GameSession::GetMap() const noexcept {
    return map_;
}

std::vector<std::shared_ptr<app::Dog>> & GameSession::GetDogs() {
    return dogs_;
}

}  // namespace model

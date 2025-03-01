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

DogPosition Dog::GetDogPosition() const {
    return dog_position_;
}

void Dog::SetDogPosition(DogPosition dog_position) {
    dog_position_ = dog_position;
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
    grid_.AddRoad(roads_.back());
}

void Map::AddBuilding(const Building &building) {
    buildings_.emplace_back(building);
}

double Map::GetDogSpeed() const {
    return dog_speed_;
}

std::vector<const Road *> Map::GetRoadsNearPosition(const app::DogPosition &pos) const {
    return grid_.GetRoadsInCell(pos);
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

bool IsDogOnRoad(const app::Dog& dog, const Road& road) {
    app::DogPosition pos = dog.GetDogPosition();

    if (road.IsHorizontal()) {
        // Для горизонтальной дороги: проверяем, находится ли собака в пределах ширины дороги (по оси Y)
        double upper_edge = road.GetStart().y + 0.4;
        double lower_edge = road.GetStart().y - 0.4;

        // Проверяем, находится ли X собаки между началом и концом дороги
        return pos.x >= road.GetStart().x && pos.x <= road.GetEnd().x && pos.y >= lower_edge && pos.y <= upper_edge;
    } else if (road.IsVertical()) {
        // Для вертикальной дороги: проверяем, находится ли собака в пределах ширины дороги (по оси X)
        double left_edge = road.GetStart().x - 0.4;
        double right_edge = road.GetStart().x + 0.4;

        // Проверяем, находится ли Y собаки между началом и концом дороги
        return pos.y >= road.GetStart().y && pos.y <= road.GetEnd().y && pos.x >= left_edge && pos.x <= right_edge;
    }
    return false;
}
void Game::Update(int time_delta) {
    for (auto& session : sessions_) {
        for (auto& dog : session.GetPlayers()) {
            app::DogPosition pos = dog.GetDogPosition();
            app::DogSpeed speed = dog.GetDogSpeed();

            // Предварительно рассчитываем новую позицию
            app::DogPosition new_pos = pos;
            new_pos.x += speed.sx * time_delta;
            new_pos.y += speed.sy * time_delta;

            // Получаем список дорог рядом с текущей позицией
            std::vector<const Road*> roads_nearby = session.GetMap()->GetRoadsNearPosition(pos);

            bool on_road = false;
            for (const Road* road : roads_nearby) {
                if (IsDogOnRoad(dog, *road)) {
                    // Если новая позиция не выходит за границы дороги, обновляем её
                    dog.SetDogPosition(new_pos);
                    on_road = true;
                    break;
                }
            }

            if (!on_road) {
                // Если собака не на дороге, её скорость обнуляется полностью
                dog.SetDogSpeed({0.0, 0.0});
            }
        }
    }
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

std::vector<app::Dog> & GameSession::GetPlayers() {
    return dogs_;
}

void Grid::AddRoad(const Road& road) {
    auto [start_x, start_y] = GetCellIndex({static_cast<double>(road.GetStart().x), static_cast<double>(road.GetStart().y)});
    auto [end_x, end_y] = GetCellIndex({static_cast<double>(road.GetEnd().x), static_cast<double>(road.GetEnd().y)});

    // Проходим по всем ячейкам между началом и концом дороги
    for (int x = std::min(start_x, end_x); x <= std::max(start_x, end_x); ++x) {
        for (int y = std::min(start_y, end_y); y <= std::max(start_y, end_y); ++y) {
            cells_[{x, y}].push_back(&road);
        }
    }
}

std::pair<int, int> Grid::GetCellIndex(const app::DogPosition& pos) const {
    int cell_x = static_cast<int>(std::floor(pos.x));  // Размер ячейки равен 1, округляем вниз
    int cell_y = static_cast<int>(std::floor(pos.y));  // Размер ячейки равен 1, округляем вниз
    return {cell_x, cell_y};
}

std::vector<const Road*> Grid::GetRoadsInCell(const app::DogPosition& pos) const {
    auto [cell_x, cell_y] = GetCellIndex(pos);

    auto it = cells_.find({cell_x, cell_y});
    if (it != cells_.end()) {
        return it->second;
    }
    return {};
}

}  // namespace model

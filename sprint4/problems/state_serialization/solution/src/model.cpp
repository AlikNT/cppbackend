#include "model.h"

#include <stdexcept>

using namespace std::literals;

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

void Dog::AddLoot(const LootPtr& loot_ptr) {
    loots_in_bag_.emplace_back(loot_ptr);
}

size_t Dog::GetLootsCountInBag() const {
    return loots_in_bag_.size();
}

std::vector<Dog::LootPtr> Dog::GetLootsInBag() const {
    return loots_in_bag_;
}

void Dog::ClearLootsInBag() {
    loots_in_bag_.clear();
}

void Dog::AddScoreValue(const unsigned score) {
    score_ += score;
}

unsigned Dog::GetScore() const {
    return score_;
}

unsigned Loot::GetLootTypeId() const noexcept {
    return loot_type_id_;
}

LootPosition Loot::GetLootPosition() const {
    return loot_position_;
}

void Loot::SetLootStatus(const LootStatus loot_status) {
    loot_status_ = loot_status;
}

LootStatus Loot::GetLootStatus() const {
    return loot_status_;
}

void ItemGathererProvider::AddGatherer(const geom::Point2D start_pos, const geom::Point2D end_pos,
    const PlayerDogId dog_id) {
    gatherers_.emplace_back(collision_detector::Gatherer{start_pos, end_pos, GATHERER_WIDTH});
    dog_by_id_[gatherers_.size() - 1] = dog_id;
}

size_t ItemGathererProvider::ItemsCount() const {
    return items_.size();
}

collision_detector::Item ItemGathererProvider::GetItem(size_t idx) const {
    if (idx >= items_.size()) {
        throw std::invalid_argument("index out of range"s);
    }
    return items_[idx];
}

ItemGathererProvider::MapObject ItemGathererProvider::GetMapObjectById(const ItemIndex item_index) const {
    if (map_object_by_id_.contains(item_index)) {
        return map_object_by_id_.at(item_index);
    }
    throw std::invalid_argument("Invalid item index"s);
}

size_t ItemGathererProvider::GatherersCount() const {
    return gatherers_.size();
}

collision_detector::Gatherer ItemGathererProvider::GetGatherer(size_t idx) const {
    if (idx >= gatherers_.size()) {
        throw std::invalid_argument("Invalid index"s);
    }
    return gatherers_[idx];
}

PlayerDogId ItemGathererProvider::GetDogById(GathererIndex gather_index) const {
    if (dog_by_id_.contains(gather_index)) {
        return dog_by_id_.at(gather_index);
    }
    throw std::invalid_argument("Invalid gather index"s);
}
}

namespace model {
using namespace std::literals;

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse"s);
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

Map::Map(Map::Id id, std::string name, const double dog_speed, const size_t bag_capacity) noexcept
        : id_(std::move(id)),
        name_(std::move(name)),
        dog_speed_(dog_speed),
        bag_capacity_(bag_capacity) {
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

void Map::AddExtraData(extra_data::ExtraDataStorage extra_data) {
    extra_data_ = std::move(extra_data);
}

const extra_data::ExtraDataStorage & Map::GetExtraData() const {
    return extra_data_;
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
        throw std::runtime_error("Map with the given ID does not exist"s);
    }

    // Проверяем, существует ли уже сессия для этой карты
    if (map_id_to_session_index_.contains(map_id)) {
        throw std::runtime_error("Session for this map already exists"s);
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

Constrains CalculateConstrains(const std::vector<size_t>& on_roads_indexes, const GameSession& session) {
    Constrains constrains{0, 0, 0, 0};
    for (auto it = on_roads_indexes.begin(); it != on_roads_indexes.end(); ++it) {
        const Road &road = session.GetMap()->GetRoads()[*it];
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
    return constrains;
}

void ActDogsOnTick (GameSession &session, app::ItemGathererProvider& provider, const uint64_t time_delta) {
    for (const auto &dog: session.GetDogs()) {
        constexpr double MS_IN_S = 1000;
        app::DogPosition pos = dog->GetDogPosition();
        app::DogSpeed speed = dog->GetDogSpeed();

        // Предварительно рассчитываем новую позицию
        app::DogPosition new_pos = pos;
        new_pos.x += speed.sx * static_cast<double>(time_delta) / MS_IN_S;
        new_pos.y += speed.sy * static_cast<double>(time_delta) / MS_IN_S;

        // Получаем список дорог на текущей позиции
        std::vector<size_t> on_roads_indexes = session.GetMap()->GetRoadsByPosition(pos);
        // Считаем ограничения по дорогам, которые пересекают текущую позицию
        Constrains constrains = CalculateConstrains(on_roads_indexes, session);
        if (CorrectDogPosition(constrains, new_pos, dog->GetDirection())) {
            dog->SetDogSpeed({0.0, 0.0});
        }
        dog->SetDogPosition(new_pos);
        provider.AddGatherer({pos.x, pos.y}, {new_pos.x, new_pos.y}, dog->GetId());
    }
}

void AddLootsToGathererProvider(const GameSession &session, app::ItemGathererProvider& provider) {
    for (const auto &loot: session.GetLoots()) {
        constexpr double LOOT_WIDTH = 0.0;
        if (loot->GetLootStatus() != app::LootStatus::ROAD) {
            continue;
        }
        provider.AddItem({loot->GetLootPosition().x, loot->GetLootPosition().y}, LOOT_WIDTH, loot);
    }
}

void AddOfficesToGathererProvider(const GameSession &session, app::ItemGathererProvider& provider) {
    for (const auto &office: session.GetMap()->GetOffices()) {
        constexpr double OFFICE_WIDTH = 0.5;
        const geom::Point2D office_position = {
            static_cast<double>(office.GetPosition().x), static_cast<double>(office.GetPosition().y)
        };
        provider.AddItem(office_position, OFFICE_WIDTH, office.GetId());
    }
}

void DetectCollisions(const GameSession& session, const app::ItemGathererProvider& provider) {
    auto events = collision_detector::FindGatherEvents(provider);
    std::sort(events.begin(), events.end(),
              [](const collision_detector::GatheringEvent &a, const collision_detector::GatheringEvent &b) {
                  return a.time < b.time;
              });
    for (const auto& event: events) {
        auto map_object = provider.GetMapObjectById(event.item_id);
        auto dog = session.GetDogById(event.gatherer_id);
        if (std::holds_alternative<std::shared_ptr<app::Loot> >(map_object)) {
            auto loot = std::get<std::shared_ptr<app::Loot> >(map_object);
            if (dog->GetLootsCountInBag() < session.GetMap()->GetBagCapacity()) {
                loot->SetLootStatus(app::LootStatus::BAG);
                dog->AddLoot(loot);
            }
        } else {
            for (const auto &loot: dog->GetLootsInBag()) {
                dog->AddScoreValue(session.GetMap()->GetLootValue(loot->GetLootTypeId()));
            }
            dog->ClearLootsInBag();
        }
    }
}

void Game::Tick(const std::chrono::milliseconds time_delta_ms) {
    const auto time_delta = time_delta_ms.count();
    app::ItemGathererProvider provider;
    for (const auto& session : sessions_) {
        // Перемещение собак, добавление в провайдер для расчета столкновений
        ActDogsOnTick(*session, provider, time_delta);
        // Добавление трофеев в провайдер для расчета столкновений
        AddLootsToGathererProvider(*session, provider);
        // Добавление баз в провайдер для расчета столкновений
        AddOfficesToGathererProvider(*session, provider);
        // Расчет столкновений и действия, связанные с этим
        DetectCollisions(*session, provider);
        // Добавление случайных трофеев на дороги
        session->AddLoots(loot_generator_ptr_->Generate(time_delta_ms, session->GetLootsCount(), session->GetDogs().size()));
    }
}

void Game::AddLootGenerator(const LootGeneratorPtr &generator_ptr) {
    loot_generator_ptr_ = generator_ptr;
}

Game::LootGeneratorPtr Game::GetLootGenerator() const {
    return loot_generator_ptr_;
}

unsigned Map::GetLootTypesCount() const {
    return loot_values_.size();
}

unsigned Map::GetBagCapacity() const {
    return bag_capacity_;
}

void Map::AddLootValue(unsigned value) noexcept {
    loot_values_.emplace_back(value);
}

unsigned Map::GetLootValue(const size_t index) const {
    if (index >= GetLootTypesCount()) {
        throw std::out_of_range("index out of range");
    }
    return loot_values_.at(index);
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

void GameSession::AddLoots(const size_t loots_count) {
    for (unsigned i = 0; i < loots_count; ++i) {
        const unsigned random_road_index = loot_gen::GenerateRandomUnsigned(0, map_->GetRoads().size() - 1);
        auto road = map_->GetRoads()[random_road_index];
        double random_loot_x;
        double random_loot_y;
        constexpr double HALF_ROAD_WIDTH = 0.4;
        if (road.IsHorizontal()) {
            const int road_x1 = std::min(road.GetStart().x, road.GetEnd().x);
            const int road_x2 = std::max(road.GetStart().x, road.GetEnd().x);
            const int road_y = road.GetStart().y;
            random_loot_x = loot_gen::GenerateRandomDouble(road_x1 - HALF_ROAD_WIDTH, road_x2 + HALF_ROAD_WIDTH);
            random_loot_y = loot_gen::GenerateRandomDouble(road_y - HALF_ROAD_WIDTH, road_y + HALF_ROAD_WIDTH);
        } else {
            const int road_y1 = std::min(road.GetStart().y, road.GetEnd().y);
            const int road_y2 = std::max(road.GetStart().y, road.GetEnd().y);
            const int road_x = road.GetStart().x;
            random_loot_x = loot_gen::GenerateRandomDouble(road_x - HALF_ROAD_WIDTH, road_x + HALF_ROAD_WIDTH);
            random_loot_y = loot_gen::GenerateRandomDouble(road_y1 - HALF_ROAD_WIDTH, road_y2 + HALF_ROAD_WIDTH);
        }
        const auto random_loot_type = loot_gen::GenerateRandomUnsigned(0, map_->GetLootTypesCount() - 1);
        auto loot_ptr = std::make_shared<app::Loot>(app::Loot{random_loot_type, {random_loot_x, random_loot_y}});
        loots_.emplace_back(loot_ptr);
        loots_to_id_[loot_ptr] = loots_.size() - 1;
    }
}

size_t GameSession::GetPlayersCount() const noexcept {
    return dogs_.size();
}

const Map *GameSession::GetMap() const noexcept {
    return map_;
}

std::vector<std::shared_ptr<app::Dog>> & GameSession::GetDogs() {
    return dogs_;
}

size_t GameSession::GetLootsCount() const {
    return std::ranges::count_if(loots_, [](const std::shared_ptr<app::Loot>& loot) {
        return loot->GetLootStatus() == app::LootStatus::ROAD;
    });
}

GameSession::Loots GameSession::GetLoots() const {
    return loots_;
}

size_t GameSession::GetDogIndexById(app::PlayerDogId id) const {
    const auto it = dog_id_to_index_.find(id);
    if (it == dog_id_to_index_.end()) {
        throw std::runtime_error("Dog not found"s);
    }
    return it->second;
}

std::shared_ptr<app::Dog> GameSession::GetDogById(app::PlayerDogId id) const {
    if (!dog_id_to_index_.contains(id)) {
        throw std::runtime_error("Invalid dog id"s);
    }
    const size_t dog_index = dog_id_to_index_.at(id);
    if (dog_index >= dogs_.size()) {
        throw std::runtime_error("Invalid dog index"s);
    }
    return dogs_[dog_index];
}

size_t GameSession::GetIndexByLootPtr(const std::shared_ptr<app::Loot> &loot_ptr) const {
    if (loots_to_id_.contains(loot_ptr)) {
        return loots_to_id_.at(loot_ptr);
    }
    throw std::invalid_argument("Invalid Loot"s);
}
}  // namespace model

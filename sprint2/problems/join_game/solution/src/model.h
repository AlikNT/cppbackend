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

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

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

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Dog {
public:
    Dog(std::string dog_name, uint32_t dog_id)
        : dog_name_(std::move(dog_name))
        , dog_id_(dog_id) {}

    [[nodiscard]] const std::string& GetName() const noexcept {
        return dog_name_;
    }

    [[nodiscard]] uint32_t GetId() const {
       return dog_id_;
    }

private:
    uint32_t dog_id_;
    std::string dog_name_;
};

class GameSession {
public:
    // Конструктор, который связывает сессию с картой
    explicit GameSession(const Map* map) : map_(map) {}

    // Добавление собаки/игрока в сессию
    const Dog& AddDog(const std::string& player_name) {

        dogs_.emplace_back(player_name, dogs_.size());
        auto& dog = dogs_.back();
        dog_id_to_index_[dog.GetId()] = dogs_.size() - 1;
        return dog;
    }

    // Возвращает количество игроков в сессии
    [[nodiscard]] size_t GetPlayerCount() const noexcept {
        return dogs_.size();
    }

    // Возвращает карту, с которой связана сессия
    [[nodiscard]] const Map* GetMap() const noexcept {
        return map_;
    }

    std::vector<Dog>& GetPlayers() {
        return dogs_;
    }
private:
    std::vector<Dog> dogs_;     // Карта, с которой связана сессия
    const Map* map_;            // Список всех собак (игроков) в сессии
    std::unordered_map<uint32_t, size_t> dog_id_to_index_;
};

class Player {
public:
    Player(Dog* dog, GameSession* session)
        : dog_(dog)
        , session_(session) {}

    Dog* GetDog() {
        return dog_;
    }

    GameSession* GetSession() {
        return session_;
    }

private:
    GameSession* session_;
    Dog* dog_;
};

class Players {
public:
    const Player& Add(Dog* dog, GameSession* session) {
        players_.emplace_back(dog, session);
        auto& player= players_.back();

        // Добавляем в карту быстрый доступ по комбинации dog_id и map_id
        if (session && session->GetMap()) {
            dog_map_to_player_[{dog->GetId(), session->GetMap()->GetId()}] = &player;
        }
        return player;
    }

    Player* FindByDogIdAndMapId(uint32_t dog_id, const Map::Id& map_id) {
        auto it = dog_map_to_player_.find({dog_id, map_id});
        if (it != dog_map_to_player_.end()) {
            return it->second;
        }
        return nullptr;  // Возвращаем nullptr, если игрок не найден
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;

    std::vector<Player> players_;

    // Используем std::pair<dog_id, map_id> как ключ для быстрого поиска игрока
    using DogMapKey = std::pair<uint32_t, Map::Id>;

    struct DogMapKeyHasher {
        std::size_t operator()(const DogMapKey& key) const {
            std::size_t h1 = std::hash<uint32_t>()(key.first);
            std::size_t h2 = util::TaggedHasher<Map::Id>()(key.second);
            return h1 ^ (h2 << 1);  // Комбинирование двух хэшей
        }
    };

    // Сопоставление комбинации dog_id и map_id с указателем на игрока
    std::unordered_map<DogMapKey, Player*, DogMapKeyHasher> dog_map_to_player_;
};

namespace detail {
struct TokenTag {};
} // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

class PlayerTokens {
public:
    // Метод для поиска игрока по токену
    std::shared_ptr<Player> FindPlayerByToken(const Token& token) {
        auto it = token_to_player_.find(token);
        if (it != token_to_player_.end()) {
            return it->second;
        }
        return nullptr;
    }

    // Метод для добавления игрока и генерации токена
    Token AddPlayer(std::shared_ptr<Player> player) {
        Token token = GenerateToken();
        token_to_player_[token] = std::move(player);
        return token;
    }

private:
    using TokenHasher = util::TaggedHasher<Token>;

    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};

    std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> token_to_player_;

    // Метод для генерации уникального токена
    Token GenerateToken() {
        std::ostringstream token_stream;

        // Генерируем два 64-разрядных случайных числа и переводим их в hex
        uint64_t part1 = generator1_();
        uint64_t part2 = generator2_();

        token_stream << std::hex << std::setw(16) << std::setfill('0') << part1
                     << std::setw(16) << std::setfill('0') << part2;

        // Создаем и возвращаем Token на основе hex-строки
        return Token(token_stream.str());
    }
};

class Game {
public:
    using Maps = std::vector<Map>;
    using Sessions = std::vector<GameSession>;
    static constexpr size_t MAX_PLAYERS_PER_SESSION = 10;

    // Конструктор, который создает объект PlayerTokens в куче
    Game()
        : player_tokens_(std::make_unique<PlayerTokens>()) {}

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    Token AddPlayer(const std::string& player_name, const Map::Id& map_id) {
        // Поиск карты
        const Map* map = FindMap(map_id);
        assert(map);

        // Поиск игровой сессии для этой карты
        GameSession* session = nullptr;
        for (auto& game_session : sessions_) {
            if (game_session.GetMap() == map && game_session.GetPlayerCount() < MAX_PLAYERS_PER_SESSION) {
                session = &game_session;
                break;
            }
        }

        // Если подходящей сессии не найдено, создаем новую
        if (!session) {
            sessions_.emplace_back(map);
            session = &sessions_.back();
        }

        // Создание собаки и добавление в сессию
        Dog dog = session->AddDog(player_name);

        // Добавление игрока
        const Player& player = players_.Add(&dog, session);

        // Генерация токена для игрока и его добавление в систему токенов
        return player_tokens_->AddPlayer(std::make_shared<Player>(player));
    }

    std::shared_ptr<Player> FindPlayerByToken(const Token& token) {
        return player_tokens_->FindPlayerByToken(token);
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    std::unique_ptr<PlayerTokens> player_tokens_;
    Maps maps_;
    MapIdToIndex map_id_to_index_;
    Players players_;
    Sessions sessions_;
};

}  // namespace model

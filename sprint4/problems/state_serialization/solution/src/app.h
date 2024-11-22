#pragma once

#include <string>
#include <optional>
#include <chrono>

#include "tagged.h"
#include "model.h"
#include "json_loader.h"

namespace app {
using PlayersList = std::optional<std::vector<std::shared_ptr<app::Dog> > >;

using PlayerDogId = uint32_t;


class Player {
public:
    Player(std::shared_ptr<Dog> dog, std::shared_ptr<model::GameSession> session);

    [[nodiscard]] std::shared_ptr<Dog> GetDogPtr() const;

    [[nodiscard]] PlayerDogId GetPlayerId() const;

    [[nodiscard]] std::shared_ptr<model::GameSession> GetSession() const;

    [[nodiscard]] app::DogSpeed GetDogSpeed() const;

private:
    std::shared_ptr<Dog> dog_;
    std::shared_ptr<model::GameSession> session_;
};

struct DogMapKeyHasher {
    std::size_t operator()(const std::pair<uint32_t, model::Map::Id> &key) const {
        std::size_t h1 = std::hash<uint32_t>()(key.first);
        std::size_t h2 = util::TaggedHasher<model::Map::Id>()(key.second);
        return h1 ^ (h2 << 1); // Комбинирование двух хэшей
    }
};

class Players {
public:
    using PlayersContainer = std::vector<std::shared_ptr<Player>>;
    // Используем std::pair<dog_id, map_id> как ключ для быстрого поиска игрока
    using DogMapKey = std::pair<uint32_t, model::Map::Id>;
    using DogMapToPlayer = std::unordered_map<DogMapKey, std::shared_ptr<Player>, DogMapKeyHasher>;

    std::shared_ptr<Player> AddPlayer(const std::shared_ptr<Dog> &dog, const std::shared_ptr<model::GameSession> &session);

    std::shared_ptr<Player> FindByDogIdAndMapId(PlayerDogId dog_id, const model::Map::Id &map_id);

    [[nodiscard]] PlayersContainer GetPlayers() const;

    DogMapToPlayer GetDogMapToPlayer() const;

    void SetPlayer(const std::shared_ptr<Player>& player_ptr) noexcept;

    void SetDogMapToPlayer(DogMapToPlayer dog_map_to_player) noexcept;

private:
    PlayersContainer players_;

    // Сопоставление комбинации dog_id и map_id с указателем на игрока
    DogMapToPlayer dog_map_to_player_;
};

namespace detail {
struct TokenTag {
};
} // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

class PlayerTokens {
public:
    // Метод для поиска игрока по токену
    std::shared_ptr<Player> FindPlayerByToken(const Token &token);

    // Метод для добавления игрока и генерации токена
    Token AddPlayer(std::shared_ptr<Player> player);

private:
    using TokenHasher = util::TaggedHasher<Token>;

    std::random_device random_device_;
    std::mt19937_64 generator1_{
        [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()
    };
    std::mt19937_64 generator2_{
        [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()
    };

    std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> token_to_player_;

    // Метод для генерации уникального токена
    Token GenerateToken();
};

bool IsValidToken(const app::Token &token);

struct JoinGameResult {
    Token token{""};
    PlayerDogId player_id{0};
};

using GameStateResult = std::vector<std::shared_ptr<Dog> >;

enum class MovePlayersResult {
    OK,
    UNKNOWN_TOKEN,
    UNKNOWN_MOVE
};

class GetMapByIdUseCase {
public:
    explicit GetMapByIdUseCase(model::Game &game);

    [[nodiscard]] json::object GetMapById(const model::Map::Id &map_id) const;

private:
    model::Game &game_model_;

    static json::array AddRoads(const model::Map *map);

    static json::array AddBuildings(const model::Map *map);

    static json::array AddOffices(const model::Map *map);
};

class JoinGameUseCase {
public:
    JoinGameUseCase(model::Game &game, PlayerTokens &player_tokens, Players &players);

    JoinGameResult JoinGame(const model::Map::Id &map_id, const std::string &name);

private:
    model::Game &game_model_;
    PlayerTokens &player_tokens_;
    Players &players_;
};

class ListPlayersUseCase {
public:
    explicit ListPlayersUseCase(PlayerTokens &player_tokens);

    PlayersList ListPlayers(const Token &token);

private:
    PlayerTokens &player_tokens_;
};

class GameStateUseCase {
public:
    explicit GameStateUseCase(PlayerTokens &player_tokens);

    json::object GameState(const Token &token);

private:
    PlayerTokens &player_tokens_;
};

class MovePlayersUseCase {
public:
    explicit MovePlayersUseCase(PlayerTokens &player_tokens);

    MovePlayersResult MovePlayers(const Token &token, std::string_view move);

private:
    PlayerTokens &player_tokens_;
};

class TickUseCase {
public:
    explicit TickUseCase(model::Game &game_model, std::chrono::milliseconds time_delta);

    void Update();

private:
    model::Game &game_model_;
    std::chrono::milliseconds time_delta_;
};

class Application {
public:
    explicit Application(model::Game &model_game);

    json::object GetMapsById(const model::Map::Id &map_id) const;

    JoinGameResult JoinGame(const model::Map::Id &map_id, const std::string &user_name);

    PlayersList ListPlayers(const Token &token);

    json::object GameState(const Token &token);

    MovePlayersResult MovePlayers(const Token &token, std::string_view move);

    void Tick(std::chrono::milliseconds time_delta);

private:
    model::Game &game_model_;
    Players players_;
    PlayerTokens player_tokens_;

    std::shared_ptr<Player> FindPlayerByToken(const Token &token);
};
} // namespace app

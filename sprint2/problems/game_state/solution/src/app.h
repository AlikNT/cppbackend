#pragma once

#include <string>
#include <optional>

#include "tagged.h"
#include "model.h"

namespace app {

using PlayerDogId = uint32_t;

class Player {
public:
    Player(Dog* dog, const model::GameSession* session);

    Dog* GetDog();

    PlayerDogId GetPlayerId();

    const model::GameSession* GetSession();

private:
    const model::GameSession* session_;
    Dog* dog_;
};

class Players {
public:
    const Player& Add(Dog* dog, const model::GameSession* session);

    Player* FindByDogIdAndMapId(PlayerDogId dog_id, const model::Map::Id& map_id);

private:
    std::vector<Player> players_;

    // Используем std::pair<dog_id, map_id> как ключ для быстрого поиска игрока
    using DogMapKey = std::pair<uint32_t, model::Map::Id>;

    struct DogMapKeyHasher {
        std::size_t operator()(const DogMapKey& key) const {
            std::size_t h1 = std::hash<uint32_t>()(key.first);
            std::size_t h2 = util::TaggedHasher<model::Map::Id>()(key.second);
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
    std::shared_ptr<Player> FindPlayerByToken(const Token& token);

    // Метод для добавления игрока и генерации токена
    Token AddPlayer(std::shared_ptr<Player> player);

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
    Token GenerateToken();
};

bool IsValidToken(const app::Token& token);

struct JoinGameResult {
   Token token{""};
   PlayerDogId player_id{0};
};

class JoinGameUseCase {
public:
    JoinGameUseCase(model::Game& game, PlayerTokens& player_tokens, Players& players);

    JoinGameResult JoinGame(const model::Map::Id& map_id, const std::string& name);

private:
    model::Game& game_model_;
    PlayerTokens& player_tokens_;
    Players& players_;
};

class ListPlayersUseCase {
public:
    using PlayersList = std::optional<std::vector<app::Dog>>;

    explicit ListPlayersUseCase(PlayerTokens& player_tokens);

    PlayersList ListPlayers(const Token& token);
private:
    PlayerTokens& player_tokens_;
};

class GameStateUseCase {
public:
private:
};

class Application {
public:
    using PlayersList = std::optional<std::vector<app::Dog>>;

    explicit Application(model::Game& model_game);

    JoinGameResult JoinGame(const model::Map::Id& map_id, const std::string& user_name);

    PlayersList ListPlayers(const Token& token);

    void GameState() {

    }

private:
    model::Game& game_model_;
    Players players_;
    PlayerTokens player_tokens_;

    std::shared_ptr<Player> FindPlayerByToken(const Token& token);
};

} // namespace app
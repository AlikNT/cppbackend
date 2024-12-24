#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <boost/signals2.hpp>

#include "postgres.h"
#include "tagged.h"
#include "model.h"
#include "json_loader.h"

namespace sig = boost::signals2;

namespace app {

using DogsList = std::optional<std::map<unsigned, std::shared_ptr<Dog>>>;
using DogId = uint32_t;

namespace detail {
struct TokenTag {
};
} // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

class DogTokens {
public:
    using TokenHasher = util::TaggedHasher<Token>;
    using TokenToDog = std::unordered_map<Token, std::shared_ptr<Dog>, TokenHasher>;
    using TokenToSession = std::unordered_map<Token, std::shared_ptr<model::GameSession>, TokenHasher>;

    std::shared_ptr<Dog> FindDogByToken(const Token &token) const;
    std::shared_ptr<model::GameSession> FindSessionByToken(const Token &token) const;
    Token AddDog(std::shared_ptr<Dog> dog_ptr, std::shared_ptr<model::GameSession> session_ptr);
    TokenToDog GetTokensToDog() const;
    TokenToSession GetTokensToSession() const;
    void SetTokenToDog(TokenToDog token_to_dog);
    void SetTokenToSession(TokenToSession token_to_session);
    bool DeleteDogToken(const std::shared_ptr<Dog> &dog_ptr) noexcept;


private:

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

    TokenToDog token_to_dog_;
    TokenToSession token_to_session_;

    // Метод для генерации уникального токена
    Token GenerateToken();
};

bool IsValidToken(const app::Token &token);

struct JoinGameResult {
    Token token{""};
    DogId dog_id{0};
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
    JoinGameUseCase(model::Game &game, DogTokens &dog_tokens);
    JoinGameResult JoinGame(const model::Map::Id &map_id, const std::string &name);

private:
    model::Game &game_model_;
    DogTokens &dog_tokens_;
};

class ListDogsUseCase {
public:
    explicit ListDogsUseCase(DogTokens &dog_tokens);
    [[nodiscard]] DogsList ListDogs(const Token &token) const;

private:
    DogTokens &dog_tokens_;
};

class GameStateUseCase {
public:
    explicit GameStateUseCase(DogTokens &dog_tokens);
    json::object GameState(const Token &token);

private:
    DogTokens &dog_tokens_;
};

class MovePlayersUseCase {
public:
    explicit MovePlayersUseCase(DogTokens &dog_tokens);
    MovePlayersResult MovePlayers(const Token &token, std::string_view move);

private:
    DogTokens &dog_tokens_;
};

class TickUseCase {
public:
    explicit TickUseCase(model::Game &game_model, std::chrono::milliseconds time_delta);
    void Tick();

private:
    model::Game &game_model_;
    std::chrono::milliseconds time_delta_;
};

class SaveRetiredPlayerUseCase {
public:
    SaveRetiredPlayerUseCase(postgres::Database &db, const std::shared_ptr<model::GameSession>& session_ptr, DogTokens &player_tokens);
    void Save(std::map<unsigned, std::shared_ptr<Dog>>::const_iterator &dog_it) const;

private:
    postgres::Database &db_;
    const std::shared_ptr<model::GameSession> session_ptr_;
    DogTokens &player_tokens_;
};

class TableOfRecordsUseCase {
public:
    TableOfRecordsUseCase(postgres::Database &db, int start, int max_items);
    [[nodiscard]] json::array GetTableOfRecords() const;

private:
    postgres::Database &db_;
    int start_;
    int max_items_;
};

class Application {
public:
    using milliseconds = std::chrono::milliseconds;
    using TickSignal = sig::signal<void(milliseconds delta)>;
    using TableOfRecords = std::vector<postgres::PlayerRecordResult>;

    explicit Application(model::Game &model_game);

    void SetDatabase(std::shared_ptr<postgres::Database> database_ptr);
    json::object GetMapsById(const model::Map::Id &map_id) const;
    JoinGameResult JoinGame(const model::Map::Id &map_id, const std::string &user_name);
    DogsList ListPlayers(const Token &token);
    json::object GameState(const Token &token);
    MovePlayersResult MovePlayers(const Token &token, std::string_view move);
    // Обработчик сигнала tick и возвращаем объект connection для управления,
    // при помощи которого можно отписаться от сигнала
    [[nodiscard]] sig::connection DoOnTick(const TickSignal::slot_type& handler);
    void Tick(std::chrono::milliseconds time_delta);
    model::Game& GetGame() const;
    void SetSessions (model::Game::Sessions sessions) const;
    void SetTokenToDog(DogTokens::TokenToDog token_to_dog);
    void SetTokenToSession(DogTokens::TokenToSession token_to_session);
    const DogTokens& GetDogTokens() const;
    void OnRetiredDog(std::map<unsigned, std::shared_ptr<Dog>>::const_iterator& dog_it, const std::shared_ptr<model::GameSession> &session_ptr);
    void SaveRetiredPlayers(std::map<unsigned, std::shared_ptr<Dog>>::const_iterator &dog_it, const std::shared_ptr<model::GameSession> &session_ptr);
    json::array GetTableOfRecords(int start, int max_items) const;

private:
    model::Game &game_model_;
    DogTokens dog_tokens_;
    std::shared_ptr<postgres::Database> db_;
    TickSignal tick_signal_;
    sig::scoped_connection dog_retired_connection_;
};

} // namespace app

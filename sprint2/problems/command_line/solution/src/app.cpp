#include "app.h"

#include <utility>

namespace app {



Player::Player(std::shared_ptr<Dog> dog, std::shared_ptr<model::GameSession> session)
        : dog_(std::move(dog))
        , session_(std::move(session)) {}

std::shared_ptr<Dog> Player::GetDog() {
    return dog_;
}

PlayerDogId Player::GetPlayerId() {
    return dog_->GetId();
}

std::shared_ptr<model::GameSession> Player::GetSession() {
    return session_;
}

app::DogSpeed Player::GetDogSpeed() const {
   return dog_->GetDogSpeed();
}

std::shared_ptr<Player> Players::Add(const std::shared_ptr<Dog>& dog, const std::shared_ptr<model::GameSession>& session) {
    players_.emplace_back(std::make_shared<Player>(dog, session));
    auto& player = players_.back();

    // Добавляем в карту быстрый доступ по комбинации dog_id и map_id
    if (session && session->GetMap()) {
        dog_map_to_player_[{dog->GetId(), session->GetMap()->GetId()}] = player;
    }
    return player;
}

std::shared_ptr<Player> Players::FindByDogIdAndMapId(PlayerDogId dog_id, const model::Map::Id &map_id) {
    auto it = dog_map_to_player_.find({dog_id, map_id});
    if (it != dog_map_to_player_.end()) {
        return it->second;
    }
    return nullptr;  // Возвращаем nullptr, если игрок не найден
}

std::shared_ptr<Player> PlayerTokens::FindPlayerByToken(const Token &token) {
    auto it = token_to_player_.find(token);
    if (it != token_to_player_.end()) {
        return it->second;
    }
    return nullptr;
}

Token PlayerTokens::AddPlayer(std::shared_ptr<Player> player) {
    Token token = GenerateToken();
    token_to_player_[token] = std::move(player);
    return token;
}

Token PlayerTokens::GenerateToken() {
    std::ostringstream token_stream;

    // Генерируем два 64-разрядных случайных числа и переводим их в hex
    uint64_t part1 = generator1_();
    uint64_t part2 = generator2_();

    token_stream << std::hex << std::setw(16) << std::setfill('0') << part1
                 << std::setw(16) << std::setfill('0') << part2;

    // Создаем и возвращаем Token на основе hex-строки
    return Token(token_stream.str());
}

JoinGameUseCase::JoinGameUseCase(model::Game &game, PlayerTokens &player_tokens, Players &players)
        : game_model_(game),
          player_tokens_(player_tokens),
          players_(players) {}

JoinGameResult JoinGameUseCase::JoinGame(const model::Map::Id &map_id, const std::string &name) {
    // Поиск карты
    const model::Map* map = game_model_.FindMap(map_id);
    assert(map);

    auto session = game_model_.FindSession(map_id);
    if (!session) {
        session = game_model_.AddSession(map_id);
    }

    // Создание собаки и добавление в сессию
    auto dog = session->AddDog(name);

    // Добавление игрока
    auto player = players_.Add(dog, session);

    // Генерация токена для игрока и его добавление в систему токенов
    return {player_tokens_.AddPlayer(player), dog->GetId()};
}

Application::Application(model::Game &model_game)
        : game_model_(model_game),
          players_(),
          player_tokens_() {}

JoinGameResult Application::JoinGame(const model::Map::Id &map_id, const std::string &user_name) {
    JoinGameUseCase join_game(game_model_, player_tokens_, players_);
    return join_game.JoinGame(map_id, user_name);
}

std::shared_ptr<Player> Application::FindPlayerByToken(const Token &token) {
    return player_tokens_.FindPlayerByToken(token);
}

PlayersList Application::ListPlayers(const Token& token) {
    ListPlayersUseCase list_players(player_tokens_);
    return list_players.ListPlayers(token);
}

std::optional<GameStateResult> Application::GameState(const Token &token) {
    GameStateUseCase game_state(player_tokens_);
    return game_state.GameState(token);
}

MovePlayersResult Application::MovePlayers(const Token &token, std::string_view move) {
    MovePlayersUseCase move_players(player_tokens_);
    return move_players.MovePlayers(token, move);
}

void Application::Tick(std::chrono::milliseconds time_delta) {
    TickUseCase tick(game_model_, time_delta);
    tick.Update();
}

PlayersList ListPlayersUseCase::ListPlayers(const Token &token) {
    // Ищем игрока по токену
    auto player = player_tokens_.FindPlayerByToken(token);
    if (!player) {
        return std::nullopt;
    }

    // Получаем список игроков в сессии
    auto session = player->GetSession();
    auto players = session->GetDogs();

    return players;
}

ListPlayersUseCase::ListPlayersUseCase(PlayerTokens &player_tokens)
    : player_tokens_(player_tokens) {}

bool IsValidToken(const Token &token) {
    constexpr int TOKEN_SIZE = 32;
    const std::string& token_str = *token;
    return token_str.size() == TOKEN_SIZE && std::all_of(token_str.begin(), token_str.end(), ::isxdigit);
}

GameStateUseCase::GameStateUseCase(PlayerTokens &player_tokens)
    : player_tokens_(player_tokens) {}

std::optional<GameStateResult> GameStateUseCase::GameState(const Token &token) {
    // Ищем игрока по токену
    auto player = player_tokens_.FindPlayerByToken(token);
    if (!player) {
        return std::nullopt;
    }

    // Получаем список игроков в сессии
    auto session = player->GetSession();
    auto dogs = session->GetDogs();

    return dogs;
}

MovePlayersUseCase::MovePlayersUseCase(PlayerTokens &player_tokens)
    : player_tokens_(player_tokens) {}

MovePlayersResult MovePlayersUseCase::MovePlayers(const Token &token, std::string_view move) {
    // Ищем игрока по токену
    auto player = player_tokens_.FindPlayerByToken(token);
    if (!player) {
        return MovePlayersResult::UNKNOWN_TOKEN;
    }
    // Update the player's speed based on the move value
    double speed = player->GetSession()->GetMap()->GetDogSpeed();
    if (move == "L") {
        player->GetDog()->SetDogSpeed({-speed, 0});
        player->GetDog()->SetDogDirection(app::Direction::WEST);
    } else if (move == "R") {
        player->GetDog()->SetDogSpeed({speed, 0});
        player->GetDog()->SetDogDirection(app::Direction::EAST);
    } else if (move == "U") {
        player->GetDog()->SetDogSpeed({0, -speed});
        player->GetDog()->SetDogDirection(app::Direction::NORTH);
    } else if (move == "D") {
        player->GetDog()->SetDogSpeed({0, speed});
        player->GetDog()->SetDogDirection(app::Direction::SOUTH);
    } else if (move.empty()) {
        player->GetDog()->SetDogSpeed({0, 0});
    } else {
        return MovePlayersResult::UNKNOWN_MOVE;
    }
    return MovePlayersResult::OK;
}

TickUseCase::TickUseCase(model::Game &game_model, std::chrono::milliseconds time_delta)
    : game_model_(game_model)
    , time_delta_(time_delta) {}

void TickUseCase::Update() {
    game_model_.Update(time_delta_);
}

} // namespace app
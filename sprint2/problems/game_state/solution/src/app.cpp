#include "app.h"

namespace app {



Player::Player(Dog *dog, const model::GameSession *session)
        : dog_(dog)
        , session_(session) {}

Dog *Player::GetDog() {
    return dog_;
}

PlayerDogId Player::GetPlayerId() {
    return dog_->GetId();
}

const model::GameSession *Player::GetSession() {
    return session_;
}

app::DogSpeed Player::GetDogSpeed() const {
   return dog_->GetDogSpeed();
}

const Player &Players::Add(Dog *dog, const model::GameSession *session) {
    players_.emplace_back(dog, session);
    auto& player= players_.back();

    // Добавляем в карту быстрый доступ по комбинации dog_id и map_id
    if (session && session->GetMap()) {
        dog_map_to_player_[{dog->GetId(), session->GetMap()->GetId()}] = &player;
    }
    return player;
}

Player *Players::FindByDogIdAndMapId(PlayerDogId dog_id, const model::Map::Id &map_id) {
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

    model::GameSession* session = game_model_.FindSession(map_id);
    if (!session) {
        session = game_model_.AddSession(map_id);
    }

    // Создание собаки и добавление в сессию
    auto dog = session->AddDog(name);

    // Добавление игрока
    const Player& player = players_.Add(dog, session);

    // Генерация токена для игрока и его добавление в систему токенов
    return {player_tokens_.AddPlayer(std::make_shared<Player>(player)), dog->GetId()};
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
/*
    // Ищем игрока по токену
    auto player = player_tokens_.FindPlayerByToken(token);
    if (!player) {
        return std::nullopt;
    }

    // Получаем список игроков в сессии
    const model::GameSession* session = player->GetSession();
    const std::vector<Dog>& players = session->GetPlayers();

    return GameStateResult{players, player->GetDogSpeed()};
*/
}

PlayersList ListPlayersUseCase::ListPlayers(const Token &token) {
    // Ищем игрока по токену
    auto player = player_tokens_.FindPlayerByToken(token);
    if (!player) {
        return std::nullopt;
    }

    // Получаем список игроков в сессии
    auto session = player->GetSession();
    auto players = session->GetPlayers();

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
    : player_tokens_(player_tokens) {
}

std::optional<GameStateResult> GameStateUseCase::GameState(const Token &token) {
    // Ищем игрока по токену
    auto player = player_tokens_.FindPlayerByToken(token);
    if (!player) {
        return std::nullopt;
    }

    // Получаем список игроков в сессии
    const model::GameSession* session = player->GetSession();
    const std::vector<Dog>& players = session->GetPlayers();

    return GameStateResult{players, player->GetDogSpeed()};
}
} // namespace app
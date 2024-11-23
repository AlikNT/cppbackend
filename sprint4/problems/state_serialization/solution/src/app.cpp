#include "app.h"

#include <utility>

namespace app {
using namespace std::literals;

Player::Player(std::shared_ptr<Dog> dog, std::shared_ptr<model::GameSession> session)
        : dog_(std::move(dog))
        , session_(std::move(session)) {}

std::shared_ptr<Dog> Player::GetDogPtr() const {
    return dog_;
}

PlayerDogId Player::GetPlayerId() const {
    return dog_->GetId();
}

std::shared_ptr<model::GameSession> Player::GetSession() const {
    return session_;
}

app::DogSpeed Player::GetDogSpeed() const {
   return dog_->GetDogSpeed();
}

std::shared_ptr<Player> Players::AddPlayer(const std::shared_ptr<Dog>& dog, const std::shared_ptr<model::GameSession>& session) {
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

Players::PlayersContainer Players::GetPlayers() const {
    return players_;
}

Players::DogMapToPlayer Players::GetDogMapToPlayer() const {
    return dog_map_to_player_;
}

void Players::SetPlayer(const std::shared_ptr<Player>& player_ptr) noexcept {
    players_.push_back(player_ptr);
}

void Players::SetDogMapToPlayer(DogMapToPlayer dog_map_to_player) noexcept {
    dog_map_to_player_ = std::move(dog_map_to_player);
}

std::shared_ptr<Player> PlayerTokens::FindPlayerByToken(const Token &token) const {
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

PlayerTokens::TokenToPlayer PlayerTokens::GetTokens() const {
    return token_to_player_;
}

void PlayerTokens::SetTokens(TokenToPlayer token_to_player) {
    if (token_to_player.empty()) {
        throw std::invalid_argument("token_to_player is empty");
    }
    token_to_player_ = std::move(token_to_player);
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
    if (!map) {
        throw std::runtime_error("Map not found");
    }

    auto session = game_model_.FindSession(map_id);
    if (!session) {
        session = game_model_.AddSession(map_id);
    }

    // Создание собаки и добавление в сессию
    auto dog = session->AddDog(name);

    // Добавление игрока
    auto player = players_.AddPlayer(dog, session);

    // Генерация токена для игрока и его добавление в систему токенов
    return {player_tokens_.AddPlayer(player), dog->GetId()};
}

Application::Application(model::Game &model_game)
        : game_model_(model_game),
          players_(),
          player_tokens_() {}

json::object Application::GetMapsById(const model::Map::Id &map_id) const {
    GetMapByIdUseCase get_map_by_id(game_model_);
    return get_map_by_id.GetMapById(map_id);
}

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

json::object Application::GameState(const Token &token) {
    GameStateUseCase game_state(player_tokens_);
    return game_state.GameState(token);
}

MovePlayersResult Application::MovePlayers(const Token &token, std::string_view move) {
    MovePlayersUseCase move_players(player_tokens_);
    return move_players.MovePlayers(token, move);
}

sig::connection Application::DoOnTick(const TickSignal::slot_type &handler) {
    return tick_signal_.connect(handler);
}

void Application::Tick(std::chrono::milliseconds time_delta) {
    TickUseCase tick(game_model_, time_delta);
    tick.Update();
    tick_signal_(time_delta);
}

model::Game & Application::GetGame() const {
    return game_model_;
}

const Players & Application::GetPlayers() const {
    return players_;
}

const PlayerTokens & Application::GetPlayerTokens() const {
    return player_tokens_;
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

GetMapByIdUseCase::GetMapByIdUseCase(model::Game &game): game_model_(game) {
}

json::object GetMapByIdUseCase::GetMapById(const model::Map::Id &map_id) const {
    const auto map = game_model_.FindMap(map_id);
    if (!map) {
        return {};
    }

    // Добавление map id
    json::object map_json;
    map_json[MAP_ID] = *(map->GetId());
    map_json["name"] = map->GetName();

    // Добавление дорог
    json::array roads_json;
    map_json["roads"] = AddRoads(map);

    // Добавление зданий
    json::array buildings_json;
    map_json["buildings"] = AddBuildings(map);

    // Добавление офисов
    json::array objects_json;
    map_json["offices"] = AddOffices(map);

    // Добавление типов трофеев
    const json::array loot_types_json = map->GetExtraData().GetLootTypes();
    map_json["lootTypes"] = loot_types_json;

    return map_json;
}

json::array GetMapByIdUseCase::AddRoads(const model::Map *map) {
    json::array roads_json;

    for (const auto &road: map->GetRoads()) {
        json::object road_json;
        road_json[ROAD_BEGIN_X0] = road.GetStart().x;
        road_json[ROAD_BEGIN_Y0] = road.GetStart().y;

        if (road.IsHorizontal()) {
            road_json[ROAD_END_X1] = road.GetEnd().x;
        } else {
            road_json[ROAD_END_Y1] = road.GetEnd().y;
        }

        roads_json.push_back(road_json);
    }
    return roads_json;
}

json::array GetMapByIdUseCase::AddBuildings(const model::Map *map) {
    json::array buildings_json;

    for (const auto &building: map->GetBuildings()) {
        const auto &bounds = building.GetBounds();
        buildings_json.push_back({
            {X, bounds.position.x},
            {Y, bounds.position.y},
            {BUILDING_WIDTH, bounds.size.width},
            {BUILDING_HEIGHT, bounds.size.height}
        });
    }
    return buildings_json;
}

json::array GetMapByIdUseCase::AddOffices(const model::Map *map) {
    json::array offices_json;

    for (const auto &office: map->GetOffices()) {
        offices_json.push_back({
            {OFFICE_ID, *office.GetId()},
            {X, office.GetPosition().x},
            {Y, office.GetPosition().y},
            {OFFICE_OFFSET_X, office.GetOffset().dx},
            {OFFICE_OFFSET_Y, office.GetOffset().dy}
        });
    }
    return offices_json;
}

GameStateUseCase::GameStateUseCase(PlayerTokens &player_tokens)
    : player_tokens_(player_tokens) {}

json::array MakeLootsInBagJson(const model::GameSession& session, const Dog& dog) {
    json::array bag_json;
    for (const auto &loot_ptr : dog.GetLootsInBag()) {
        if (loot_ptr->GetLootStatus() != app::LootStatus::BAG) {
            continue;
        }
        json::object loot_in_bag_json;
        loot_in_bag_json["id"s] = session.GetIndexByLootPtr(loot_ptr);
        loot_in_bag_json["type"s] = loot_ptr->GetLootTypeId();
        bag_json.emplace_back(std::move(loot_in_bag_json));
    }
    return bag_json;
}

json::object MakePlayersJson(const model::GameSession& session, const std::vector<std::shared_ptr<Dog>>& dogs) {
    const std::unordered_map<app::Direction, std::string> dir{
        {app::Direction::NORTH, "U"s},
        {app::Direction::SOUTH, "D"s},
        {app::Direction::WEST, "L"s},
        {app::Direction::EAST, "R"s}
    };
    json::object players_json;
    for (const auto &dog : dogs) {
        json::object player_json;
        player_json["pos"s] = {dog->GetPosition().x, dog->GetPosition().y};
        player_json["speed"s] = {dog->GetDogSpeed().sx, dog->GetDogSpeed().sy};
        player_json["dir"s] = dir.at(dog->GetDirection());

        json::array bag_json = MakeLootsInBagJson(session, *dog);
        player_json["bag"s] = std::move(bag_json);
        player_json["score"s] = dog->GetScore();
        players_json[std::to_string(dog->GetId())] = std::move(player_json);
    }
    return players_json;
}

json::object MakeLostObjectsJson(const model::GameSession& session) {
    json::object lost_objects_json;
    const auto &loots = session.GetLoots();
    for (size_t i = 0; i < loots.size(); ++i) {
        if (loots[i]->GetLootStatus() != app::LootStatus::ROAD) {
            continue;
        }
        json::object loot_json;
        loot_json["type"s] = loots[i]->GetLootTypeId();
        loot_json["pos"s] = {loots[i]->GetLootPosition().x, loots[i]->GetLootPosition().y};
        lost_objects_json[std::to_string(i)] = std::move(loot_json);
    }
    return lost_objects_json;
}

json::object GameStateUseCase::GameState(const Token &token) {
    // Ищем игрока по токену
    const auto player = player_tokens_.FindPlayerByToken(token);
    if (!player) {
        return {};
    }

    json::object json_body;

    // Получаем список игроков в сессии
    const auto session = player->GetSession();
    const auto dogs = session->GetDogs();

    // Формируем json списка игроков
    json::object players_json = MakePlayersJson(*session, dogs);
    json_body["players"s] = std::move(players_json);

    // Формируем json потерянных объектов
    json::object lost_objects_json = MakeLostObjectsJson(*session);
    json_body["lostObjects"s] = std::move(lost_objects_json);

    return json_body;
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
        player->GetDogPtr()->SetDogSpeed({-speed, 0});
        player->GetDogPtr()->SetDogDirection(app::Direction::WEST);
    } else if (move == "R") {
        player->GetDogPtr()->SetDogSpeed({speed, 0});
        player->GetDogPtr()->SetDogDirection(app::Direction::EAST);
    } else if (move == "U") {
        player->GetDogPtr()->SetDogSpeed({0, -speed});
        player->GetDogPtr()->SetDogDirection(app::Direction::NORTH);
    } else if (move == "D") {
        player->GetDogPtr()->SetDogSpeed({0, speed});
        player->GetDogPtr()->SetDogDirection(app::Direction::SOUTH);
    } else if (move.empty()) {
        player->GetDogPtr()->SetDogSpeed({0, 0});
    } else {
        return MovePlayersResult::UNKNOWN_MOVE;
    }
    return MovePlayersResult::OK;
}

TickUseCase::TickUseCase(model::Game &game_model, std::chrono::milliseconds time_delta)
    : game_model_(game_model)
    , time_delta_(time_delta) {}

void TickUseCase::Update() {
    game_model_.Tick(time_delta_);
}

} // namespace app
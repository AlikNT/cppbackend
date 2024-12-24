#include "app.h"

#include <iostream>
#include <utility>

namespace app {
using namespace std::literals;

std::shared_ptr<Dog> DogTokens::FindDogByToken(const Token &token) const {
    if (const auto it = token_to_dog_.find(token); it != token_to_dog_.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<model::GameSession> DogTokens::FindSessionByToken(const Token &token) const {
    if (const auto it = token_to_session_.find(token); it != token_to_session_.end()) {
        return it->second;
    }
    return nullptr;
}

Token DogTokens::AddDog(std::shared_ptr<Dog> dog_ptr, std::shared_ptr<model::GameSession> session_ptr) {
    Token token = GenerateToken();
    token_to_dog_[token] = std::move(dog_ptr);
    token_to_session_[token] = std::move(session_ptr);
    return token;
}

DogTokens::TokenToDog DogTokens::GetTokensToDog() const {
    return token_to_dog_;
}

DogTokens::TokenToSession DogTokens::GetTokensToSession() const {
    return token_to_session_;
}

void DogTokens::SetTokenToDog(TokenToDog token_to_dog) {
    token_to_dog_ = std::move(token_to_dog);
}

void DogTokens::SetTokenToSession(TokenToSession token_to_session) {
    token_to_session_ = std::move(token_to_session);
}

bool DogTokens::DeleteDogToken(const std::shared_ptr<Dog> &dog_ptr) noexcept {
    if (!dog_ptr) {
        return false;
    }
    for (auto it = token_to_dog_.begin(); it != token_to_dog_.end(); ++it) {
        if (it->second == dog_ptr) {
            token_to_session_.erase(it->first);
            token_to_dog_.erase(it);
            return true;
        }
    }
    return false;
}

Token DogTokens::GenerateToken() {
    std::ostringstream token_stream;

    // Генерируем два 64-разрядных случайных числа и переводим их в hex
    uint64_t part1 = generator1_();
    uint64_t part2 = generator2_();

    token_stream << std::hex << std::setw(16) << std::setfill('0') << part1
                 << std::setw(16) << std::setfill('0') << part2;

    // Создаем и возвращаем Token на основе hex-строки
    return Token(token_stream.str());
}

JoinGameUseCase::JoinGameUseCase(model::Game &game, DogTokens &dog_tokens)
        : game_model_(game),
          dog_tokens_(dog_tokens) {}

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
    const auto dog = session->AddDog(name);

    // Генерация токена для игрока и его добавление в систему токенов
    return {dog_tokens_.AddDog(dog, session), dog->GetId()};
}

Application::Application(model::Game &model_game)
        : game_model_(model_game)
        , dog_tokens_()
        , db_(nullptr) {
    game_model_.SubscribeDogRetirementTime([this](std::map<unsigned, std::shared_ptr<Dog>>::const_iterator& dog_it, const std::shared_ptr<model::GameSession> &session_ptr) {
        OnRetiredDog(dog_it, session_ptr);
    });
}

void Application::SetDatabase(std::shared_ptr<postgres::Database> database_ptr) {
    if (!database_ptr) {
        throw std::invalid_argument("Database ptr is null");
    }
    db_ = std::move(database_ptr);
}

json::object Application::GetMapsById(const model::Map::Id &map_id) const {
    GetMapByIdUseCase get_map_by_id(game_model_);
    return get_map_by_id.GetMapById(map_id);
}

JoinGameResult Application::JoinGame(const model::Map::Id &map_id, const std::string &user_name) {
    JoinGameUseCase join_game(game_model_, dog_tokens_);
    return join_game.JoinGame(map_id, user_name);
}

DogsList Application::ListPlayers(const Token& token) {
    const ListDogsUseCase list_players(dog_tokens_);
    return list_players.ListDogs(token);
}

json::object Application::GameState(const Token &token) {
    GameStateUseCase game_state(dog_tokens_);
    return game_state.GameState(token);
}

MovePlayersResult Application::MovePlayers(const Token &token, std::string_view move) {
    MovePlayersUseCase move_players(dog_tokens_);
    return move_players.MovePlayers(token, move);
}

sig::connection Application::DoOnTick(const TickSignal::slot_type &handler) {
    return tick_signal_.connect(handler);
}

void Application::Tick(std::chrono::milliseconds time_delta) {
    TickUseCase tick(game_model_, time_delta);
    tick.Tick();
    tick_signal_(time_delta);
}

model::Game & Application::GetGame() const {
    return game_model_;
}

void Application::SetSessions(model::Game::Sessions sessions) const {
    game_model_.SetSessions(std::move(sessions));
}

void Application::SetTokenToDog(DogTokens::TokenToDog token_to_dog) {
    dog_tokens_.SetTokenToDog(std::move(token_to_dog));
}

void Application::SetTokenToSession(DogTokens::TokenToSession token_to_session) {
    dog_tokens_.SetTokenToSession(std::move(token_to_session));
}

const DogTokens & Application::GetDogTokens() const {
    return dog_tokens_;
}

void Application::OnRetiredDog(std::map<unsigned, std::shared_ptr<Dog>>::const_iterator& dog_it, const std::shared_ptr<model::GameSession> &session_ptr) {
    SaveRetiredPlayers(dog_it, session_ptr);
}

void Application::SaveRetiredPlayers(std::map<unsigned, std::shared_ptr<Dog>>::const_iterator &dog_it, const std::shared_ptr<model::GameSession> &session_ptr) {
    const auto retired_players_use_case = SaveRetiredPlayerUseCase(*db_, session_ptr, dog_tokens_);
    retired_players_use_case.Save(dog_it);
}

json::array Application::GetTableOfRecords(const int start, const int max_items) const {
    const auto table_of_records_use_case = TableOfRecordsUseCase(*db_, start, max_items);
    return table_of_records_use_case.GetTableOfRecords();
}

DogsList ListDogsUseCase::ListDogs(const Token &token) const {
    if (const auto dog = dog_tokens_.FindDogByToken(token); !dog) {
        return std::nullopt;
    }
    const auto session = dog_tokens_.FindSessionByToken(token);
    auto dogs = session->GetDogs();

    return dogs;
}

ListDogsUseCase::ListDogsUseCase(DogTokens &dog_tokens)
    : dog_tokens_(dog_tokens) {}

bool IsValidToken(const Token &token) {
    constexpr int TOKEN_SIZE = 32;
    const std::string& token_str = *token;
    return token_str.size() == TOKEN_SIZE && std::all_of(token_str.begin(), token_str.end(), ::isxdigit);
}

GetMapByIdUseCase::GetMapByIdUseCase(model::Game &game)
    : game_model_(game) {
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

GameStateUseCase::GameStateUseCase(DogTokens &dog_tokens)
    : dog_tokens_(dog_tokens) {}

json::array MakeLootsInBagJson(const model::GameSession& session, const Dog& dog) {
    json::array bag_json;
    for (const auto &loot_ptr : dog.GetLootsInBag()) {
        if (loot_ptr == nullptr) {
            throw std::runtime_error("Loot pointer is nullptr");
        }
        json::object loot_in_bag_json;
        loot_in_bag_json["id"s] = loot_ptr->GetLootId();
        loot_in_bag_json["type"s] = loot_ptr->GetLootTypeId();
        bag_json.emplace_back(std::move(loot_in_bag_json));
    }
    return bag_json;
}

json::object MakePlayersJson(const model::GameSession& session, const std::map<unsigned, std::shared_ptr<Dog>> &dogs) {
    const std::unordered_map<app::Direction, std::string> dir{
        {app::Direction::NORTH, "U"s},
        {app::Direction::SOUTH, "D"s},
        {app::Direction::WEST, "L"s},
        {app::Direction::EAST, "R"s}
    };
    json::object players_json;
    for (const auto &[dog_id, dog_ptr] : dogs) {
        json::object player_json;
        player_json["pos"s] = {dog_ptr->GetPosition().x, dog_ptr->GetPosition().y};
        player_json["speed"s] = {dog_ptr->GetDogSpeed().sx, dog_ptr->GetDogSpeed().sy};
        player_json["dir"s] = dir.at(dog_ptr->GetDirection());

        json::array bag_json = MakeLootsInBagJson(session, *dog_ptr);
        player_json["bag"s] = std::move(bag_json);
        player_json["score"s] = dog_ptr->GetScore();
        players_json[std::to_string(dog_ptr->GetId())] = std::move(player_json);
    }
    return players_json;
}

json::object MakeLostObjectsJson(const model::GameSession& session) {
    json::object lost_objects_json;
    for (const auto &loots = session.GetLoots(); const auto &[loot_id, loot_ptr] : loots) {
        json::object loot_json;
        loot_json["type"s] = loot_ptr->GetLootTypeId();
        loot_json["pos"s] = {loot_ptr->GetLootPosition().x, loot_ptr->GetLootPosition().y};
        lost_objects_json[std::to_string(loot_id)] = std::move(loot_json);
    }
    return lost_objects_json;
}

json::object GameStateUseCase::GameState(const Token &token) {
    if (const auto dog_ptr = dog_tokens_.FindDogByToken(token); !dog_ptr) {
        return {};
    }

    json::object json_body;

    // Получаем список игроков в сессии
    const auto session = dog_tokens_.FindSessionByToken(token);
    const auto dogs = session->GetDogs();

    // Формируем json списка игроков
    json::object players_json = MakePlayersJson(*session, dogs);
    json_body["players"s] = std::move(players_json);

    // Формируем json потерянных объектов
    json::object lost_objects_json = MakeLostObjectsJson(*session);
    json_body["lostObjects"s] = std::move(lost_objects_json);

    return json_body;
}

MovePlayersUseCase::MovePlayersUseCase(DogTokens &dog_tokens)
    : dog_tokens_(dog_tokens) {}

MovePlayersResult MovePlayersUseCase::MovePlayers(const Token &token, std::string_view move) {
    auto dog_ptr = dog_tokens_.FindDogByToken(token);
    if (!dog_ptr) {
        return MovePlayersResult::UNKNOWN_TOKEN;
    }
    // Update the player's speed based on the move value
    double speed = dog_tokens_.FindSessionByToken(token)->GetMap()->GetDogSpeed();
    if (move == "L") {
        dog_ptr->SetDogSpeed({-speed, 0});
        dog_ptr->SetDogDirection(app::Direction::WEST);
    } else if (move == "R") {
        dog_ptr->SetDogSpeed({speed, 0});
        dog_ptr->SetDogDirection(app::Direction::EAST);
    } else if (move == "U") {
        dog_ptr->SetDogSpeed({0, -speed});
        dog_ptr->SetDogDirection(app::Direction::NORTH);
    } else if (move == "D") {
        dog_ptr->SetDogSpeed({0, speed});
        dog_ptr->SetDogDirection(app::Direction::SOUTH);
    } else if (move.empty()) {
        dog_ptr->SetDogSpeed({0, 0});
    } else {
        return MovePlayersResult::UNKNOWN_MOVE;
    }
    return MovePlayersResult::OK;
}

TickUseCase::TickUseCase(model::Game &game_model, std::chrono::milliseconds time_delta)
    : game_model_(game_model)
    , time_delta_(time_delta) {}

void TickUseCase::Tick() {
    game_model_.Tick(time_delta_);
}

SaveRetiredPlayerUseCase::SaveRetiredPlayerUseCase(postgres::Database &db, const std::shared_ptr<model::GameSession>& session_ptr, DogTokens &player_tokens)
                                                    : db_(db)
                                                    , session_ptr_(session_ptr)
                                                    , player_tokens_(player_tokens) {
}

void SaveRetiredPlayerUseCase::Save(std::map<unsigned, std::shared_ptr<Dog>>::const_iterator &dog_it) const {
    const auto conn = db_.GetTransaction();
    auto transaction = pqxx::work(*conn);
    try {
        const postgres::RetiredPlayersRepository player_repository{transaction};
        const auto dog_ptr = dog_it->second;
        player_repository.Save(dog_ptr->GetName(), static_cast<int>(dog_ptr->GetScore()), static_cast<int>(dog_ptr->GetPlayTime().count()));
        player_tokens_.DeleteDogToken(dog_ptr);
        if (!session_ptr_) {
            throw std::runtime_error("GameSession is nullptr");
        }
        session_ptr_->DeleteDog(dog_it);
    } catch (const std::exception &) {
        transaction.abort();
        throw;
        // return;
    }
    transaction.commit();
}

TableOfRecordsUseCase::TableOfRecordsUseCase(postgres::Database &db, const int start, const int max_items): db_(db)
    , start_(start)
    , max_items_(max_items) {
}

json::array TableOfRecordsUseCase::GetTableOfRecords() const {
    const auto conn = db_.GetTransaction();
    pqxx::transaction transaction(*conn);
    const postgres::RetiredPlayersRepository player_repository{transaction};
    auto player_records = player_repository.Load(start_, max_items_);
    json::array player_records_json;
    for (const auto &record : player_records) {
        player_records_json.push_back({
            {"name"s, record.name},
            {"score"s, record.score},
            {"playTime", record.play_time},
        });
    }
    return player_records_json;
}

} // namespace app
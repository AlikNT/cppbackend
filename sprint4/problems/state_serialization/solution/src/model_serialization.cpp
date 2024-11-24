#include "model_serialization.h"

namespace serialization {
LootRepr::LootRepr(const app::Loot &loot): loot_type_id_(loot.GetLootTypeId()),
                                                          loot_position_(loot.GetLootPosition()),
                                                          loot_status_(loot.GetLootStatus()) {}

app::Loot LootRepr::Restore() const {
    app::Loot loot(loot_type_id_, loot_position_);
    loot.SetLootStatus(loot_status_);
    return loot;
}

DogRepr::DogRepr(const app::Dog &dog): id_(dog.GetId())
                                       , name_(dog.GetName())
                                       , pos_(dog.GetPosition())
                                       , speed_(dog.GetSpeed())
                                       , direction_(dog.GetDirection())
                                       , score_(dog.GetScore())
                                       , bag_content_(dog.GetLootsInBag()) {
}

app::Dog DogRepr::Restore() const {
    app::Dog dog{name_, id_};
    dog.SetDogPosition(pos_);
    dog.SetDogSpeed(speed_);
    dog.SetDogDirection(direction_);
    dog.AddScoreValue(score_);
    for (const auto& item : bag_content_) {
        dog.AddLoot(item);
    }
    return dog;
}

GameSessionRepr::GameSessionRepr(const model::GameSession &session): dogs_(session.GetDogs())
                                                                     , map_id_(*session.GetMap()->GetId())
                                                                     , loots_(session.GetLoots())
                                                                     , dog_id_to_index_(session.GetDogIdToIndexMap())
                                                                     , loot_to_id_(session.GetLootToIdMap()) {
}

model::GameSession GameSessionRepr::Restore(const model::Game &game) const {
    const auto map_ptr = game.FindMap(model::Map::Id(map_id_));
    if (!map_ptr) {
        throw std::invalid_argument("Invalid map id");
    }
    model::GameSession session(map_ptr);
    for (const auto& dog : dogs_) {
        session.AddDogPtr(dog);
    }
    for (const auto& loot : loots_) {
        session.AddLoot(loot);
    }
    session.SetDogIdToIndexMap(dog_id_to_index_);
    session.SetLootToIdMap(loot_to_id_);
    return session;
}

GameSessionsRepr::GameSessionsRepr(const model::Game &game) {
    for (const auto& session : game.GetSessions()) {
        sessions_.emplace_back(*session);
    }
}

std::vector<std::shared_ptr<model::GameSession>> GameSessionsRepr::Restore(const model::Game &game) const {
    std::vector<std::shared_ptr<model::GameSession>> sessions;
    for (const auto& session_repr : sessions_) {
        sessions.push_back(std::make_shared<model::GameSession>(session_repr.Restore(game)));
    }
    return sessions;
}

PlayerRepr::PlayerRepr(const app::Player &player): dog_ptr_(player.GetDogPtr())
                                                   , session_repr_(GameSessionRepr(*player.GetSession())) {}

app::Player PlayerRepr::Restore(const model::Game &game) const {
    app::Player player(dog_ptr_, std::make_shared<model::GameSession>(session_repr_.Restore(game)));
    return player;
}

PlayersRepr::PlayersRepr(const app::Players &players) {
    for (const auto& player_ptr : players.GetPlayers()) {
        players_repr_.emplace_back(*player_ptr);
    }
}

app::Players PlayersRepr::Restore(const model::Game &game) const {
    app::Players players;

    // Восстанавливаем вектор игроков
    for (const auto& player_repr : players_repr_) {
        const auto player = std::make_shared<app::Player>(player_repr.Restore(game));
        players.AddPlayer(player->GetDogPtr(), player->GetSession());
    }

    return players;
}

TokenToPlayer::TokenToPlayer(const app::PlayerTokens &player_tokens) {
    for (const auto& [token, player_ptr] : player_tokens.GetTokens()) {
        token_to_player_.emplace_back(*token, PlayerRepr(*player_ptr));
    }
}

app::PlayerTokens::TokenToPlayer TokenToPlayer::Restore(const model::Game &game) const {
    app::PlayerTokens::TokenToPlayer token_to_player;
    for (const auto& [token_str, player_ptr] : token_to_player_) {
        auto x = app::Token(token_str);
        token_to_player[app::Token(token_str)] = std::make_shared<app::Player>(player_ptr.Restore(game));
    }
    return token_to_player;
}
} // namespace serialization
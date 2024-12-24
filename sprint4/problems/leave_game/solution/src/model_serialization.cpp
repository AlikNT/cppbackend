#include "model_serialization.h"

namespace serialization {
LootRepr::LootRepr(const app::Loot &loot)
    : loot_id_(loot.GetLootId())
    , loot_type_id_(loot.GetLootTypeId())
    , loot_position_(loot.GetLootPosition()) {
}

app::Loot LootRepr::Restore() const {
    const app::Loot loot(loot_id_, loot_type_id_, loot_position_);
    return loot;
}

DogRepr::DogRepr(const app::Dog &dog)
    : id_(dog.GetId())
    , name_(dog.GetName())
    , pos_(dog.GetPosition())
    , speed_(dog.GetSpeed())
    , direction_(dog.GetDirection())
    , score_(dog.GetScore())
    , down_time_(static_cast<int>(dog.GetDownTime().count()))
    , play_time_(static_cast<int>(dog.GetPlayTime().count()))
    , bag_content_(dog.GetLootsInBag()) {
}

app::Dog DogRepr::Restore() const {
    app::Dog dog{name_, id_};
    dog.SetDogPosition(pos_);
    dog.SetDogSpeed(speed_);
    dog.SetDogDirection(direction_);
    dog.AddScoreValue(score_);
    for (const auto& item : bag_content_) {
        dog.AddLootToBag(item);
    }
    dog.SetDownTime(std::chrono::milliseconds(down_time_));
    dog.SetPlayTime(std::chrono::milliseconds(play_time_));
    return dog;
}

GameSessionRepr::GameSessionRepr(const model::GameSession &session)
    : map_id_(*session.GetMap()->GetId()) {
    for (const auto& dog : session.GetDogs()) {
        dogs_.emplace_back(dog.second);
    }
    for (const auto& loot : session.GetLoots()) {
        loots_.emplace_back(loot.second);
    }
}

model::GameSession GameSessionRepr::Restore(const model::Game &game) const {
    const auto map_ptr = game.FindMap(model::Map::Id(map_id_));
    if (!map_ptr) {
        throw std::invalid_argument("Invalid map id");
    }
    model::GameSession session(map_ptr);
    for (const auto& dog_ptr : dogs_) {
        session.AddDog(dog_ptr);
    }
    for (const auto& loot_ptr : loots_) {
        session.AddLoot(loot_ptr);
    }
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

TokenToDogRepr::TokenToDogRepr(const app::DogTokens &dog_tokens) {
    for (const auto& [token, dog_ptr] : dog_tokens.GetTokensToDog()) {
        token_to_dog_.emplace_back(*token, dog_ptr);
    }
    for (const auto& [token, session_ptr] : dog_tokens.GetTokensToSession()) {
        token_to_sessions_.emplace_back(*token, *session_ptr);
    }
}

app::DogTokens::TokenToDog TokenToDogRepr::RestoreTokenToDog() const {
    app::DogTokens::TokenToDog dog_tokens;
    for (const auto& [token_str, dog_ptr] : token_to_dog_) {
        dog_tokens[app::Token(token_str)] = dog_ptr;
    }
    return dog_tokens;
}

app::DogTokens::TokenToSession TokenToDogRepr::RestoreTokenToSession(const model::Game& game) const {
    app::DogTokens::TokenToSession session_tokens;
    for (const auto& [token_str, session_repr] : token_to_sessions_) {
       session_tokens[app::Token(token_str)] = std::make_shared<model::GameSession>(session_repr.Restore(game));
    }
    return session_tokens;
}
} // namespace serialization
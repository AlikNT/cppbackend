#pragma once

#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <utility>

#include "app.h"
#include "model.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}


template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace app {

template <typename Archive>
void serialize(Archive& ar, Position& position, [[maybe_unused]] const unsigned version) {
    ar& position.x;
    ar& position.y;
}

template <typename Archive>
void serialize(Archive& ar, DogSpeed& dog_speed, [[maybe_unused]] const unsigned version) {
    ar& dog_speed.sx;
    ar& dog_speed.sy;
}

}

namespace serialization {

class LootRepr {
public:
    LootRepr() = default;

    explicit LootRepr(const app::Loot& loot);
    [[nodiscard]] app::Loot Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& loot_id_;
        ar& loot_type_id_;
        ar& loot_position_;
    }

private:
    unsigned loot_id_ = 0;
    unsigned loot_type_id_ = 0;
    app::LootPosition loot_position_;
};
} // namespace serialization

namespace app {
// Поддержка для shared_ptr<Loot>
template <typename Archive>
void serialize(Archive& ar, std::shared_ptr<Loot>& loot_ptr, [[maybe_unused]] const unsigned version) {
    if (Archive::is_saving::value) {
        if (loot_ptr) {
            serialization::LootRepr repr(*loot_ptr);
            ar & repr;
        } else {
            serialization::LootRepr empty_repr;
            ar & empty_repr;
        }
    } else {
        serialization::LootRepr repr;
        ar & repr;
        loot_ptr = std::make_shared<app::Loot>(repr.Restore());
    }
}

} // namespace app

namespace serialization {
// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    using BagContent = std::vector<std::shared_ptr<app::Loot>>;

    DogRepr() = default;

    explicit DogRepr(const app::Dog& dog);
    [[nodiscard]] app::Dog Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& name_;
        ar& pos_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
        ar& down_time_;
        ar& play_time_;
    }

private:
    app::DogId id_ = {0};
    std::string name_;
    app::DogPosition pos_;
    app::DogSpeed speed_;
    app::Direction direction_ = app::Direction::NORTH;
    unsigned score_ = 0;
    int down_time_{0};
    int play_time_{0};
    BagContent bag_content_;
};
} //namespace serialization

namespace app {

// Поддержка для shared_ptr<Dog>
template <typename Archive>
void serialize(Archive& ar, std::shared_ptr<Dog>& dog_ptr, [[maybe_unused]] const unsigned version) {
    if (Archive::is_saving::value) {
        if (dog_ptr) {
            serialization::DogRepr repr(*dog_ptr);
            ar & repr;
        } else {
            serialization::DogRepr empty_repr;
            ar & empty_repr;
        }
    } else {
        serialization::DogRepr repr;
        ar & repr;
        dog_ptr = std::make_shared<Dog>(repr.Restore());
    }
}
} // namespace app

namespace serialization {
class GameSessionRepr {
public:
    GameSessionRepr() = default;

    explicit GameSessionRepr(const model::GameSession& session);
    [[nodiscard]] model::GameSession Restore(const model::Game& game) const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & map_id_;
        ar & dogs_;
        ar & loots_;
    }

    [[nodiscard]] model::Map::Id GetMapId() const {
        return model::Map::Id(map_id_);
    }

private:
    std::string map_id_;
    std::vector<std::shared_ptr<app::Dog>> dogs_;
    std::vector<std::shared_ptr<app::Loot>> loots_;
};

// Обертка для коллекции GameSession
class GameSessionsRepr {
public:
    using SessionsRepr = std::vector<GameSessionRepr>;

    GameSessionsRepr() = default;

    explicit GameSessionsRepr(const model::Game& game);

    // Восстанавливает вектор GameSession из представлений
    [[nodiscard]] std::vector<std::shared_ptr<model::GameSession>> Restore(const model::Game& game) const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned int version) {
        ar & sessions_;
    }

private:
    SessionsRepr sessions_;
};

class TokenToDogRepr {
public:
    TokenToDogRepr() = default;

    explicit TokenToDogRepr(const app::DogTokens& dog_tokens);

    [[nodiscard]] app::DogTokens::TokenToDog RestoreTokenToDog() const;
    [[nodiscard]] app::DogTokens::TokenToSession RestoreTokenToSession(const model::Game& game) const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar & token_to_dog_;
        ar & token_to_sessions_;
    }

private:
    std::vector<std::pair<std::string, std::shared_ptr<app::Dog>>> token_to_dog_;
    std::vector<std::pair<std::string, GameSessionRepr>> token_to_sessions_;
};

}  // namespace serialization

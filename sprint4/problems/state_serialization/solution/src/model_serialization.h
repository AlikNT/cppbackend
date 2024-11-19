#pragma once
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "app.h"
#include "model.h"
#include "model_precode.h"

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
void serialize(Archive& ar, app::Position& position, [[maybe_unused]] const unsigned version) {
    ar& position.x;
    ar& position.y;
}

template <typename Archive>
void serialize(Archive& ar, app::DogSpeed& dog_speed, [[maybe_unused]] const unsigned version) {
    ar& dog_speed.sx;
    ar& dog_speed.sy;
}

template <typename Archive>
void serialize(Archive& ar, app::LootStatus& loot_status, [[maybe_unused]] const unsigned version) {
    ar& loot_status;
}

}

namespace model {

template <typename Archive>
void serialize(Archive& ar, FoundObject& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj.id);
    ar&(obj.type);
}

}  // namespace model

namespace serialization {
// LootRepr (LootRepresentation) - сериализованное представление класса Loot
class LootRepr {
public:
    LootRepr() = default;

    explicit LootRepr(const app::Loot& loot)
        : loot_type_id_(loot.GetLootTypeId()),
        loot_position_(loot.GetLootPosition()),
        loot_status_(loot.GetLootStatus()) {}

    [[nodiscard]] app::Loot Restore() const {
        app::Loot loot(loot_type_id_, loot_position_);
        loot.SetLootStatus(loot_status_);
        return loot;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& loot_type_id_;
        ar& loot_position_;
        ar& loot_status_;
    }

private:
    unsigned loot_type_id_ = 0;
    app::LootPosition loot_position_;
    app::LootStatus loot_status_ = app::LootStatus::ROAD;
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

    explicit DogRepr(const app::Dog& dog)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetPosition())
        , speed_(dog.GetSpeed())
        , direction_(dog.GetDirection())
        , score_(dog.GetScore())
        , bag_content_(dog.GetLootsInBag()) {
    }

    [[nodiscard]] app::Dog Restore() const {
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

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& name_;
        ar& pos_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
    }

private:
    app::PlayerDogId id_ = {0};
    std::string name_;
    app::DogPosition pos_;
    app::DogSpeed speed_;
    app::Direction direction_ = app::Direction::NORTH;
    unsigned score_ = 0;
    BagContent bag_content_;
};

/* Другие классы модели сериализуются и десериализуются похожим образом */

}  // namespace serialization

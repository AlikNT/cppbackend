#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "../src/model_precode.h"
#include "../src/model_serialization.h"

// using namespace model;
using namespace std::literals;
namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

}  // namespace

SCENARIO_METHOD(Fixture, "Point serialization") {
    GIVEN("A point") {
        const geom::Point2D p{10, 20};
        WHEN("point is serialized") {
            output_archive << p;

            THEN("it is equal to point after serialization") {
                InputArchive input_archive{strm};
                geom::Point2D restored_point;
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Dog Serialization") {
    GIVEN("a dog") {
        const auto test_dog = [] {
            app::Dog dog{ "Pluto"s, 42};
            dog.SetDogPosition({42.2, 12.5});
            dog.AddScoreValue(42);
            dog.AddLoot(std::make_shared<app::Loot>(app::Loot{2, {1.2, 3.4}}));
            dog.SetDogDirection(app::Direction::EAST);
            dog.SetDogSpeed({2.3, -1.2});
            return dog;
        }();

        WHEN("dog is serialized") {
            {
                serialization::DogRepr repr{test_dog};
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::DogRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore();

                CHECK(test_dog.GetId() == restored.GetId());
                CHECK(test_dog.GetName() == restored.GetName());
                CHECK(test_dog.GetPosition().x == restored.GetPosition().x);
                CHECK(test_dog.GetPosition().y == restored.GetPosition().y);
                CHECK(test_dog.GetSpeed().sx == restored.GetSpeed().sx);
                CHECK(test_dog.GetSpeed().sy == restored.GetSpeed().sy);
                CHECK(test_dog.GetLootsInBag().front()->GetLootTypeId() == restored.GetLootsInBag().front()->GetLootTypeId());
                CHECK(test_dog.GetLootsInBag().front()->GetLootPosition().x == restored.GetLootsInBag().front()->GetLootPosition().x);
                CHECK(test_dog.GetLootsInBag().front()->GetLootPosition().y == restored.GetLootsInBag().front()->GetLootPosition().y);
                CHECK(test_dog.GetLootsInBag().front()->GetLootStatus() == restored.GetLootsInBag().front()->GetLootStatus());
            }
        }
    }
    GIVEN("a game session") {
        model::Map map(model::Map::Id("TestMap"s), "Test map", 1.5, 3);
        const auto test_session = [&map] {
            model::GameSession session(&map);
            const auto loot_ptr = std::make_shared<app::Loot>(5, app::LootPosition{1.2, 3.4});
            session.AddLoot(loot_ptr);
            session.AddDog("TestDog1"s);
            session.AddDog("TestDog2"s);
            session.GetDogs()[0]->AddLoot(loot_ptr);
            return session;
        }();

        WHEN("session is serialized") {
            {
                serialization::GameSessionRepr rep{test_session};
                output_archive << rep;
            }
            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::GameSessionRepr repr;
                input_archive >> repr;
                const auto restored = repr.Restore(&map);
                CHECK(test_session.GetDogs()[0]->GetName() == restored.GetDogs()[0]->GetName());
                CHECK(test_session.GetDogs()[0]->GetLootsInBag().front()->GetLootPosition().x == restored.GetLoots()[0]->GetLootPosition().x);
                CHECK(test_session.GetDogs()[0]->GetLootsInBag().front()->GetLootPosition().y == restored.GetLoots()[0]->GetLootPosition().y);
                CHECK(restored.GetDogs()[0]->GetLootsInBag().front()->GetLootPosition().x == test_session.GetLoots()[0]->GetLootPosition().x);
                CHECK(restored.GetDogs()[0]->GetLootsInBag().front()->GetLootPosition().y == test_session.GetLoots()[0]->GetLootPosition().y);
                CHECK(test_session.GetDogs()[1]->GetName() == restored.GetDogs()[1]->GetName());
                CHECK(test_session.GetLootsCount() == restored.GetLootsCount());
                CHECK(test_session.GetDogIndexById(0) == restored.GetDogIndexById(0));
                CHECK(test_session.GetDogIndexById(1) == restored.GetDogIndexById(1));
                CHECK(test_session.GetDogById(0)->GetName() == restored.GetDogById(0)->GetName());
                CHECK(test_session.GetDogById(1)->GetName() == restored.GetDogById(1)->GetName());
                CHECK(test_session.GetDogIdToIndexMap().size() == restored.GetDogIdToIndexMap().size());
            }
        }
    }
}

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

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

SCENARIO_METHOD(Fixture, "Game Serialization") {
    GIVEN("a dog") {
        const auto test_dog = [] {
            app::Dog dog{ "Pluto"s, 42};
            dog.SetDogPosition({42.2, 12.5});
            dog.AddScoreValue(42);
            dog.AddLootToBag(std::make_shared<app::Loot>(app::Loot{0, 2, {1.2, 3.4}}));
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
            }
        }
    }
    GIVEN("game session collection") {
        model::Game game;
        model::Map map_1(model::Map::Id("TestMap_1"s), "Test map 1", 1.5, 3);
        model::Map map_2(model::Map::Id("TestMap_2"s), "Test map 2", 1.5, 3);
        model::Map map_3(model::Map::Id("TestMap_3"s), "Test map 3", 1.5, 3);
        game.AddMap(map_1);
        game.AddMap(map_2);
        game.AddMap(map_3);

        auto test_session_1 = game.AddSession(map_1.GetId());
        const auto loot_ptr_1 = std::make_shared<app::Loot>(1, 1, app::LootPosition{1.2, 3.4});
        test_session_1->AddLoot(loot_ptr_1);
        test_session_1->AddDog("TestDog1"s);
        test_session_1->AddDog("TestDog2"s);
        test_session_1->GetDogs().at(0)->AddLootToBag(loot_ptr_1);

        auto test_session_2 = game.AddSession(map_2.GetId());
        const auto loot_ptr_2 = std::make_shared<app::Loot>(2, 1, app::LootPosition{3.4, 5.6});
        test_session_2->AddLoot(loot_ptr_2);
        test_session_2->AddDog("TestDog3"s);
        test_session_2->AddDog("TestDog4"s);
        test_session_2->GetDogs().at(0)->AddLootToBag(loot_ptr_2);

        auto test_session_3 = game.AddSession(map_3.GetId());
        const auto loot_ptr_3 = std::make_shared<app::Loot>(3, 1, app::LootPosition{6.7, 8.9});
        test_session_3->AddLoot(loot_ptr_3);
        test_session_3->AddDog("TestDog5"s);
        test_session_3->AddDog("TestDog6"s);
        test_session_3->GetDogs().at(0)->AddLootToBag(loot_ptr_3);

        std::vector<std::shared_ptr<model::GameSession>> sessions;
        sessions.emplace_back(test_session_1);
        sessions.emplace_back(test_session_2);
        sessions.emplace_back(test_session_3);
        app::DogTokens dog_tokens;
        dog_tokens.AddDog(test_session_1->GetDogs().at(0), test_session_1);
        WHEN("session collection is serialized") {
            {
                serialization::GameSessionRepr session_repr{*test_session_3};
                output_archive << session_repr;
            }
            {
                serialization::GameSessionsRepr sessions_repr(game);
                output_archive << sessions_repr;
            }
            {
                serialization::TokenToDogRepr token_to_dog_repr(dog_tokens);
                output_archive << token_to_dog_repr;
            }
            THEN("session collection can be deserialized") {
                InputArchive input_archive{strm};

                serialization::GameSessionRepr session_repr;
                input_archive >> session_repr;
                const auto restored_session = session_repr.Restore(game);
                CHECK(*test_session_3->GetMap()->GetId() == *restored_session.GetMap()->GetId());

                serialization::GameSessionsRepr sessions_repr;
                input_archive >> sessions_repr;
                auto restored_sessions = sessions_repr.Restore(game);

                serialization::TokenToDogRepr token_to_dog_repr;
                input_archive >> token_to_dog_repr;
                app::DogTokens::TokenToDog token_to_dog = token_to_dog_repr.RestoreTokenToDog();
                app::DogTokens::TokenToSession token_to_session = token_to_dog_repr.RestoreTokenToSession(game);
                app::DogTokens dog_tokens_restored;
                dog_tokens_restored.SetTokenToDog(token_to_dog);
                dog_tokens_restored.SetTokenToSession(token_to_session);

                CHECK(dog_tokens.GetTokensToDog().size() == dog_tokens_restored.GetTokensToDog().size());
                CHECK(*dog_tokens.GetTokensToDog().begin()->first == *dog_tokens_restored.GetTokensToDog().begin()->first);
                CHECK(dog_tokens.GetTokensToDog().begin()->second->GetName() == dog_tokens_restored.GetTokensToDog().begin()->second->GetName());
                CHECK(*dog_tokens.GetTokensToSession().begin()->first == *dog_tokens_restored.GetTokensToSession().begin()->first);
                CHECK(*dog_tokens.GetTokensToSession().begin()->second->GetMap()->GetId() == *dog_tokens_restored.GetTokensToSession().begin()->second->GetMap()->GetId());

                for (size_t i = 0; i < restored_sessions.size(); i++) {
                    auto& restored = *restored_sessions[i];
                    auto& session = *sessions[i];
                    CHECK(*session.GetMap()->GetId() == *restored.GetMap()->GetId());
                    CHECK(session.GetDogs().at(0)->GetName() == restored.GetDogs().at(0)->GetName());
                    CHECK(session.GetDogs().at(0)->GetLootsInBag().front()->GetLootPosition().x == restored.GetDogs().at(0)->GetLootsInBag().front()->GetLootPosition().x);
                    CHECK(session.GetDogs().at(0)->GetLootsInBag().front()->GetLootPosition().y == restored.GetDogs().at(0)->GetLootsInBag().front()->GetLootPosition().y);
                    CHECK(session.GetDogs().at(1)->GetName() == restored.GetDogs().at(1)->GetName());
                    CHECK(session.GetLootsCount() == restored.GetLootsCount());
                    CHECK(session.GetDogPtrById(0)->GetName() == restored.GetDogPtrById(0)->GetName());
                    CHECK(session.GetDogPtrById(1)->GetName() == restored.GetDogPtrById(1)->GetName());
                }
            }
        }
    }
}

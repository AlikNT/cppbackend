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
    GIVEN("game session collection") {
        model::Game game;
        model::Map map_1(model::Map::Id("TestMap_1"s), "Test map 1", 1.5, 3);
        model::Map map_2(model::Map::Id("TestMap_2"s), "Test map 2", 1.5, 3);
        model::Map map_3(model::Map::Id("TestMap_3"s), "Test map 3", 1.5, 3);
        game.AddMap(map_1);
        game.AddMap(map_2);
        game.AddMap(map_3);

        auto test_session_1 = game.AddSession(map_1.GetId());
        const auto loot_ptr_1 = std::make_shared<app::Loot>(1, app::LootPosition{1.2, 3.4});
        test_session_1->AddLoot(loot_ptr_1);
        test_session_1->AddDog("TestDog1"s);
        test_session_1->AddDog("TestDog2"s);
        test_session_1->GetDogs()[0]->AddLoot(loot_ptr_1);

        auto test_session_2 = game.AddSession(map_2.GetId());
        const auto loot_ptr_2 = std::make_shared<app::Loot>(1, app::LootPosition{3.4, 5.6});
        test_session_2->AddLoot(loot_ptr_2);
        test_session_2->AddDog("TestDog3"s);
        test_session_2->AddDog("TestDog4"s);
        test_session_2->GetDogs()[0]->AddLoot(loot_ptr_2);

        auto test_session_3 = game.AddSession(map_3.GetId());
        const auto loot_ptr_3 = std::make_shared<app::Loot>(1, app::LootPosition{6.7, 8.9});
        test_session_3->AddLoot(loot_ptr_3);
        test_session_3->AddDog("TestDog5"s);
        test_session_3->AddDog("TestDog6"s);
        test_session_3->GetDogs()[0]->AddLoot(loot_ptr_3);

        std::vector<std::shared_ptr<model::GameSession>> sessions;
        sessions.emplace_back(test_session_1);
        sessions.emplace_back(test_session_2);
        sessions.emplace_back(test_session_3);
        app::Players test_players;
        app::PlayerTokens player_tokens;
        player_tokens.AddPlayer(test_players.AddPlayer(test_session_1->GetDogs()[0], sessions[0]));
        player_tokens.AddPlayer(test_players.AddPlayer(test_session_1->GetDogs()[1], sessions[1]));
        player_tokens.AddPlayer(test_players.AddPlayer(test_session_2->GetDogs()[0], sessions[0]));
        player_tokens.AddPlayer(test_players.AddPlayer(test_session_2->GetDogs()[1], sessions[1]));

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
                serialization::PlayersRepr players_repr(test_players);
                output_archive << players_repr;
            }
            {
                serialization::TokenToPlayer player_tokens_repr(player_tokens);
                output_archive << player_tokens_repr;
            }
            THEN("session collection can be deserialized") {
                InputArchive input_archive{strm};

                serialization::GameSessionRepr session_repr;
                input_archive >> session_repr;
                const auto restored_session = session_repr.Restore(game);
                CHECK(*test_session_3->GetMap()->GetId() == *restored_session.GetMap()->GetId());

                serialization::GameSessionsRepr sessions_repr;
                input_archive >> sessions_repr;
                const auto restored_sessions = sessions_repr.Restore(game);

                serialization::PlayersRepr players_repr;
                input_archive >> players_repr;
                const auto restored_players = players_repr.Restore(game);

                serialization::TokenToPlayer player_tokens_repr;
                input_archive >> player_tokens_repr;
                app::PlayerTokens::TokenToPlayer restored_token_to_player = player_tokens_repr.Restore(game);
                app::PlayerTokens restored_player_tokens;
                restored_player_tokens.SetTokens(restored_token_to_player);

                for (size_t i = 0; i < restored_sessions.size(); i++) {
                    const auto& restored = *restored_sessions[i];
                    const auto& session = *sessions[i];
                    CHECK(*session.GetMap()->GetId() == *restored.GetMap()->GetId());
                    CHECK(session.GetDogs()[0]->GetName() == restored.GetDogs()[0]->GetName());
                    CHECK(session.GetDogs()[0]->GetLootsInBag().front()->GetLootPosition().x == restored.GetLoots()[0]->GetLootPosition().x);
                    CHECK(session.GetDogs()[0]->GetLootsInBag().front()->GetLootPosition().y == restored.GetLoots()[0]->GetLootPosition().y);
                    CHECK(restored.GetDogs()[0]->GetLootsInBag().front()->GetLootPosition().x == session.GetLoots()[0]->GetLootPosition().x);
                    CHECK(restored.GetDogs()[0]->GetLootsInBag().front()->GetLootPosition().y == session.GetLoots()[0]->GetLootPosition().y);
                    CHECK(session.GetDogs()[1]->GetName() == restored.GetDogs()[1]->GetName());
                    CHECK(session.GetLootsCount() == restored.GetLootsCount());
                    CHECK(session.GetDogIndexById(0) == restored.GetDogIndexById(0));
                    CHECK(session.GetDogIndexById(1) == restored.GetDogIndexById(1));
                    CHECK(session.GetDogById(0)->GetName() == restored.GetDogById(0)->GetName());
                    CHECK(session.GetDogById(1)->GetName() == restored.GetDogById(1)->GetName());
                    CHECK(session.GetDogIdToIndexMap().size() == restored.GetDogIdToIndexMap().size());
                }
                CHECK(test_players.GetDogMapToPlayer().size() == restored_players.GetDogMapToPlayer().size());
                CHECK(test_players.GetPlayers()[0]->GetDogPtr()->GetName() == restored_players.GetPlayers()[0]->GetDogPtr()->GetName());
                CHECK(test_players.GetPlayers()[1]->GetDogPtr()->GetName() == restored_players.GetPlayers()[1]->GetDogPtr()->GetName());
                CHECK(test_players.GetPlayers()[2]->GetDogPtr()->GetName() == restored_players.GetPlayers()[2]->GetDogPtr()->GetName());
                CHECK(test_players.GetPlayers()[3]->GetDogPtr()->GetName() == restored_players.GetPlayers()[3]->GetDogPtr()->GetName());
                CHECK(test_players.GetPlayers()[0]->GetSession()->GetDogs()[0]->GetName() == restored_players.GetPlayers()[0]->GetSession()->GetDogs()[0]->GetName());
                CHECK(test_players.GetPlayers()[1]->GetSession()->GetDogs()[0]->GetName() == restored_players.GetPlayers()[1]->GetSession()->GetDogs()[0]->GetName());
                CHECK(test_players.GetPlayers()[0]->GetSession()->GetDogs()[1]->GetName() == restored_players.GetPlayers()[0]->GetSession()->GetDogs()[1]->GetName());
                CHECK(test_players.GetPlayers()[1]->GetSession()->GetDogs()[1]->GetName() == restored_players.GetPlayers()[1]->GetSession()->GetDogs()[1]->GetName());
                for (const auto& [token, player] : player_tokens.GetTokens()) {
                    CHECK(restored_player_tokens.GetTokens()[token]->GetPlayerId() == player->GetPlayerId());
                }
            }
        }
    }
}

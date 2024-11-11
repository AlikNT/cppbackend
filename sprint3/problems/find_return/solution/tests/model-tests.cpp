#define BOOST_TEST_MODULE GameServerTests
#include <catch2/catch_test_macros.hpp>

#include "../src/model.h"
#include "../src/json_loader.h"
#include "../src/app.h"

using namespace std::literals;

SCENARIO("Model testing") {
    GIVEN("Game initialization") {
        model::Game game = json_loader::LoadGame("../tests/data/config.json"s);
        app::Application app(game);
        THEN("Check correct parsing loots generator config") {
            auto loot_generator = game.GetLootGenerator();
            REQUIRE(loot_generator->GetProbability() == 0.5);
            REQUIRE(loot_generator->GetTimeInterval() == 5s);
        }
        THEN("Check correct parsing loot types") {
            REQUIRE(game.FindMap(model::Map::Id("map1"s))->GetLootTypesCount() == 2);
            REQUIRE(game.FindMap(model::Map::Id("town"s))->GetLootTypesCount() == 2);
        }
        THEN("Check correct parsing bag capacity") {
            REQUIRE(game.FindMap(model::Map::Id("map1"s))->GetBagCapacity() == 3);
            REQUIRE(game.FindMap(model::Map::Id("town"s))->GetBagCapacity() == 3);
        }
        WHEN("Add session and dogs to map") {
            const auto& session = game.AddSession(model::Map::Id("town"s));
            session->AddDog("DogName1");
            session->AddDog("DogName2");
            session->AddDog("DogName3");
            session->AddLoots(2);
            THEN("Check correct loots adding") {
                REQUIRE(session->GetLootsCount() == 2);
            }
            THEN("Check correct loots positions") {
                const auto& loots = session->GetLoots();
                for (const auto& [loot_id, loot] : loots) {
                    REQUIRE(loot.GetLootTypeId() < 2);
                    auto road_indexes = session->GetMap()->GetRoadsByPosition(loot.GetLootPosition());
                    REQUIRE(!road_indexes.empty());
                }
            }
        }
    }
}

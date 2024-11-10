#define _USE_MATH_DEFINES
#define CATCH_CONFIG_MAIN

#include "../src/collision_detector.h"

// #include "catch.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <vector>
#include <sstream>

// Напишите здесь тесты для функции collision_detector::FindGatherEvents

class TestProvider : public collision_detector::ItemGathererProvider {
public:
    virtual ~TestProvider() = default;

    void AddItem(geom::Point2D position, double width) {
        items_.emplace_back(collision_detector::Item{position, width});
    }

    void AddGatherer(geom::Point2D start_pos, geom::Point2D end_pos, double width) {
        gatherers_.emplace_back(collision_detector::Gatherer{start_pos, end_pos, width});
    }

    size_t ItemsCount() const override {
        return items_.size();
    }

    collision_detector::Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    collision_detector::Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }

private:
    std::vector<collision_detector::Item> items_;
    std::vector<collision_detector::Gatherer> gatherers_;
};

namespace Catch {
template<>
struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(collision_detector::GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";
        return tmp.str();
    }
};
} // namespace Catch

bool AreEventsEqual(const collision_detector::GatheringEvent& lhs, const collision_detector::GatheringEvent& rhs) {
    constexpr double EPSILON = 1e-10;
    return lhs.item_id == rhs.item_id &&
           lhs.gatherer_id == rhs.gatherer_id &&
           std::abs(lhs.sq_distance - rhs.sq_distance) < EPSILON &&
           std::abs(lhs.time - rhs.time) < EPSILON;
}

TEST_CASE("Test FindGatherEvents for different scenarios") {
    TestProvider provider;

    SECTION("Single gatherer moving towards a single item") {
        provider.AddItem({10, 0}, 0.71);
        provider.AddGatherer({0, 0.8}, {20, 0.8}, 0.1);

        auto events = collision_detector::FindGatherEvents(provider);

        std::vector<collision_detector::GatheringEvent> expected_events = {
            {0, 0, 0.64, 0.5} // item_id, gatherer_id, sq_distance, time
        };

        REQUIRE(events.size() == expected_events.size());
        CAPTURE(events.front());
        CAPTURE(events.size());
        REQUIRE(std::equal(events.begin(), events.end(), expected_events.begin(), AreEventsEqual));
    }

    SECTION("Multiple gatherers and items") {
        provider.AddItem({10, 0}, 1);
        provider.AddItem({5, 5}, 1);
        provider.AddGatherer({0, 0}, {20, 0}, 1);
        provider.AddGatherer({0, 0}, {10, 10}, 1);

        auto events = collision_detector::FindGatherEvents(provider);

        std::vector<collision_detector::GatheringEvent> expected_events = {
            {0, 0, 0, 0.5}, // Gatherer 0 collects Item 0
            {1, 1, 0, 0.5}  // Gatherer 1 collects Item 1
        };

        REQUIRE(events.size() == expected_events.size());
        REQUIRE(std::equal(events.begin(), events.end(), expected_events.begin(), AreEventsEqual));
    }

    SECTION("No collection due to distance") {
        provider.AddItem({100, 100}, 1);
        provider.AddGatherer({0, 0}, {10, 10}, 1);

        auto events = collision_detector::FindGatherEvents(provider);

        REQUIRE(events.empty());
    }

    SECTION("Chronological order of events") {
        provider.AddItem({10, 0}, 1);
        provider.AddItem({15, 0}, 1);
        provider.AddGatherer({0, 0}, {20, 0}, 1);

        auto events = collision_detector::FindGatherEvents(provider);

        std::vector<collision_detector::GatheringEvent> expected_events = {
            {0, 0, 0, 0.5},
            {1, 0, 0, 0.75}
        };

        REQUIRE(events.size() == expected_events.size());
        REQUIRE(std::equal(events.begin(), events.end(), expected_events.begin(), AreEventsEqual));
    }
}

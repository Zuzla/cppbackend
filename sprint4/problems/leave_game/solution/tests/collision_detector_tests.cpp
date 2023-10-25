#define _USE_MATH_DEFINES
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "../src/collision_detector.h"

#include <sstream>

using namespace collision_detector;
using Catch::Matchers::WithinRel;

namespace Catch
{
    template <>
    struct StringMaker<GatheringEvent>
    {
        static std::string convert(GatheringEvent const &value)
        {
            std::ostringstream tmp;
            tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";

            return tmp.str();
        }
    };
}


using namespace std::literals;

TEST_CASE("1 of 1 on x-axis 1", "GatherEvents")
{
    Item item{0, {5, 0}, 0.6};
    Gatherer gatherer{"0", {0, 0}, {7, 0}, 0.6};
    Provider provider;
    provider.AddItem(item);
    provider.AddGatherer(gatherer);
    auto events = FindGatherEvents(provider);

    CHECK(events.size() == 1);
    CHECK(events.at(0).item_id == 0);
    CHECK(events.at(0).gatherer_id == "0");
    CHECK_THAT(events.at(0).sq_distance, WithinRel(0.0, 1e-9));
    CHECK_THAT(events.at(0).time, WithinRel((item.position.x / gatherer.end_pos.x), 1e-9));
}

TEST_CASE("1 of 1 on x-axis 2", "GatherEvents")
{
    Item item{0, {5, 0}, 0.6};
    Gatherer gatherer{"0", {0, 0}, {5, 0}, 0.6};
    Provider provider;
    provider.AddItem(item);
    provider.AddGatherer(gatherer);
    auto events = FindGatherEvents(provider);

    CHECK(events.size() == 1);
    CHECK(events.at(0).item_id == 0);
    CHECK(events.at(0).gatherer_id == "0");
    CHECK_THAT(events.at(0).sq_distance, WithinRel(0.0, 1e-9));
    CHECK_THAT(events.at(0).time, WithinRel((item.position.x / gatherer.end_pos.x), 1e-9));
}

TEST_CASE("Gather collect one item moving on y-axis", "GatherEvents")
{
    Item item{0, {0, 6}, 0.6};
    Gatherer gatherer{"0", {0, 0}, {0, 15}, 0.6};
    Provider provider;
    provider.AddItem(item);
    provider.AddGatherer(gatherer);
    auto events = FindGatherEvents(provider);

    CHECK(events.size() == 1);
    CHECK(events.at(0).item_id == 0);
    CHECK(events.at(0).gatherer_id == "0");
    CHECK_THAT(events.at(0).sq_distance, WithinRel(0.0, 1e-9));
    CHECK_THAT(events.at(0).time, WithinRel((item.position.y / gatherer.end_pos.y), 1e-9));
}

TEST_CASE("2 of 2 on x-axis", "GatherEvents")
{
    Item item1{0, {6, 0}, 0.6};
    Item item2{1, {2, 0}, 0.6};
    Gatherer gatherer{"0", {0, 0}, {15, 0}, 0.6};
    Provider provider;
    provider.AddItem(item1);
    provider.AddItem(item2);
    provider.AddGatherer(gatherer);
    auto events = FindGatherEvents(provider);

    CHECK(events.size() == 2);

    CHECK(events.at(0).item_id == 1);
    CHECK(events.at(0).gatherer_id == "0");
    CHECK_THAT(events.at(0).sq_distance, WithinRel(0.0, 1e-9));
    CHECK_THAT(events.at(0).time, WithinRel((item2.position.x / gatherer.end_pos.x), 1e-9));

    CHECK(events.at(1).item_id == 0);
    CHECK(events.at(1).gatherer_id == "0");
    CHECK_THAT(events.at(1).sq_distance, WithinRel(0.0, 1e-9));
    CHECK_THAT(events.at(1).time, WithinRel((item1.position.x / gatherer.end_pos.x), 1e-9));
}

TEST_CASE("1 of 2 ob x-axis", "GatherEvents")
{
    Item item1{0, {7, 0}, 0.6};
    Item item2{1, {1, 0}, 0.6};
    Gatherer gatherer{"0", {0, 0}, {4, 0}, 0.6};
    Provider provider;
    provider.AddItem(item1);
    provider.AddItem(item2);
    provider.AddGatherer(gatherer);
    auto events = FindGatherEvents(provider);

    CHECK(events.size() == 1);

    CHECK(events.at(0).item_id == 1);
    CHECK(events.at(0).gatherer_id == "0");
    CHECK_THAT(events.at(0).sq_distance, WithinRel(0.0, 1e-9));
    CHECK_THAT(events.at(0).time, WithinRel((item2.position.x / gatherer.end_pos.x), 1e-9));
}

TEST_CASE("2 of 2 on x-axis and y-axis", "GatherEvents")
{
    Item item1{0, {0, 8.5}, 0.6};
    Item item2{1, {3.4, 0}, 0.6};
    Gatherer gatherer1{"0", {0, 0}, {12, 0}, 0.6};
    Gatherer gatherer2{"1", {0, 0}, {0, 15}, 0.6};
    Provider provider;
    provider.AddItem(item1);
    provider.AddItem(item2);
    provider.AddGatherer(gatherer1);
    provider.AddGatherer(gatherer2);
    std::vector<GatheringEvent> events = FindGatherEvents(provider);

    CHECK(events.size() == 2);

    CHECK(events.at(0).item_id == 1);
    CHECK(events.at(0).gatherer_id == "0");
    CHECK_THAT(events.at(0).sq_distance, WithinRel(0.0, 1e-9));
    CHECK_THAT(events.at(0).time, WithinRel((item2.position.x / gatherer1.end_pos.x), 1e-9));

    CHECK(events.at(1).item_id == 0);
    CHECK(events.at(1).gatherer_id == "1");
    CHECK_THAT(events.at(1).sq_distance, WithinRel(0.0, 1e-9));
    CHECK_THAT(events.at(1).time, WithinRel((item1.position.y / gatherer2.end_pos.y), 1e-9));
}
#include <cmath>

#include <catch2/catch.hpp>

#include "kaacore/log.h"
#include "kaacore/node_transitions.h"
#include "kaacore/transitions.h"

#include "runner.h"

using namespace std::chrono_literals;

TEST_CASE("test_nested_infinite_transitions_sequence", "[transitions]")
{
    kaacore::initialize_logging();

    kaacore::NodeOwnerPtr node = kaacore::make_node();

    auto tr_inner = kaacore::make_node_transitions_sequence(
        {
            kaacore::make_node_transition<kaacore::NodePositionTransition>(
                glm::dvec2{10., 10.}, kaacore::AttributeTransitionMethod::set,
                1.s, kaacore::TransitionWarping{}),
            kaacore::make_node_transition<kaacore::NodePositionTransition>(
                glm::dvec2{-10., -10.}, kaacore::AttributeTransitionMethod::add,
                1.s, kaacore::TransitionWarping{}),
        },
        kaacore::TransitionWarping{0, false});

    REQUIRE(tr_inner->duration.count() == INFINITY);
    REQUIRE(tr_inner->internal_duration.count() == 2.);

    auto tr_outer = kaacore::make_node_transitions_sequence(
        {kaacore::make_node_transition<kaacore::NodeRotationTransition>(
             10., kaacore::AttributeTransitionMethod::set, 1.s,
             kaacore::TransitionWarping{}),
         tr_inner},
        kaacore::TransitionWarping{0, false});

    REQUIRE(tr_outer->duration.count() == INFINITY);
    REQUIRE(tr_outer->internal_duration.count() == 3.);
}

#include <catch2/catch.hpp>

#include "kaacore/fonts.h"
#include "kaacore/nodes.h"
#include "kaacore/shapes.h"

#include "runner.h"

TEST_CASE("test_empty_text_shape", "[fonts]")
{
    auto engine = initialize_testing_engine();

    auto text_node = kaacore::make_node(kaacore::NodeType::text);
    text_node->text.content("");
    REQUIRE(text_node->shape().type == kaacore::ShapeType::none);
}

TEST_CASE("test_whitespace_text_shape", "[fonts]")
{
    auto engine = initialize_testing_engine();

    auto text_node = kaacore::make_node(kaacore::NodeType::text);
    text_node->text.content(" ");
    REQUIRE(text_node->shape().type == kaacore::ShapeType::freeform);
}

TEST_CASE("test_update_empty_text", "[fonts]")
{
    auto engine = initialize_testing_engine();
    TestingScene scene;

    auto text_node_owner = kaacore::make_node(kaacore::NodeType::text);
    text_node_owner->text.content("");
    REQUIRE(text_node_owner->shape().type == kaacore::ShapeType::none);

    auto text_node = scene.root_node.add_child(text_node_owner);

    scene.update_function = [&scene, &text_node](auto dt) {
        if (scene.frames_left < 2) {
            text_node->text.content("X");
        }
    };

    scene.run_on_engine(2);
}

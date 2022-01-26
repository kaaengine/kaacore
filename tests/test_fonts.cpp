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

#include <catch2/catch.hpp>

#include "kaacore/engine.h"
#include "kaacore/nodes.h"
#include "kaacore/physics.h"
#include "runner.h"

TEST_CASE("Test hitboxes", "[hitboxes][nodes]")
{
    kaacore::initialize_logging();

    auto engine = initialize_testing_engine();

    SECTION("Add as direct child")
    {
        auto space = kaacore::make_node(kaacore::NodeType::space);
        kaacore::NodeOwnerPtr tmp = kaacore::make_node(kaacore::NodeType::body);
        auto body = space->add_child(tmp);
        auto hitbox = kaacore::make_node(kaacore::NodeType::hitbox);
        hitbox->shape(kaacore::Shape::Circle(1.));
        REQUIRE_NOTHROW(body->add_child(hitbox));

        TestingScene scene;
        scene.update_function = [&scene, &space](auto dt) {
            scene.root_node.add_child(space);
        };
    }

    SECTION("Add as indirect child")
    {
        auto space = kaacore::make_node(kaacore::NodeType::space);
        auto tmp_body = kaacore::make_node(kaacore::NodeType::body);
        auto tmp_node = kaacore::make_node();
        auto hitbox = kaacore::make_node(kaacore::NodeType::hitbox);
        hitbox->shape(kaacore::Shape::Circle(1.));
        auto body = space->add_child(tmp_body);
        auto node = body->add_child(tmp_node);
        REQUIRE_NOTHROW(node->add_child(hitbox));

        TestingScene scene;
        scene.update_function = [&scene, &space](auto dt) {
            scene.root_node.add_child(space);
        };
    }

    SECTION("Add to tree as direct child")
    {
        TestingScene scene;
        scene.update_function = [&scene](auto dt) {
            auto tmp_space = kaacore::make_node(kaacore::NodeType::space);
            auto space = scene.root_node.add_child(tmp_space);
            kaacore::NodeOwnerPtr tmp_body =
                kaacore::make_node(kaacore::NodeType::body);
            auto body = space->add_child(tmp_body);
            auto hitbox = kaacore::make_node(kaacore::NodeType::hitbox);
            hitbox->shape(kaacore::Shape::Circle(1.));
            REQUIRE_NOTHROW(body->add_child(hitbox));
        };
        scene.run_on_engine(1);
    }

    SECTION("Add to tree as indirect child")
    {
        TestingScene scene;
        scene.update_function = [&scene](auto dt) {
            auto tmp_space = kaacore::make_node(kaacore::NodeType::space);
            auto space = scene.root_node.add_child(tmp_space);
            auto tmp_body = kaacore::make_node(kaacore::NodeType::body);
            auto tmp_node = kaacore::make_node();
            auto body = space->add_child(tmp_body);
            auto node = body->add_child(tmp_node);
            auto hitbox = kaacore::make_node(kaacore::NodeType::hitbox);
            hitbox->shape(kaacore::Shape::Circle(1.));
            REQUIRE_NOTHROW(node->add_child(hitbox));
        };
        scene.run_on_engine(1);
    }
}

#include <math.h>

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

    SECTION("Hitbox chain transformations")
    {
        TestingScene scene;
        auto owned_node = kaacore::make_node();
        auto owned_node2 = kaacore::make_node();
        auto owned_body = kaacore::make_node(kaacore::NodeType::body);
        auto owned_space = kaacore::make_node(kaacore::NodeType::space);
        auto owned_hitbox = kaacore::make_node(kaacore::NodeType::hitbox);
        auto owned_hitbox2 = kaacore::make_node(kaacore::NodeType::hitbox);

        auto shape = kaacore::Shape::Box(glm::dvec2(2.));
        owned_hitbox->shape(shape);
        owned_hitbox2->shape(shape);

        auto space = scene.root_node.add_child(owned_space);
        auto body = space->add_child(owned_body);
        auto node = body->add_child(owned_node);
        auto hitbox = node->add_child(owned_hitbox);
        auto node2 = hitbox->add_child(owned_node2);
        auto hitbox2 = node2->add_child(owned_hitbox2);

        auto body_scale = glm::dvec2(2.);
        body->scale(body_scale);
        node->position(glm::dvec2(2., 0));
        hitbox->rotation(M_PI / 2);
        node2->position(glm::dvec2(0, 10.));

        auto expected_hitbox_transformation =
            hitbox->get_relative_transformation(body.get());
        auto expected_hitbox2_transformation =
            hitbox2->get_relative_transformation(body.get());
        expected_hitbox_transformation |=
            kaacore::Transformation::scale(body_scale);
        expected_hitbox2_transformation |=
            kaacore::Transformation::scale(body_scale);

        auto result = calculate_inherited_hitbox_transformation(hitbox.get());
        auto result2 = calculate_inherited_hitbox_transformation(hitbox2.get());

        auto transformation_approx_equal =
            [](const kaacore::Transformation& expected_result,
               const kaacore::Transformation& result) {
                for (int i = 0; i < 16; i++) {
                    int x = i / 4;
                    int y = i % 4;
                    REQUIRE(
                        expected_result.at(x, y) == Approx(result.at(x, y)));
                }
            };

        transformation_approx_equal(expected_hitbox_transformation, result);
        transformation_approx_equal(expected_hitbox2_transformation, result2);
    }
}

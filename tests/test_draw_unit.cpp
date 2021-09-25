#include <algorithm>
#include <vector>

#include <catch2/catch.hpp>
#include <glm/glm.hpp>

#include "kaacore/draw_unit.h"
#include "kaacore/engine.h"

#include "runner.h"

TEST_CASE(
    "Test direct rendering with DrawBucket", "[.][visual_test][draw_unit]")
{
    auto engine = initialize_testing_engine(true);

    TestingScene scene;
    scene.camera().position({0., 0.});

    scene.update_function = [&](auto dt) {
        auto test_shape1 =
            kaacore::Shape::Circle(7.5, {-35. + 0.15 * scene.frames_left, 25.});
        kaacore::DrawBucketKey dbk;
        dbk.render_passes =
            kaacore::RenderPassIndexSet{std::unordered_set<int16_t>{0}};
        dbk.viewports =
            kaacore::ViewportIndexSet{std::unordered_set<int16_t>{0}};
        dbk.z_index = 0;
        dbk.root_distance = 0;
        dbk.texture = nullptr;
        dbk.material =
            kaacore::get_engine()->renderer->default_material.res_ptr.get();
        ;
        dbk.state_flags = 0;
        dbk.stencil_flags = 0;
        kaacore::DrawUnit du1{1, {}};
        du1.details.vertices = test_shape1.vertices;
        du1.details.indices = test_shape1.indices;

        auto test_shape2 = kaacore::Shape::Box({50., 5});
        kaacore::DrawUnit du2{2, {}};
        du2.details.vertices = test_shape2.vertices;
        for (auto& vt : du2.details.vertices) {
            vt.rgba = {1., 0., 0., 0.5};
        }
        du2.details.indices = test_shape2.indices;

        kaacore::DrawBucket draw_bucket;
        draw_bucket.draw_units = {du1, du2};

        auto batch = kaacore::RenderBatch::from_bucket(dbk, draw_bucket);
        engine->renderer->render_batch(
            batch, dbk.render_passes, dbk.viewports,
            scene.render_passes.take_snapshot(),
            scene.viewports.take_snapshot());
    };
    scene.run_on_engine(500);
}

TEST_CASE("test_calculating_node_draw_unit_updates", "[draw_unit]")
{
    auto engine = initialize_testing_engine(true);

    auto test_shape_1 = kaacore::Shape::Circle(7.5);
    auto test_shape_2 = kaacore::Shape::Box({7.5, 2.5});
    auto test_shape_3 = kaacore::Shape::Circle(10.);
    const auto make_test_nodes = [&]() -> kaacore::NodeOwnerPtr {
        auto node_1 = kaacore::make_node();
        node_1->shape(test_shape_1);

        auto node_2 = kaacore::make_node();
        node_2->position({15., 10.});
        node_2->color({0., 1., 0., 1.});
        node_2->shape(test_shape_2);
        node_1->add_child(node_2);

        return node_1;
    };

    const auto simulate_frame_step = [](const kaacore::NodePtr node) {
        if (node->has_draw_unit_updates()) {
            auto node_mod = node->calculate_draw_unit_updates().first;
            node->clear_draw_unit_updates(node_mod->lookup_key);
        }
    };

    TestingScene scene;
    scene.update_function = [&](auto dt) {};

    auto node_1_owner = make_test_nodes();
    kaacore::NodePtr node_1 = scene.root_node.add_child(node_1_owner);
    kaacore::NodePtr node_2 = node_1->children()[0];

    SECTION("Test insert")
    {
        REQUIRE(node_1->has_draw_unit_updates());
        auto [node_1_mod_1, node_1_mod_2] =
            node_1->calculate_draw_unit_updates();

        REQUIRE(node_1_mod_1.has_value());
        REQUIRE(
            node_1_mod_1->type == kaacore::DrawUnitModification::Type::insert);
        REQUIRE(node_1_mod_1->updated_vertices_indices == true);
        REQUIRE(not node_1_mod_1->state_update.vertices.empty());
        REQUIRE(not node_1_mod_1->state_update.indices.empty());

        REQUIRE(
            node_1_mod_1->lookup_key.render_passes ==
            kaacore::RenderPassIndexSet{std::unordered_set<int16_t>{0}});
        REQUIRE(node_1_mod_1->lookup_key.z_index == 0);
        REQUIRE(node_1_mod_1->lookup_key.root_distance == 1);
        REQUIRE(node_1_mod_1->lookup_key.texture == nullptr);
        REQUIRE(node_1_mod_1->lookup_key.material == nullptr);
        REQUIRE(node_1_mod_1->lookup_key.state_flags == 0u);
        REQUIRE(node_1_mod_1->lookup_key.stencil_flags == 0u);

        // no remove
        REQUIRE(not node_1_mod_2.has_value());

        node_1->clear_draw_unit_updates(node_1_mod_1->lookup_key);
        REQUIRE(not node_1->has_draw_unit_updates());
    }

    SECTION("Test remove")
    {
        simulate_frame_step(node_1);
        simulate_frame_step(node_2);

        auto node_1_remove_mod = node_1->calculate_draw_unit_removal();
        auto node_2_remove_mod = node_2->calculate_draw_unit_removal();
        REQUIRE(node_1_remove_mod.has_value());
        REQUIRE(node_1_remove_mod->id == node_1->scene_tree_id());
        REQUIRE(
            node_1_remove_mod->type ==
            kaacore::DrawUnitModification::Type::remove);
        REQUIRE(node_2_remove_mod.has_value());
        REQUIRE(node_2_remove_mod->id == node_2->scene_tree_id());
        REQUIRE(
            node_2_remove_mod->type ==
            kaacore::DrawUnitModification::Type::remove);
    }

    SECTION("Test immediate remove")
    {
        REQUIRE(not node_1->calculate_draw_unit_removal().has_value());
        REQUIRE(not node_2->calculate_draw_unit_removal().has_value());
    }

    SECTION("Test update - shape")
    {
        simulate_frame_step(node_1);
        simulate_frame_step(node_2);
        REQUIRE(not node_1->has_draw_unit_updates());
        REQUIRE(not node_2->has_draw_unit_updates());

        node_1->shape(test_shape_3);
        REQUIRE(node_1->has_draw_unit_updates());
        REQUIRE(not node_2->has_draw_unit_updates());

        node_2->shape(test_shape_3);
        REQUIRE(node_2->has_draw_unit_updates());
    }

    SECTION("Test update - position (parent)")
    {
        simulate_frame_step(node_1);
        simulate_frame_step(node_2);
        REQUIRE(not node_1->has_draw_unit_updates());
        REQUIRE(not node_2->has_draw_unit_updates());

        node_1->position({10., 10.});
        REQUIRE(node_1->has_draw_unit_updates());
        REQUIRE(node_2->has_draw_unit_updates());
    }

    SECTION("Test update - position (child)")
    {
        simulate_frame_step(node_1);
        simulate_frame_step(node_2);
        REQUIRE(not node_1->has_draw_unit_updates());
        REQUIRE(not node_2->has_draw_unit_updates());

        node_2->position({10., 10.});
        REQUIRE(not node_1->has_draw_unit_updates());
        REQUIRE(node_2->has_draw_unit_updates());
    }

    SECTION("Test update - z-index")
    {
        simulate_frame_step(node_1);
        simulate_frame_step(node_2);
        REQUIRE(not node_1->has_draw_unit_updates());
        REQUIRE(not node_2->has_draw_unit_updates());

        node_1->z_index(100);
        REQUIRE(node_1->has_draw_unit_updates());
        REQUIRE(not node_2->has_draw_unit_updates());

        auto [mod_1, mod_2] = node_1->calculate_draw_unit_updates();
        REQUIRE(mod_1->type == kaacore::DrawUnitModification::Type::insert);
        REQUIRE(mod_1->lookup_key.z_index == 100);
        REQUIRE(mod_2.has_value());
        REQUIRE(mod_2->type == kaacore::DrawUnitModification::Type::remove);
        REQUIRE(mod_2->lookup_key.z_index == 0);
    }
}

TEST_CASE("test_draw_bucket_modifications", "[draw_unit][draw_bucket]")
{
    auto engine = initialize_testing_engine(true);

    auto test_shape_1 = kaacore::Shape::Circle(7.5);
    auto test_shape_2 = kaacore::Shape::Box({7.5, 2.5});
    auto test_shape_3 = kaacore::Shape::Circle(10.);
    const auto make_test_nodes = [&]() -> kaacore::NodeOwnerPtr {
        auto node_1 = kaacore::make_node();
        node_1->shape(test_shape_1);

        auto node_2 = kaacore::make_node();
        node_2->position({15., 10.});
        node_2->color({0., 1., 0., 1.});
        node_2->shape(test_shape_2);
        node_1->add_child(node_2);

        auto node_3 = kaacore::make_node();
        node_3->position({-15., 10.});
        node_3->color({0., 0., 1., 1.});
        node_3->shape(test_shape_3);
        node_1->add_child(node_3);

        auto node_4 = kaacore::make_node();
        node_4->position({-5., -20.});
        node_4->color({0., 1., 1., 1.});
        node_4->shape(test_shape_1);
        node_1->add_child(node_4);

        return node_1;
    };

    const auto gather_modifications =
        [](const kaacore::NodePtr node,
           std::vector<kaacore::DrawUnitModification>& out) {
            if (node->has_draw_unit_updates()) {
                auto [node_mod_1, node_mod_2] =
                    node->calculate_draw_unit_updates();
                if (node_mod_1) {
                    out.push_back(*node_mod_1);
                    node->clear_draw_unit_updates(node_mod_1->lookup_key);
                } else {
                    node->clear_draw_unit_updates(std::nullopt);
                }
                if (node_mod_2) {
                    out.push_back(*node_mod_2);
                }
            }
        };

    const auto validate_bucket_content =
        [](const kaacore::DrawBucket& db,
           std::vector<kaacore::NodePtr>&& nodes) {
            REQUIRE(db.draw_units.size() == nodes.size());
            for (auto n : nodes) {
                REQUIRE(
                    std::find_if(
                        db.draw_units.begin(), db.draw_units.end(),
                        [&n](const auto& du) {
                            return du.id == n->scene_tree_id();
                        }) != db.draw_units.end());
            }
        };

    TestingScene scene;
    scene.update_function = [&](auto dt) {};

    kaacore::DrawBucket draw_bucket;
    std::vector<kaacore::DrawUnitModification> modifications;

    auto node_1_owner = make_test_nodes();
    kaacore::NodePtr node_1 = scene.root_node.add_child(node_1_owner);
    kaacore::NodePtr node_2 = node_1->children()[0];
    kaacore::NodePtr node_3 = node_1->children()[1];
    kaacore::NodePtr node_4 = node_1->children()[2];

    // We won't be using node_1 as it has different lookup key
    // than it's children (due to root_distance)

    SECTION("Test lifecycle - insert and modify")
    {
        for (auto n : {node_2, node_3, node_4}) {
            REQUIRE(n->has_draw_unit_updates());
            gather_modifications(n, modifications);
        }

        std::sort(modifications.begin(), modifications.end());
        draw_bucket.consume_modifications(
            modifications.begin(), modifications.end());
        modifications.clear();

        validate_bucket_content(draw_bucket, {node_2, node_3, node_4});

        // simple position changes
        for (auto n : {node_2, node_3, node_4}) {
            n->position(n->position() + glm::dvec2{4., 10.});
            REQUIRE(n->has_draw_unit_updates());
            gather_modifications(n, modifications);
        }

        std::sort(modifications.begin(), modifications.end());
        draw_bucket.consume_modifications(
            modifications.begin(), modifications.end());
        modifications.clear();

        validate_bucket_content(draw_bucket, {node_2, node_3, node_4});
    }

    SECTION("Test lifecycle - insert and swap buckets")
    {
        for (auto n : {node_2, node_3, node_4}) {
            REQUIRE(n->has_draw_unit_updates());
            gather_modifications(n, modifications);
        }

        std::sort(modifications.begin(), modifications.end());
        draw_bucket.consume_modifications(
            modifications.begin(), modifications.end());
        modifications.clear();

        validate_bucket_content(draw_bucket, {node_2, node_3, node_4});

        node_2->position({5., 0.});
        node_3->z_index(15);
        node_4->position({5., 0.});
        for (auto n : {node_2, node_3, node_4}) {
            REQUIRE(n->has_draw_unit_updates());
            gather_modifications(n, modifications);
        }

        REQUIRE(modifications.size() == 4);
        // take out insert-to-other-bucket modification
        // as it would interfere with consuming modifications
        auto insert_mod_it = std::find_if(
            modifications.begin(), modifications.end(), [&](auto& du_mod) {
                return du_mod.type ==
                       kaacore::DrawUnitModification::Type::insert;
            });
        REQUIRE(insert_mod_it != modifications.end());
        REQUIRE(insert_mod_it->id == node_3->scene_tree_id());
        REQUIRE(insert_mod_it->lookup_key.z_index == 15);
        modifications.erase(insert_mod_it);

        std::sort(modifications.begin(), modifications.end());
        draw_bucket.consume_modifications(
            modifications.begin(), modifications.end());
        modifications.clear();

        validate_bucket_content(draw_bucket, {node_2, node_4});
    }

    SECTION("Test lifecycle - insert and remove all")
    {
        for (auto n : {node_2, node_3, node_4}) {
            REQUIRE(n->has_draw_unit_updates());
            gather_modifications(n, modifications);
        }

        std::sort(modifications.begin(), modifications.end());
        draw_bucket.consume_modifications(
            modifications.begin(), modifications.end());
        modifications.clear();

        validate_bucket_content(draw_bucket, {node_2, node_3, node_4});

        for (auto n : {node_2, node_3, node_4}) {
            n->z_index(15);
            REQUIRE(n->has_draw_unit_updates());
            gather_modifications(n, modifications);
        }

        REQUIRE(modifications.size() == 6);
        // take out insert-to-other-bucket modifications
        // as it would interfere with consuming modifications
        modifications.erase(
            std::remove_if(
                modifications.begin(), modifications.end(),
                [&](auto& du_mod) {
                    return du_mod.type ==
                           kaacore::DrawUnitModification::Type::insert;
                }),
            modifications.end());
        REQUIRE(modifications.size() == 3);

        std::sort(modifications.begin(), modifications.end());
        draw_bucket.consume_modifications(
            modifications.begin(), modifications.end());
        modifications.clear();
        REQUIRE(draw_bucket.draw_units.size() == 0);
    }

    SECTION("Test lifecycle - insert and swap buckets and add new node")
    {
        for (auto n : {node_2, node_3, node_4}) {
            REQUIRE(n->has_draw_unit_updates());
            gather_modifications(n, modifications);
        }

        std::sort(modifications.begin(), modifications.end());
        draw_bucket.consume_modifications(
            modifications.begin(), modifications.end());
        modifications.clear();

        validate_bucket_content(draw_bucket, {node_2, node_3, node_4});

        node_2->position({5., 0.});
        node_3->z_index(15);
        node_4->position({5., 0.});

        auto node_5_owner = kaacore::make_node();
        auto node_5 = node_1->add_child(node_5_owner);
        node_5->shape(test_shape_2);

        for (auto n : {node_2, node_3, node_4, node_5}) {
            REQUIRE(n->has_draw_unit_updates());
            gather_modifications(n, modifications);
        }

        REQUIRE(modifications.size() == 5);
        // take out insert-to-other-bucket modification
        // as it would interfere with consuming modifications
        auto insert_mod_it = std::find_if(
            modifications.begin(), modifications.end(), [&](auto& du_mod) {
                return du_mod.type ==
                       kaacore::DrawUnitModification::Type::insert;
            });
        REQUIRE(insert_mod_it != modifications.end());
        REQUIRE(insert_mod_it->id == node_3->scene_tree_id());
        REQUIRE(insert_mod_it->lookup_key.z_index == 15);
        modifications.erase(insert_mod_it);

        std::sort(modifications.begin(), modifications.end());
        draw_bucket.consume_modifications(
            modifications.begin(), modifications.end());
        modifications.clear();

        validate_bucket_content(draw_bucket, {node_2, node_4, node_5});
    }

    SECTION("Test lifecycle - toggle visibility")
    {
        for (auto n : {node_2, node_3, node_4}) {
            REQUIRE(n->has_draw_unit_updates());
            gather_modifications(n, modifications);
        }

        std::sort(modifications.begin(), modifications.end());
        draw_bucket.consume_modifications(
            modifications.begin(), modifications.end());
        modifications.clear();

        validate_bucket_content(draw_bucket, {node_2, node_3, node_4});

        node_2->visible(false);
        node_3->visible(true);
        node_4->visible(false);

        for (auto n : {node_2, node_4}) {
            REQUIRE(n->has_draw_unit_updates());
            gather_modifications(n, modifications);
        }
        REQUIRE(not node_3->has_draw_unit_updates());

        REQUIRE(modifications.size() == 2);

        std::sort(modifications.begin(), modifications.end());
        draw_bucket.consume_modifications(
            modifications.begin(), modifications.end());
        modifications.clear();

        validate_bucket_content(draw_bucket, {node_3});
    }
}

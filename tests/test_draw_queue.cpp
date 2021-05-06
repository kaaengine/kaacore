#include <catch2/catch.hpp>

#include "kaacore/draw_queue.h"
#include "kaacore/geometry.h"
#include "kaacore/shapes.h"

#include "runner.h"

TEST_CASE("test_draw_queue_rendering", "[.][visual_test][draw_queue]")
{
    auto engine = initialize_testing_engine(true);

    TestingScene scene;
    scene.camera().position({0., 0.});

    const auto make_draw_unit_insert_modification =
        [](const size_t id, const int16_t z_index,
           const kaacore::Shape& shape) {
            kaacore::DrawBucketKey dbk;
            dbk.views = kaacore::ViewIndexSet{std::unordered_set<int16_t>{0}};
            dbk.z_index = z_index;
            dbk.root_distance = 0;
            dbk.texture_raw_ptr = nullptr;
            dbk.material_raw_ptr =
                kaacore::get_engine()->renderer->default_material.res_ptr.get();
            ;
            dbk.state_flags = 0;
            dbk.stencil_flags = 0;

            kaacore::DrawUnitModification du_mod;
            du_mod.type = kaacore::DrawUnitModification::Type::insert;
            du_mod.id = id;
            du_mod.lookup_key = dbk;
            du_mod.updated_vertices_indices = true;
            du_mod.state_update.vertices = shape.vertices;
            du_mod.state_update.indices = shape.indices;

            return du_mod;
        };

    const auto test_shape1 = kaacore::Shape::Circle(7.5, {0., 0.});
    const auto test_shape2 = kaacore::Shape::Box({10., 15.});

    const auto tr1 = kaacore::Transformation::translate({-10., 5.});
    const auto tr2 = kaacore::Transformation::translate({-1., 25.});
    const auto tr3 = kaacore::Transformation::translate({20., -15.});
    const auto tr4 = kaacore::Transformation::scale({0.5, 0.5});
    const auto tr5 = kaacore::Transformation::scale({2., 2.});
    const auto tr6 = kaacore::Transformation::rotate(1.5);
    const auto tr7 = kaacore::Transformation::rotate(2.5);

    kaacore::DrawQueue draw_queue;

    draw_queue.enqueue_modification(
        make_draw_unit_insert_modification(0, 0, test_shape1.transform(tr1)));
    draw_queue.enqueue_modification(
        make_draw_unit_insert_modification(1, 0, test_shape1.transform(tr2)));
    draw_queue.enqueue_modification(
        make_draw_unit_insert_modification(2, 0, test_shape1.transform(tr3)));
    draw_queue.enqueue_modification(
        make_draw_unit_insert_modification(3, 1, test_shape1.transform(tr4)));
    draw_queue.enqueue_modification(
        make_draw_unit_insert_modification(4, 1, test_shape1.transform(tr5)));
    draw_queue.enqueue_modification(
        make_draw_unit_insert_modification(5, 1, test_shape1.transform(tr7)));

    draw_queue.process_modifications();

    scene.update_function = [&](auto dt) {
        engine->renderer->render_draw_queue(draw_queue);
    };
    scene.run_on_engine(500);
}

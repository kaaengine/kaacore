#include <cstdlib>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/scenes.h"
#include "kaacore/shapes.h"
#include "kaacore/stencil.h"

namespace kaacore {
namespace demos {

struct StencilDemoScene : Scene {
    Shape default_shape;
    glm::dvec4 color_1;
    glm::dvec4 color_2;

    StencilDemoScene()
        : default_shape(Shape::Box({45., 45.})), color_1({0., 1., 1., 1.}),
          color_2({1., 1., 0., 1.})
    {
        this->camera().position({0., 0.});

        this->create_pair({-350, -350}, StencilMode(), StencilMode());
        this->create_pair(
            {-250, -350},
            StencilMode(
                1u, 255u, StencilTest::equal, StencilOp::replace,
                StencilOp::keep, StencilOp::keep
            ),
            StencilMode(
                1u, 255u, StencilTest::equal, StencilOp::keep, StencilOp::keep,
                StencilOp::keep
            )
        );
        this->create_pair(
            {-150, -350},
            StencilMode(
                1u, 255u, StencilTest::always, StencilOp::keep, StencilOp::keep,
                StencilOp::replace
            ),
            StencilMode(
                1u, 255u, StencilTest::equal, StencilOp::keep, StencilOp::keep,
                StencilOp::keep
            )
        );
        this->create_pair(
            {-350, -250},
            StencilMode(
                25u, 255u, StencilTest::greater_equal, StencilOp::replace,
                StencilOp::replace, StencilOp::replace
            ),
            StencilMode(
                25u, 255u, StencilTest::greater_equal, StencilOp::keep,
                StencilOp::keep, StencilOp::keep
            )
        );
        this->create_pair(
            {-250, -250},
            StencilMode(
                50u, 255u, StencilTest::always, StencilOp::replace,
                StencilOp::replace, StencilOp::replace
            ),
            StencilMode(
                50u, 255u, StencilTest::not_equal, StencilOp::keep,
                StencilOp::keep, StencilOp::keep
            )
        );
    }

    void create_pair(
        const glm::dvec2 slot_position, const StencilMode parent_stencil,
        const StencilMode child_stencil
    )
    {
        auto parent_node = make_node();
        parent_node->position(slot_position + glm::dvec2{-20., -20.});
        parent_node->shape(this->default_shape);
        parent_node->color(this->color_1);
        parent_node->stencil_mode(parent_stencil);

        auto child_node = make_node();
        child_node->position(glm::dvec2{20., 20.});
        child_node->shape(this->default_shape);
        child_node->color(this->color_2);
        child_node->stencil_mode(child_stencil);

        parent_node->add_child(child_node);
        this->root_node.add_child(parent_node);
    }

    void update(const kaacore::Duration dt) override
    {
        for (auto const& event : this->get_events()) {
            auto keyboard_key = event.keyboard_key();
            if (keyboard_key and keyboard_key->key() == kaacore::Keycode::q) {
                kaacore::get_engine()->quit();
                break;
            }
        }
    }
};

} // namespace demos
} // namespace kaacore

extern "C" int
main(int argc, char* argv[])
{
    kaacore::Engine eng({800, 800});
    kaacore::demos::StencilDemoScene scene;
    eng.run(&scene);

    return 0;
}

#include <iostream>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/input.h"
#include "kaacore/log.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"

using namespace kaacore;

struct DemoScene : Scene {
    NodePtr background;
    NodePtr node1;
    NodePtr node2;
    NodePtr container_node;
    Shape specific_shape;
    Shape polygon_shape;

    DemoScene()
    {
        const std::vector<StandardVertexData> vertices = {
            {-1., -1., 0., 0., 1., -1., -1., 0., 1., 1., 1.},
            {1., -1., 0., 1., 1., 1., -1., 1., 0., 1., 1.},
            {1., 1., 0., 1., 0., 1., 1., 1., 1., 0., 1.},
            {-1., 1., 0., 0., 0., -1., 1., 1., 1., 1., 0.}};

        const std::vector<uint16_t> indices = {0, 2, 1, 0, 3, 2};

        this->root_node.scale({50., 50.});

        this->specific_shape = Shape::Freeform(indices, vertices);
        this->polygon_shape =
            Shape::Polygon({{0, 1.5}, {-1, 1}, {-1, -1}, {1, -1}, {1, 1}});

        auto background = make_node();
        background->shape(Shape::Box({1e4, 1e4}));
        background->color({0.5, 0.5, 0.5, 0.25});
        background->z_index(-100);

        this->background = this->root_node.add_child(background);

        auto node1 = make_node();
        node1->position({3., 3.});
        node1->rotation(1.);
        node1->scale({
            1.,
            3.,
        });
        node1->color({1., 0., 0., 1});
        node1->shape(Shape::Box({2., 1.}));
        node1->z_index(10);
        node1->recalculate_model_matrix();
        node1->recalculate_render_data();

        this->node1 = this->root_node.add_child(node1);

        auto node2 = make_node();
        node2->position({-3., 3.});
        node2->rotation(10.);
        node2->scale({1., 1.});
        node2->color({0., 1., 0., 1});
        node2->shape(Shape::Segment({-5., -5.}, {2., 2.}));
        node2->z_index(10);
        node2->recalculate_model_matrix();
        node2->recalculate_render_data();
        this->node2 = this->root_node.add_child(node2);

        std::vector<glm::dvec2> positions = {{-2., -2.}, {0., -2.}, {2., -2.},
                                             {-2., 0.},  {0., 0.},  {2., 0.},
                                             {-2., 2.},  {0., 2.},  {2., 2.}};

        auto container_node = make_node();
        container_node->position({0., 0.});
        container_node->shape(Shape::Box({9., 9.}));
        container_node->recalculate_model_matrix();
        container_node->recalculate_render_data();

        for (const auto& p : positions) {
            auto inner_node = make_node();
            inner_node->position(p);
            inner_node->color({0., 0., 1., 1});
            inner_node->scale({0.5, 0.5});
            inner_node->shape(this->polygon_shape);
            if (p.x != 0. and p.y != 0.) {
                inner_node->z_index(10);
            } else {
                inner_node->z_index(-10);
            }

            container_node->add_child(inner_node);
        }

        this->container_node = this->root_node.add_child(container_node);
    }

    void update(const Seconds dt) override
    {
        KAACORE_APP_LOG_DEBUG("DemoScene update {}s.", dt.count());
        auto texture = get_engine()->renderer->default_texture;

        for (auto const& event : this->get_events()) {
            if (auto keyboard_key = event.keyboard_key()) {
                if (keyboard_key->key() == Keycode::q) {
                    get_engine()->quit();
                    break;
                } else if (keyboard_key->key() == Keycode::w) {
                    this->camera().position(
                        this->camera().position() + glm::dvec2(0., -0.05));
                } else if (keyboard_key->key() == Keycode::a) {
                    this->camera().position(
                        this->camera().position() + glm::dvec2(-0.05, 0.));
                } else if (keyboard_key->key() == Keycode::s) {
                    this->camera().position(
                        this->camera().position() + glm::dvec2(0., 0.05));
                } else if (keyboard_key->key() == Keycode::d) {
                    this->camera().position(
                        this->camera().position() + glm::dvec2(0.05, 0.));
                } else if (keyboard_key->key() == Keycode::i) {
                    this->camera().scale(
                        this->camera().scale() + glm::dvec2(0.1, 0.1));
                } else if (keyboard_key->key() == Keycode::o) {
                    this->camera().scale(
                        this->camera().scale() - glm::dvec2(0.1, 0.1));
                } else if (keyboard_key->key() == Keycode::r) {
                    this->camera().rotation(this->camera().rotation() + 0.2);
                } else if (keyboard_key->key() == Keycode::m) {
                    this->node1->rotation(this->node1->rotation() + 0.2);
                    this->node1->position(
                        this->node1->position() + glm::dvec2(1., 0.));
                    KAACORE_APP_LOG_INFO(
                        "Node position: {} {}", this->node1->position().x,
                        this->node1->position().y);
                } else if (keyboard_key->key() == Keycode::n) {
                    this->root_node.position(
                        this->root_node.position() + glm::dvec2(-1., -2.));
                    KAACORE_APP_LOG_INFO(
                        "World position: {} {}", this->root_node.position().x,
                        this->root_node.position().y);
                } else if (keyboard_key->key() == Keycode::c) {
                    this->camera().position(this->node1->absolute_position());
                    KAACORE_APP_LOG_INFO(
                        "Camera position: {} {}", this->camera().position().x,
                        this->camera().position().y);
                } else if (keyboard_key->key() == Keycode::f) {
                    get_engine()->window->fullscreen(
                        !get_engine()->window->fullscreen());
                } else if (keyboard_key->key() == Keycode::g) {
                    auto size = get_engine()->window->size();
                    KAACORE_APP_LOG_INFO(
                        "Current size: {} x {}", size.x, size.y);
                }
            }
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    Engine eng{{800, 600}, VirtualResolutionMode::no_stretch};
    DemoScene scene;
    eng.run(&scene);

    return 0;
}

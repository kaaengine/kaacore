#include <cstdlib>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/geometry.h"
#include "kaacore/log.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"

using namespace kaacore;

struct PolygonTesterDemoScene : Scene {
    std::vector<glm::dvec2> points;
    NodeOwnerPtr shape_repr;
    Engine* engine;

    PolygonTesterDemoScene()
    {
        this->engine = get_engine();
        this->camera.position = {0., 0.};

        this->shape_repr = make_node();
        this->shape_repr->position({0, 0});
        this->shape_repr->shape(Shape::Box({3, 3}));
        this->root_node.add_child(this->shape_repr);
    }

    void add_point(const glm::dvec2 p)
    {
        if (not this->points.empty() and this->points.back() == p) {
            return;
        }
        NodeOwnerPtr point_node = make_node();
        point_node->position(p);
        point_node->shape(Shape::Circle(1.));
        this->root_node.add_child(point_node);

        if (this->points.size()) {
            this->add_segment(p, this->points.back());
        }
        this->points.push_back(p);
    }

    void add_segment(const glm::dvec2 a, const glm::dvec2 b)
    {
        NodeOwnerPtr segment_node = make_node();
        segment_node->position(a);
        segment_node->shape(Shape::Segment({0, 0}, b - a));
        this->root_node.add_child(segment_node);
    }

    void finalize_polygon()
    {
        this->add_segment(this->points.back(), this->points.front());
        auto center = find_points_center(this->points);
        for (auto& pt : this->points) {
            pt -= center;
        }
        auto polygon_type = classify_polygon(this->points);
        log("Polygon type: %d", static_cast<int>(polygon_type));
        if (polygon_type != PolygonType::not_convex) {
            this->shape_repr->shape(Shape::Polygon(this->points));
        } else {
            log<LogLevel::error>("Polygon not convex!");
        }
        this->points.clear();
    }

    void update(uint32_t dt) override
    {
        for (auto const& event : this->get_events()) {
            if (auto mouse_button = event.mouse_button()) {
                auto pos = mouse_button->position();
                if (mouse_button->button() == MouseButton::left) {
                    pos = this->camera.unproject_position(pos);
                    log("Adding point: (%lf, %lf)", pos.x, pos.y);
                    this->add_point(pos);
                } else if (mouse_button->button() == MouseButton::left) {
                    pos = this->camera.unproject_position(pos);
                    log("Adding point: (%lf, %lf)", pos.x, pos.y);
                    this->add_point(pos);
                }
            }

            if (auto keyboard_key = event.keyboard_key()) {
                if (keyboard_key->key() == Keycode::q) {
                    get_engine()->quit();
                    break;
                } else if (keyboard_key->key() == Keycode::f) {
                    log("Finalizing polygon");
                    this->finalize_polygon();
                } else if (keyboard_key->key() == Keycode::w) {
                    this->camera.position += glm::dvec2(0., -2.5);
                } else if (keyboard_key->key() == Keycode::a) {
                    this->camera.position += glm::dvec2(-2.5, 0.);
                } else if (keyboard_key->key() == Keycode::s) {
                    this->camera.position += glm::dvec2(0., 2.5);
                } else if (keyboard_key->key() == Keycode::d) {
                    this->camera.position += glm::dvec2(2.5, 0.);
                } else if (keyboard_key->key() == Keycode::i) {
                    this->camera.scale += glm::dvec2(0.1, 0.1);
                } else if (keyboard_key->key() == Keycode::o) {
                    this->camera.scale -= glm::dvec2(0.1, 0.1);
                } else if (keyboard_key->key() == Keycode::r) {
                    this->camera.rotation += 0.3;
                }
            }
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    Engine eng({800, 600}, VirtualResolutionMode::aggresive_stretch);
    eng.window->show();
    PolygonTesterDemoScene scene;
    eng.run(&scene);

    return 0;
}

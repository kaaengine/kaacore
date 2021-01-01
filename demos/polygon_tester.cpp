#include <cstdlib>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/geometry.h"
#include "kaacore/log.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"

struct PolygonTesterDemoScene : kaacore::Scene {
    std::vector<glm::dvec2> points;
    kaacore::NodePtr shape_repr;
    kaacore::Engine* engine;

    PolygonTesterDemoScene()
    {
        this->engine = kaacore::get_engine();
        this->camera().position({0., 0.});

        auto shape_repr = kaacore::make_node();
        shape_repr->position({0, 0});
        shape_repr->shape(kaacore::Shape::Box({3, 3}));
        this->shape_repr = this->root_node.add_child(shape_repr);
    }

    void add_point(const glm::dvec2 p)
    {
        if (not this->points.empty() and this->points.back() == p) {
            return;
        }
        kaacore::NodeOwnerPtr point_node = kaacore::make_node();
        point_node->position(p);
        point_node->shape(kaacore::Shape::Circle(1.));
        this->root_node.add_child(point_node);

        if (this->points.size()) {
            this->add_segment(p, this->points.back());
        }
        this->points.push_back(p);
    }

    void add_segment(const glm::dvec2 a, const glm::dvec2 b)
    {
        kaacore::NodeOwnerPtr segment_node = kaacore::make_node();
        segment_node->position(a);
        segment_node->shape(kaacore::Shape::Segment({0, 0}, b - a));
        this->root_node.add_child(segment_node);
    }

    void finalize_polygon()
    {
        this->add_segment(this->points.back(), this->points.front());
        auto center = kaacore::find_points_center(this->points);
        for (auto& pt : this->points) {
            pt -= center;
        }
        auto polygon_type = kaacore::classify_polygon(this->points);
        KAACORE_APP_LOG_INFO(
            "Polygon type: {}", static_cast<int>(polygon_type));
        if (polygon_type != kaacore::PolygonType::not_convex) {
            this->shape_repr->shape(kaacore::Shape::Polygon(this->points));
        } else {
            KAACORE_APP_LOG_ERROR("Polygon not convex!");
        }
        this->points.clear();
    }

    void update(const kaacore::Duration dt) override
    {
        for (auto const& event : this->get_events()) {
            if (auto mouse_button = event.mouse_button()) {
                auto pos = mouse_button->position();
                if (mouse_button->button() == kaacore::MouseButton::left) {
                    pos = this->camera().unproject_position(pos);
                    KAACORE_APP_LOG_INFO(
                        "Adding point: ({}, {})", pos.x, pos.y);
                    this->add_point(pos);
                } else if (
                    mouse_button->button() == kaacore::MouseButton::left) {
                    pos = this->camera().unproject_position(pos);
                    KAACORE_APP_LOG_INFO(
                        "Adding point: ({}, {})", pos.x, pos.y);
                    this->add_point(pos);
                }
            }

            if (auto keyboard_key = event.keyboard_key()) {
                if (keyboard_key->key() == kaacore::Keycode::q) {
                    kaacore::get_engine()->quit();
                    break;
                } else if (keyboard_key->key() == kaacore::Keycode::f) {
                    KAACORE_APP_LOG_INFO("Finalizing polygon");
                    this->finalize_polygon();
                } else if (keyboard_key->key() == kaacore::Keycode::w) {
                    this->camera().position(
                        this->camera().position() + glm::dvec2(0., -2.5));
                } else if (keyboard_key->key() == kaacore::Keycode::a) {
                    this->camera().position(
                        this->camera().position() + glm::dvec2(-2.5, 0.));
                } else if (keyboard_key->key() == kaacore::Keycode::s) {
                    this->camera().position(
                        this->camera().position() + glm::dvec2(0., 2.5));
                } else if (keyboard_key->key() == kaacore::Keycode::d) {
                    this->camera().position(
                        this->camera().position() + glm::dvec2(2.5, 0.));
                } else if (keyboard_key->key() == kaacore::Keycode::i) {
                    this->camera().scale(
                        this->camera().scale() + glm::dvec2(0.1, 0.1));
                } else if (keyboard_key->key() == kaacore::Keycode::o) {
                    this->camera().scale(
                        this->camera().scale() - glm::dvec2(0.1, 0.1));
                } else if (keyboard_key->key() == kaacore::Keycode::r) {
                    this->camera().rotation(this->camera().rotation() + 0.3);
                }
            }
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    kaacore::Engine eng(
        {800, 600}, kaacore::VirtualResolutionMode::aggresive_stretch);
    PolygonTesterDemoScene scene;
    eng.run(&scene);

    return 0;
}

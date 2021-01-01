#include <cstdlib>
#include <iostream>
#include <vector>

#include "kaacore/clock.h"
#include "kaacore/engine.h"
#include "kaacore/geometry.h"
#include "kaacore/log.h"
#include "kaacore/node_transitions.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"
#include "kaacore/transitions.h"

using namespace kaacore;

struct SpatialIndexingDemoScene : Scene {
    NodePtr stats_text_node;
    NodePtr shapes_tree;

    SpatialIndexingDemoScene()
    {
        this->camera().position({0., 0.});

        const auto movement_transition =
            make_node_transition<NodePositionTransition>(
                glm::dvec2(100, 30), AttributeTransitionMethod::add, 10.s,
                TransitionWarping(0, true));
        const auto scaling_transition =
            make_node_transition<NodeScaleTransition>(
                glm::dvec2(3.0, 3.5), AttributeTransitionMethod::set, 13.s,
                TransitionWarping(0, true));

        const std::vector<Shape> shapes{Shape::Circle(3.5),
                                        Shape::Box({4., 6.}),
                                        Shape::Segment({-4., 1.}, {1., 2.})};

        auto stats_text_node = make_node(NodeType::text);
        stats_text_node->text.content("");
        stats_text_node->text.font_size(8.);
        stats_text_node->position({-48., -48});
        stats_text_node->z_index(10);
        stats_text_node->origin_alignment(Alignment::top_left);
        this->stats_text_node = this->root_node.add_child(stats_text_node);

        auto shapes_tree = make_node();
        shapes_tree->scale({0.5, 0.5});
        shapes_tree->transition(scaling_transition);
        this->shapes_tree = this->root_node.add_child(shapes_tree);

        for (int i = -20; i <= 20; i++) {
            for (int j = -20; j <= 20; j++) {
                NodeOwnerPtr node = make_node();
                node->shape(shapes[(i + j) % shapes.size()]);
                node->position({10. * i, 10. * j});
                node->color({0.5, 0.5, 0.5, 1.0});
                node->transition(movement_transition);
                this->shapes_tree->add_child(node);
            }
        }
    }

    void update(const Seconds dt) override
    {
        for (auto const& event : this->get_events()) {
            auto keyboard_key = event.keyboard_key();
            if (keyboard_key and keyboard_key->key() == Keycode::q) {
                get_engine()->quit();
                break;
            } else if (keyboard_key and keyboard_key->key() == Keycode::r) {
                KAACORE_APP_LOG_INFO("Resetting non-indexable nodes...");

                int count = 0;
                for (auto node : this->shapes_tree->children()) {
                    if (not node->indexable()) {
                        node->indexable(true);
                        node->color({0.5, 0.5, 1., 1.});
                        count++;
                    }
                }
                KAACORE_APP_LOG_INFO("Reset nodes: %d", count);
            }

            if (auto mouse_button = event.mouse_button()) {
                auto pos = mouse_button->position();
                if (mouse_button->is_button_down() and
                    mouse_button->button() == MouseButton::left) {
                    pos = this->camera().unproject_position(pos);
                    auto query_results = this->spatial_index.query_point(pos);
                    KAACORE_APP_LOG_INFO(
                        "Clicked %ld nodes", query_results.size());
                    for (auto& node : query_results) {
                        node->color({1., 0., 0., 1.});
                    }
                } else if (
                    mouse_button->is_button_down() and
                    mouse_button->button() == MouseButton::right) {
                    pos = this->camera().unproject_position(pos);
                    auto query_results = this->spatial_index.query_point(pos);
                    KAACORE_APP_LOG_INFO(
                        "Clicked %ld nodes", query_results.size());
                    for (auto& node : query_results) {
                        node->color({0., 0.5, 1., 1.});
                        node->indexable(false);
                    }
                }
            }
        }

        auto query_results = this->spatial_index.query_bounding_box(
            BoundingBox{-50., -50., 50., 50.}, true);
        auto query_results_for_drawing =
            this->spatial_index.query_bounding_box_for_drawing(
                BoundingBox{-50., -50., 50., 50.});
        this->stats_text_node->text.content(
            "Nodes visible (normal/drawing):\n" +
            std::to_string(query_results.size()) + "/" +
            std::to_string(query_results_for_drawing.size()));
    }
};

extern "C" int
main(int argc, char* argv[])
{
    Engine eng({100, 100});
    eng.window->size({800, 600});
    eng.window->center();
    SpatialIndexingDemoScene scene;
    eng.run(&scene);

    return 0;
}

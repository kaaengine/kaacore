#include <cstdlib>
#include <iostream>
#include <vector>

#include "kaacore/engine.h"
#include "kaacore/geometry.h"
#include "kaacore/node_transitions.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"
#include "kaacore/transitions.h"

using namespace kaacore;

struct SpatialIndexingDemoScene : Scene {
    NodeOwnerPtr stats_text_node;

    SpatialIndexingDemoScene()
    {
        this->camera().position({0., 0.});

        const auto movement_transition =
            make_node_transition<NodePositionTransition>(
                glm::dvec2(100, 30), AttributeTransitionMethod::add, 10000.,
                TransitionWarping(0, true));
        const auto scaling_transition =
            make_node_transition<NodeScaleTransition>(
                glm::dvec2(3.0, 3.5), AttributeTransitionMethod::set, 13000.,
                TransitionWarping(0, true));

        const std::vector<Shape> shapes{Shape::Circle(3.5),
                                        Shape::Box({4., 6.}),
                                        Shape::Segment({-4., 1.}, {1., 2.})};

        this->stats_text_node = make_node(NodeType::text);
        this->stats_text_node->text.content("");
        this->stats_text_node->text.font_size(8.);
        this->stats_text_node->position({-48., -48});
        this->stats_text_node->z_index(10);
        this->stats_text_node->origin_alignment(Alignment::top_left);
        this->root_node.add_child(this->stats_text_node);

        auto shapes_tree = make_node();
        shapes_tree->scale({0.5, 0.5});
        shapes_tree->transition(scaling_transition);
        this->root_node.add_child(shapes_tree);

        for (int i = -20; i <= 20; i++) {
            for (int j = -20; j <= 20; j++) {
                NodeOwnerPtr node = make_node();

                node->shape(shapes[(i + j) % shapes.size()]);
                node->position({10. * i, 10. * j});
                node->color({0.5, 0.5, 0.5, 1.0});
                node->transition(movement_transition);
                shapes_tree->add_child(node);
            }
        }
    }

    void update(uint32_t dt) override
    {
        for (auto const& event : this->get_events()) {
            auto keyboard_key = event.keyboard_key();
            if (keyboard_key and keyboard_key->key() == Keycode::q) {
                get_engine()->quit();
                break;
            }

            if (auto mouse_button = event.mouse_button()) {
                auto pos = mouse_button->position();
                if (mouse_button->is_button_down() and
                    mouse_button->button() == MouseButton::left) {
                    pos = this->camera().unproject_position(pos);
                    auto query_results = this->spatial_index.query_point(pos);
                    log<LogLevel::info, LogCategory::application>(
                        "Clicked %ld nodes", query_results.size());
                    for (auto& node : query_results) {
                        node->color({1., 0., 0., 1.});
                    }
                }
            }
        }

        auto query_results = this->spatial_index.query_bounding_box(
            BoundingBox{-50., -50., 50., 50.});
        this->stats_text_node->text.content(
            "Nodes visible: " + std::to_string(query_results.size()));
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

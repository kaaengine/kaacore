#include <cstdlib>
#include <iostream>
#include <vector>

#include "kaacore/clock.h"
#include "kaacore/engine.h"
#include "kaacore/node_transitions.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"
#include "kaacore/transitions.h"

using namespace std::chrono_literals;

struct TransitionsDemoScene : kaacore::Scene {
    std::vector<kaacore::NodePtr> objects;

    TransitionsDemoScene()
    {
        this->camera().position({0., 0.});

        const kaacore::Duration tr_time = 2.s;
        const std::vector<kaacore::Shape> shapes = {
            kaacore::Shape::Circle(2.5), kaacore::Shape::Box({4., 4.}),
            kaacore::Shape::Polygon({{-2., -2.}, {2., -2.}, {-2., 2.}})};
        const std::vector<double> rotations = {M_PI, M_PI / 4., 3 * M_PI / 2.};

        const auto movement_transitions_par =
            kaacore::make_node_transitions_parallel(
                {kaacore::make_node_transitions_sequence(
                     {
                         kaacore::make_node_transition<
                             kaacore::NodePositionSteppingTransition>(
                             std::vector<glm::dvec2>{
                                 {-3., -3.}, {3., -3.}, {3., 3.}, {-3., 3.}},
                             kaacore::AttributeTransitionMethod::add,
                             tr_time * 2),
                         kaacore::make_node_transition<
                             kaacore::NodePositionTransition>(
                             glm::dvec2(-15., -15.),
                             kaacore::AttributeTransitionMethod::add, tr_time,
                             kaacore::TransitionWarping{},
                             kaacore::Easing::back_in_out),
                         kaacore::make_node_transition<
                             kaacore::NodeColorTransition>(
                             glm::dvec4(0., 1., 0., 1.),
                             kaacore::AttributeTransitionMethod::set, 0.s,
                             kaacore::TransitionWarping{}),
                         kaacore::make_node_transition<
                             kaacore::NodePositionTransition>(
                             glm::dvec2(-25., 0.),
                             kaacore::AttributeTransitionMethod::add,
                             tr_time * 4, kaacore::TransitionWarping{},
                             kaacore::Easing::sine_in_out),
                         kaacore::make_node_transition<
                             kaacore::NodeScaleTransition>(
                             glm::dvec2(2., 2.),
                             kaacore::AttributeTransitionMethod::multiply,
                             tr_time * 2, kaacore::TransitionWarping{},
                             kaacore::Easing::quintic_in_out),
                         kaacore::make_node_transitions_parallel({
                             kaacore::make_node_transition<
                                 kaacore::NodeScaleTransition>(
                                 glm::dvec2(2., 2.),
                                 kaacore::AttributeTransitionMethod::multiply,
                                 tr_time * 5),
                             kaacore::make_node_transition<
                                 kaacore::NodeColorTransition>(
                                 glm::dvec4(1., 0.2, 0.2, 0.5), tr_time * 5,
                                 kaacore::TransitionWarping{},
                                 kaacore::Easing::elastic_in_out),
                         }),
                         kaacore::make_node_transitions_parallel({
                             kaacore::make_node_transition<
                                 kaacore::NodePositionTransition>(
                                 glm::dvec2(0., 0.),
                                 kaacore::AttributeTransitionMethod::set,
                                 tr_time * 6),
                             kaacore::make_node_transition<
                                 kaacore::NodeScaleTransition>(
                                 glm::dvec2(0.3, 0.3),
                                 kaacore::AttributeTransitionMethod::multiply,
                                 tr_time * 3),
                         }),
                     },
                     kaacore::TransitionWarping(1, true)),
                 kaacore::make_node_transition<
                     kaacore::NodeShapeSteppingTransition>(
                     shapes, tr_time * 2.4,
                     kaacore::TransitionWarping(12, false)),
                 kaacore::make_node_transition<
                     kaacore::NodeRotationSteppingTransition>(
                     rotations, tr_time * 2.4,
                     kaacore::TransitionWarping(12, false))},
                kaacore::TransitionWarping(0, true));

        for (int i = 0; i < 625; i++) {
            kaacore::NodeOwnerPtr node = kaacore::make_node();

            node->shape(kaacore::Shape::Circle(2.5));
            node->position({-90. + (i / 25) * 10, -90. + (i % 25) * 10});
            node->color({1., 1., 1., 1.});
            node->transition(movement_transitions_par);

            this->objects.push_back(this->root_node.add_child(node));
        }
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

extern "C" int
main(int argc, char* argv[])
{
    kaacore::Engine eng({100, 100});
    eng.window->size({800, 600});
    eng.window->center();
    TransitionsDemoScene scene;
    eng.run(&scene);

    return 0;
}

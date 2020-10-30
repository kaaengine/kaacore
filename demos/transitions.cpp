#include <cstdlib>
#include <iostream>
#include <vector>

#include "kaacore/engine.h"
#include "kaacore/node_transitions.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"
#include "kaacore/transitions.h"

using namespace kaacore;

struct TransitionsDemoScene : Scene {
    std::vector<NodeOwnerPtr> objects;

    TransitionsDemoScene()
    {
        this->camera().position({0., 0.});

        const double tr_time = 1500.;
        const std::vector<Shape> shapes = {
            Shape::Circle(2.5), Shape::Box({4., 4.}),
            Shape::Polygon({{-2., -2.}, {2., -2.}, {-2., 2.}})};
        const std::vector<double> rotations = {M_PI, M_PI / 4., 3 * M_PI / 2.};

        const auto movement_transitions_par = make_node_transitions_parallel(
            {make_node_transitions_sequence(
                 {
                     make_node_transition<NodePositionSteppingTransition>(
                         std::vector<glm::dvec2>{
                             {-3., -3.}, {3., -3.}, {3., 3.}, {-3., 3.}},
                         AttributeTransitionMethod::add, tr_time * 2),
                     make_node_transition<NodePositionTransition>(
                         glm::dvec2(-15., -15.), AttributeTransitionMethod::add,
                         tr_time, TransitionWarping{}, Easing::back_in_out),
                     make_node_transition<NodeColorTransition>(
                         glm::dvec4(0., 1., 0., 1.),
                         AttributeTransitionMethod::set, 0.,
                         TransitionWarping{}),
                     make_node_transition<NodePositionTransition>(
                         glm::dvec2(-25., 0.), AttributeTransitionMethod::add,
                         tr_time * 4, TransitionWarping{}, Easing::sine_in_out),
                     make_node_transition<NodeScaleTransition>(
                         glm::dvec2(2., 2.),
                         AttributeTransitionMethod::multiply, tr_time * 2,
                         TransitionWarping{}, Easing::quintic_in_out),
                     make_node_transitions_parallel({
                         make_node_transition<NodeScaleTransition>(
                             glm::dvec2(2., 2.),
                             AttributeTransitionMethod::multiply, tr_time * 5),
                         make_node_transition<NodeColorTransition>(
                             glm::dvec4(1., 0.2, 0.2, 0.5), tr_time * 5,
                             TransitionWarping{}, Easing::elastic_in_out),
                     }),
                     make_node_transitions_parallel({
                         make_node_transition<NodePositionTransition>(
                             glm::dvec2(0., 0.), AttributeTransitionMethod::set,
                             tr_time * 6),
                         make_node_transition<NodeScaleTransition>(
                             glm::dvec2(0.3, 0.3),
                             AttributeTransitionMethod::multiply, tr_time * 3),
                     }),
                 },
                 TransitionWarping(1, true)),
             make_node_transition<NodeShapeSteppingTransition>(
                 shapes, tr_time * 2.4, TransitionWarping(12, false)),
             make_node_transition<NodeRotationSteppingTransition>(
                 rotations, tr_time * 2.4, TransitionWarping(12, false))},
            TransitionWarping(0, true));

        for (int i = 0; i < 625; i++) {
            NodeOwnerPtr node = make_node();

            node->shape(Shape::Circle(2.5));
            node->position({-90. + (i / 25) * 10, -90. + (i % 25) * 10});
            node->color({1., 1., 1., 1.});
            node->transition(movement_transitions_par);

            this->root_node.add_child(node);
            this->objects.push_back(std::move(node));
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
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    Engine eng({100, 100});
    eng.window->size({800, 600});
    eng.window->center();
    TransitionsDemoScene scene;
    eng.run(&scene);

    return 0;
}

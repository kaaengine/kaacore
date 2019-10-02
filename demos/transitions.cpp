#include <iostream>
#include <cstdlib>
#include <vector>

#include "kaacore/engine.h"
#include "kaacore/scenes.h"
#include "kaacore/nodes.h"
#include "kaacore/transitions.h"
#include "kaacore/node_transitions.h" 

using namespace kaacore;


struct TransitionsDemoScene : Scene {
    std::vector<Node*> objects;

    TransitionsDemoScene()
    {
        this->camera.position = {0., 0.};

        const double tr_time = 1500.;

        const auto movement_transitions_par = make_node_transitions_parallel({
            make_node_transitions_sequence({
                make_node_transition<NodePositionTransition>(glm::dvec2(-15., -15.), AttributeTransitionMethod::add, tr_time),
                make_node_transition<NodePositionTransition>(glm::dvec2(-25., 0.), AttributeTransitionMethod::add, tr_time * 4),
                make_node_transition<NodeScaleTransition>(glm::dvec2(2., 2.), AttributeTransitionMethod::multiply, tr_time * 2),
                make_node_transitions_parallel({
                    make_node_transition<NodeScaleTransition>(glm::dvec2(2., 2.), AttributeTransitionMethod::multiply, tr_time * 5),
                    make_node_transition<NodeColorTransition>(glm::dvec4(1., 0.2, 0.2, 0.5), tr_time * 5),
                }),
                make_node_transitions_parallel({
                    make_node_transition<NodePositionTransition>(glm::dvec2(0., 0.), AttributeTransitionMethod::set, tr_time * 6),
                    make_node_transition<NodeScaleTransition>(glm::dvec2(0.3, 0.3), AttributeTransitionMethod::multiply, tr_time * 3),
                }),
            }, TransitionWarping(1, true)),
        }, TransitionWarping(0, true));

        for (int i = 0; i < 625; i++) {
            Node* node = new Node();

            node->shape(Shape::Circle(2.5));
            node->position({
                -90. + (i / 25) * 10,
                -90. + (i % 25) * 10
            });
            node->color({1., 1., 1., 1.});
            node->transition(movement_transitions_par);

            this->objects.push_back(node);
            this->root_node.add_child(node);
        }
    }

    void update(uint32_t dt) override
    {
        for (auto const& event : this->get_events()) {
            if (event.is_pressing(Keycode::q) or event.is_quit()) {
                get_engine()->quit();
                break;
            }
        }
    }
};


extern "C" int main(int argc, char *argv[])
{
    Engine eng({100, 100});
    eng.window->show();
    TransitionsDemoScene scene;
    eng.run(&scene);

    return 0;
}

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

        this->root_node.transition(
            make_node_transition<NodeScaleTransition>(glm::dvec2(-0.8, -0.8), 20000.)
        );

        const double tr_time = 1500.;

        auto movement_transitions = make_node_transitions_sequence({
            make_node_transition<NodePositionTransition>(glm::dvec2(30., 0.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(15., 15.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(0., 30.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(-15., -15.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(-30., 0.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(15., 15.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(0., -30.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(-15., -15.), tr_time),
            make_node_transition<NodeScaleTransition>(glm::dvec2(0.85, 0.85), tr_time * 2),
            make_node_transition<NodePositionTransition>(glm::dvec2(30., 0.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(15., 15.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(0., 30.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(-15., -15.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(-30., 0.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(15., 15.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(0., -30.), tr_time),
            make_node_transition<NodePositionTransition>(glm::dvec2(-15., -15.), tr_time),
            make_node_transition<NodeScaleTransition>(glm::dvec2(-0.85, -0.85), tr_time * 2),
        });

        for (int i = 0; i < 625; i++) {
            Node* node = new Node();

            // node->shape(Shape::Circle(2.5));
            node->shape(Shape::Box({5., 5.}));
            node->position({
                -120. + (i / 25) * 10,
                -120. + (i % 25) * 10
            });
            node->color({0.1, 0.1, 0.1, 1.});
            node->transition(
                make_node_transitions_parallel({
                   movement_transitions,
                   make_node_transition<NodeColorTransition>(glm::dvec4(0.9, 0.9, 0.9, 0.),
                                                             40000 - i * 50)
                })
            );

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

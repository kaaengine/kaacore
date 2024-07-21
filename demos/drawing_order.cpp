#include <iostream>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "kaacore/clock.h"
#include "kaacore/engine.h"
#include "kaacore/input.h"
#include "kaacore/log.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"

struct DemoScene : kaacore::Scene {
    DemoScene()
    {
        const std::vector<glm::dvec4> colors = {
            {1., 0., 0., 1.},  {0., 1., 0., 1.},  {0., 0., 1., 1.},
            {1., 0., 0., 0.7}, {0., 1., 0., 0.7}, {0., 0., 1., 0.7}
        };

        auto background_node = kaacore::make_node();
        background_node->position({50., 50.});
        background_node->shape(kaacore::Shape::Box({100., 100.}));
        background_node->z_index(5);
        background_node->color({0.7, 0.7, 0.7, 0.9});
        this->root_node.add_child(background_node);

        auto container_node = kaacore::make_node();
        container_node->position({20., 20.});
        container_node->z_index(10);
        kaacore::NodePtr parent_node =
            this->root_node.add_child(container_node);

        for (const auto& color : colors) {
            auto node = kaacore::make_node();
            node->position({10., 10.});
            node->color(color);
            node->shape(kaacore::Shape::Box({30., 30.}));
            parent_node = parent_node->add_child(node);
        }
    }

    void update(kaacore::Duration dt) override {}
};

extern "C" int
main(int argc, char* argv[])
{
    kaacore::Engine eng{{100, 100}};
    DemoScene scene;
    eng.run(&scene);

    return 0;
}

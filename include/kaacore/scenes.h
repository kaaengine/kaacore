#pragma once

#include <vector>
#include <set>

#include <glm/glm.hpp>

#include "kaacore/nodes.h"
#include "kaacore/physics.h"
#include "kaacore/input.h"


namespace kaacore {

struct Camera {
    glm::dvec2 position = {0., 0.,};
    double rotation = 0.;
    glm::dvec2 scale = {1., 1.};
    glm::dvec2 size = {10., 10.};

    glm::fmat4 calculated_view;

    Camera();
    void refresh();
};


struct Scene {
    Node root_node;
    Camera camera;

    uint64_t time = 0;

    std::set<Node*> simulations_registry;

    Scene();
    virtual ~Scene() noexcept(false);

    virtual void process_frame(uint32_t dt);
    virtual void on_enter();
    virtual void update(uint32_t dt);
    virtual void on_exit();
    virtual void process_nodes(uint32_t dt);

    void register_simulation(Node* node);
    void unregister_simulation(Node* node);

    const std::vector<Event>& get_events() const;
};

} // namespace kaacore

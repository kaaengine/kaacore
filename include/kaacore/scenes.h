#pragma once

#include <set>
#include <vector>

#include <glm/glm.hpp>

#include "kaacore/input.h"
#include "kaacore/nodes.h"
#include "kaacore/physics.h"

namespace kaacore {

struct Camera {
    glm::dvec2 position;
    double rotation = 0.;
    glm::dvec2 scale = {1., 1.};

    glm::fmat4 calculated_view;

    Camera();
    void refresh();
    glm::dvec2 unproject_position(const glm::dvec2& pos);
};

struct Scene {
    Node root_node;
    Camera camera;

    std::set<Node*> simulations_registry;

    Scene();
    virtual ~Scene();

    virtual void on_attach();
    virtual void on_enter();
    virtual void update(uint32_t dt);
    virtual void on_exit();
    virtual void on_detach();

    void process_frame(uint32_t dt);
    void process_nodes(uint32_t dt);
    void process_nodes_drawing(uint32_t dt);

    void register_simulation(Node* node);
    void unregister_simulation(Node* node);

    const std::vector<Event>& get_events() const;
};

} // namespace kaacore

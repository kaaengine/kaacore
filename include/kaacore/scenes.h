#pragma once

#include <set>
#include <vector>

#include "kaacore/camera.h"
#include "kaacore/input.h"
#include "kaacore/nodes.h"
#include "kaacore/physics.h"
#include "kaacore/views.h"

namespace kaacore {

class Engine;

class Scene {
  public:
    Node root_node;
    ViewsManager views;
    std::set<Node*> simulations_registry;

    Scene();
    virtual ~Scene();

    Camera& camera();
    void process_frame(uint32_t dt);
    void process_physics(uint32_t dt);
    void process_nodes(uint32_t dt);
    void process_nodes_drawing();
    void register_simulation(Node* node);
    void unregister_simulation(Node* node);

    virtual void on_attach();
    virtual void on_enter();
    virtual void update(uint32_t dt);
    virtual void on_exit();
    virtual void on_detach();

    const std::vector<Event>& get_events() const;

  private:
    void _refresh_views();

    friend class Engine;
};

} // namespace kaacore

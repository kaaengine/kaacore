#pragma once

#include <set>
#include <vector>

#include "kaacore/camera.h"
#include "kaacore/clock.h"
#include "kaacore/input.h"
#include "kaacore/nodes.h"
#include "kaacore/physics.h"
#include "kaacore/views.h"

namespace kaacore {

class Scene {
  public:
    Node root_node;
    ViewsManager views;
    std::set<Node*> simulations_registry;

    Scene();
    virtual ~Scene();

    Camera& camera();
    void reset_views();
    void process_frame(microseconds dt);
    void process_physics(microseconds dt);
    void process_nodes(microseconds dt);
    void process_nodes_drawing();
    void register_simulation(Node* node);
    void unregister_simulation(Node* node);
    void time_scale(double scale);
    double time_scale();

    virtual void on_attach();
    virtual void on_enter();
    virtual void update(double dt);
    virtual void on_exit();
    virtual void on_detach();

    const std::vector<Event>& get_events() const;

  private:
    double _time_scale = 1.0;
};

} // namespace kaacore

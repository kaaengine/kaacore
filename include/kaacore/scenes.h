#pragma once

#include <set>
#include <vector>

#include "kaacore/camera.h"
#include "kaacore/clock.h"
#include "kaacore/input.h"
#include "kaacore/nodes.h"
#include "kaacore/physics.h"
#include "kaacore/spatial_index.h"
#include "kaacore/timers.h"
#include "kaacore/views.h"

namespace kaacore {

class Scene {
  public:
    Node root_node;
    ViewsManager views;
    TimersManager timers;
    SpatialIndex spatial_index;
    std::set<Node*> simulations_registry;

    Scene();
    virtual ~Scene();

    void reset_views();
    void process_physics(const HighPrecisionDuration dt);
    void process_nodes(const HighPrecisionDuration dt);
    void resolve_dirty_nodes();
    void process_nodes_drawing();
    void register_simulation(Node* node);
    void unregister_simulation(Node* node);

    Camera& camera();
    double time_scale() const;
    void time_scale(const double scale);

    virtual void on_attach();
    virtual void on_enter();
    virtual void update(const Duration dt);
    virtual void on_exit();
    virtual void on_detach();

    const std::vector<Event>& get_events() const;

  private:
    double _time_scale = 1.;
};

} // namespace kaacore

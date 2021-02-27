#pragma once

#include <set>
#include <vector>

#include "kaacore/camera.h"
#include "kaacore/clock.h"
#include "kaacore/draw_queue.h"
#include "kaacore/input.h"
#include "kaacore/nodes.h"
#include "kaacore/physics.h"
#include "kaacore/spatial_index.h"
#include "kaacore/timers.h"
#include "kaacore/views.h"

namespace kaacore {

class Scene {
    using NodesQueue = std::vector<Node*>;

  public:
    Node root_node;
    ViewsManager views;
    TimersManager timers;
    SpatialIndex spatial_index;
    std::set<Node*> simulations_registry;
    DrawQueue draw_queue;

    Scene();
    virtual ~Scene();

    void reset_views();
    NodesQueue& build_processing_queue();
    void process_physics(const HighPrecisionDuration dt);
    void process_nodes(
        const HighPrecisionDuration dt, const NodesQueue& processing_queue);
    void resolve_dirty_nodes(const NodesQueue& processing_queue);
    void update_nodes_drawing_queue(const NodesQueue& processing_queue);
    void process_drawing();
    void remove_marked_nodes();
    void register_simulation(Node* node);
    void unregister_simulation(Node* node);

    void handle_add_node_to_tree(Node* node);
    void handle_remove_node_from_tree(Node* node);

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
    NodesQueue _nodes_remove_queue;
    std::atomic<uint64_t> _node_scene_tree_id_counter = 0;
};

} // namespace kaacore

#pragma once

#include <memory>
#include <set>
#include <vector>

#include <glm/glm.hpp>

#include "kaacore/camera.h"
#include "kaacore/clock.h"
#include "kaacore/draw_queue.h"
#include "kaacore/input.h"
#include "kaacore/nodes.h"
#include "kaacore/physics.h"
#include "kaacore/render_passes.h"
#include "kaacore/renderer.h"
#include "kaacore/spatial_index.h"
#include "kaacore/timers.h"
#include "kaacore/viewports.h"

namespace kaacore {

class Scene {
    using NodesQueue = std::vector<Node*>;

  public:
    Node root_node;
    RenderPassesManager render_passes;
    ViewportsManager viewports;
    TimersManager timers;
    SpatialIndex spatial_index;
    std::set<Node*> simulations_registry;
    DrawQueue draw_queue;

    Scene();
    virtual ~Scene();

    NodesQueue& build_processing_queue();
    void process_update(const Duration dt);
    void process_physics(const HighPrecisionDuration dt);
    void process_nodes(
        const HighPrecisionDuration dt, const NodesQueue& processing_queue
    );
    void resolve_spatial_index_changes(const NodesQueue& processing_queue);
    void update_nodes_drawing_queue(const NodesQueue& processing_queue);
    void draw(
        const uint16_t render_pass, const int16_t viewport,
        const DrawCall& draw_call
    );
    void attach_frame_context(const std::unique_ptr<Renderer>& renderer);
    void render(const std::unique_ptr<Renderer>& renderer);
    void remove_marked_nodes();
    void register_simulation(Node* node);
    void unregister_simulation(Node* node);

    void handle_add_node_to_tree(Node* node);
    void handle_remove_node_from_tree(Node* node);

    Camera& camera();
    Duration total_time() const;

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
    Duration _last_dt = 0s;
    Duration _total_time = 0s;
    NodesQueue _nodes_remove_queue;
    std::vector<DrawCommand> _draw_commands;
    std::atomic<uint64_t> _node_scene_tree_id_counter = 0;

    void _reset();

    friend class Engine;
    friend class Renderer;
};

} // namespace kaacore

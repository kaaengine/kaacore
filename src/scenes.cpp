#include <algorithm>
#include <cstdlib>
#include <deque>
#include <limits>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/scenes.h"
#include "kaacore/statistics.h"

namespace kaacore {

Scene::Scene() : timers(this)
{
    this->root_node._scene = this;
    this->handle_add_node_to_tree(&this->root_node);
}

Scene::~Scene()
{
    while (not this->root_node._children.empty()) {
        delete this->root_node._children[0];
    }
    KAACORE_ASSERT_TERMINATE(
        this->simulations_registry.empty(),
        "Simulation registry not empty on scene deletion.");
}

Camera&
Scene::camera()
{
    return this->viewports[default_viewport_z_index].camera;
}

std::vector<Node*>&
Scene::build_processing_queue()
{
    static std::vector<Node*> processing_queue;
    processing_queue.clear();
    KAACORE_LOG_TRACE("Building processing queue");

    processing_queue.push_back(&this->root_node);
    size_t i = 0;
    while (i < processing_queue.size()) {
        Node* node = processing_queue[i];
        for (auto child_node : node->children()) {
            processing_queue.push_back(child_node);
        }
        i++;
    }
    KAACORE_LOG_DEBUG("Nodes to process count: {}", processing_queue.size());
    return processing_queue;
}

void
Scene::process_update(const Duration dt)
{
    this->_total_time += this->_last_dt = dt;
    this->update(dt);
}

void
Scene::process_physics(const HighPrecisionDuration dt)
{
    StopwatchStatAutoPusher stopwatch{"scene.process_physics:time"};
    for (Node* space_node : this->simulations_registry) {
        space_node->space.simulate(dt);
    }
}

void
Scene::process_nodes(
    const HighPrecisionDuration dt, const Scene::NodesQueue& processing_queue)
{
    StopwatchStatAutoPusher stopwatch{"scene.process_nodes:time"};
    CounterStatAutoPusher transitions_counter{
        "scene.transitions_processed:count"};
    for (Node* node : processing_queue) {
        if (node->_marked_to_delete) {
            continue;
        }

        if (node->_lifetime > 0us) {
            if ((node->_lifetime -= std::min(dt, node->_lifetime)) == 0us) {
                KAACORE_ASSERT(not node->_marked_to_delete, "");
                node->_mark_to_delete();
                continue;
            }
        }

        if (node->_type == NodeType::body) {
            node->body.sync_simulation_position();
            node->body.sync_simulation_rotation();
        }

        if (node->_transitions_manager) {
            node->_transitions_manager.step(node, dt);
            transitions_counter += 1;
        }
    }
}

void
Scene::resolve_spatial_index_changes(const Scene::NodesQueue& processing_queue)
{
    StopwatchStatAutoPusher stopwatch{"scene.resolve_nodes:time"};
    CounterStatAutoPusher spatial_updates_counter{
        "scene.spatial_index_updates:count"};
    for (Node* node : processing_queue) {
        if (node->_marked_to_delete) {
            continue;
        }

        if (node->query_dirty_flags(Node::DIRTY_SPATIAL_INDEX)) {
            this->spatial_index.update_single(node);
            spatial_updates_counter += 1;
        }
    }
}

void
Scene::update_nodes_drawing_queue(const NodesQueue& processing_queue)
{
    KAACORE_LOG_TRACE("Starting process_nodes_drawing()");
    StopwatchStatAutoPusher stopwatch{"scene.nodes_drawing:time"};

    for (Node* node : processing_queue) {
        if (not node->_marked_to_delete) {
            auto mods_pack = node->calculate_draw_unit_updates();
            if (mods_pack) {
                KAACORE_LOG_TRACE(
                    "DrawUnit modifications detected for node: {}",
                    fmt::ptr(node));
                auto new_lookup_key = mods_pack.new_lookup_key();
                if (mods_pack.upsert_mod) {
                    // TODO enque modification should accept pack
                    this->draw_queue.enqueue_modification(
                        std::move(*mods_pack.upsert_mod));
                }
                if (mods_pack.remove_mod) {
                    this->draw_queue.enqueue_modification(
                        std::move(*mods_pack.remove_mod));
                }
                node->clear_draw_unit_updates(new_lookup_key);
            }
        }
        node->clear_dirty_flags(
            Node::DIRTY_DRAW_KEYS_RECURSIVE |
            Node::DIRTY_DRAW_VERTICES_RECURSIVE);
    }
}

void
Scene::draw(
    const uint16_t render_pass, const int16_t viewport,
    const DrawCall& draw_call)
{
    // translate z_index to index
    uint16_t viewport_index = render_pass + std::abs(min_viewport_z_index);
    this->_draw_commands.push_back({render_pass, viewport_index, draw_call});
}

void
Scene::remove_marked_nodes()
{
    // iterate in reverse order to delete children nodes first
    for (auto it = this->_nodes_remove_queue.rbegin();
         it != this->_nodes_remove_queue.rend(); it++) {
        delete (*it);
    }
    this->_nodes_remove_queue.clear();
}

void
Scene::on_attach()
{}

void
Scene::on_enter()
{}

void
Scene::update(const Duration dt)
{}

void
Scene::on_exit()
{}

void
Scene::on_detach()
{}

void
Scene::attach_frame_context(const std::unique_ptr<Renderer>& renderer)
{
    renderer->set_frame_context(
        this->_last_dt, this->_total_time, this->render_passes._take_snapshot(),
        this->viewports._take_snapshot());
}

void
Scene::render(const std::unique_ptr<Renderer>& renderer)
{
    this->draw_queue.process_modifications();

    // render nodes tree
    for (const auto& [key, bucket] : this->draw_queue) {
        auto batch = RenderBatch::from_bucket(key, bucket);
        if (batch.geometry_stream.empty()) {
            continue;
        }
        renderer->render_batch(batch, key.render_passes, key.viewports);
    }

    // render custom draw calls
    for (auto& draw_command : this->_draw_commands) {
        renderer->render_draw_command(
            draw_command, draw_command.pass, draw_command.viewport);
    }
    this->_draw_commands.clear();

    // render effects
    for (auto& render_pass : this->render_passes) {
        if (not render_pass.effect) {
            continue;
        }
        renderer->render_effect(
            render_pass.effect.value(), render_pass.index());
    }
}

void
Scene::register_simulation(Node* node)
{
    KAACORE_ASSERT(
        node->_type == NodeType::space,
        "Invalid type - space node type expected.");
    KAACORE_ASSERT(
        node->space._cp_space != nullptr,
        "Space node has invalid internal state.");
    if (this->simulations_registry.find(node) ==
        this->simulations_registry.end()) {
        this->simulations_registry.insert(node);
    }
}

void
Scene::unregister_simulation(Node* node)
{
    KAACORE_ASSERT(
        node->_type == NodeType::space,
        "Invalid type - space node type expected.");
    KAACORE_ASSERT(
        node->space._cp_space != nullptr,
        "Space node has invalid internal state.");
    auto pos = this->simulations_registry.find(node);
    KAACORE_ASSERT(
        pos != this->simulations_registry.end(),
        "Can't unregister from simulation, space node not in registry.");
    this->simulations_registry.erase(pos);
}

void
Scene::handle_add_node_to_tree(Node* node)
{
    KAACORE_LOG_DEBUG("Adding node to scene tree: {}", fmt::ptr(node));
    KAACORE_ASSERT(node->_scene != nullptr, "Node does not belong to a scene");
    this->spatial_index.start_tracking(node);
    node->_scene_tree_id = this->_node_scene_tree_id_counter.fetch_add(
                               1, std::memory_order_relaxed) +
                           1;
}

void
Scene::handle_remove_node_from_tree(Node* node)
{
    KAACORE_LOG_DEBUG("Removing node from scene tree: {}", fmt::ptr(node));
    KAACORE_ASSERT(node->_marked_to_delete, "Node should be marked to delete");
    this->_nodes_remove_queue.push_back(node);
    this->spatial_index.stop_tracking(node);

    if (auto mod = node->calculate_draw_unit_removal()) {
        KAACORE_LOG_DEBUG("Removing node from draw queue: {}", fmt::ptr(node));
        KAACORE_ASSERT(mod->type == DrawUnitModification::Type::remove, "");
        this->draw_queue.enqueue_modification(std::move(*mod));
    }
}

Duration
Scene::total_time() const
{
    return this->_total_time;
}

double
Scene::time_scale() const
{
    return this->_time_scale;
}

void
Scene::time_scale(const double scale)
{
    this->_time_scale = scale;
}

const std::vector<Event>&
Scene::get_events() const
{
    return get_engine()->input_manager->events_queue;
}

void
Scene::_reset()
{
    this->viewports._mark_dirty();
    this->render_passes._mark_dirty();
}

} // namespace kaacore

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <limits>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/statistics.h"
#include "kaacore/views.h"

#include "kaacore/scenes.h"

namespace kaacore {

Scene::Scene() : timers(this)
{
    this->root_node._scene = this;
    this->spatial_index.start_tracking(&this->root_node);
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
    return this->views[views_default_z_index].camera;
}

void
Scene::process_physics(const HighPrecisionDuration dt)
{
    StopwatchStatAutoPusher stopwatch{"scene.physics_sync:time"};
    for (Node* space_node : this->simulations_registry) {
        space_node->space.simulate(dt);
    }
}

void
Scene::process_nodes(const HighPrecisionDuration dt)
{
    StopwatchStatAutoPusher stopwatch{"scene.process_nodes:time"};
    static std::deque<Node*> processing_queue;
    processing_queue.clear();

    processing_queue.push_back(&this->root_node);
    while (not processing_queue.empty()) {
        Node* node = processing_queue.front();
        processing_queue.pop_front();

        if (node->_marked_to_delete) {
            // TODO optimize flow
            node->recursive_call([this](const Node* n) {
                if (auto draw_unit_removal = n->calculate_draw_unit_removal()) {
                    KAACORE_LOG_TRACE(
                        "Removing node from draw queue: {}", fmt::ptr(n));
                    this->draw_queue.enqueue_modification(
                        std::move(*draw_unit_removal));
                }
            });
            delete node;
            continue;
        }

        if (node->_lifetime > 0us) {
            if ((node->_lifetime -= std::min(dt, node->_lifetime)) == 0us) {
                // ensure that node is cleaned-up before deletion
                if (not node->_marked_to_delete) {
                    node->_mark_to_delete();
                }
                // TODO optimize flow
                node->recursive_call([this](const Node* n) {
                    if (auto draw_unit_removal =
                            n->calculate_draw_unit_removal()) {
                        KAACORE_LOG_TRACE(
                            "Removing node from draw queue: {}", fmt::ptr(n));
                        this->draw_queue.enqueue_modification(
                            std::move(*draw_unit_removal));
                    }
                });
                delete node;
                continue;
            }
        }

        if (node->_type == NodeType::body) {
            node->body.sync_simulation_position();
            node->body.sync_simulation_rotation();
        }

        if (node->_transitions_manager) {
            node->_transitions_manager.step(node, dt);
        }

        if (node->_spatial_data.is_dirty) {
            this->spatial_index.update_single(node);
        }

        for (const auto child_node : node->_children) {
            processing_queue.push_back(child_node);
        }
    }
}

void
Scene::resolve_dirty_nodes()
{
    static std::deque<Node*> processing_queue;
    StopwatchStatAutoPusher stopwatch{"scene.resolve_nodes:time"};
    processing_queue.clear();

    processing_queue.push_back(&this->root_node);
    while (not processing_queue.empty()) {
        Node* node = processing_queue.front();
        processing_queue.pop_front();

        if (node->_marked_to_delete) {
            // TODO optimize flow
            if (auto draw_unit_removal = node->calculate_draw_unit_removal()) {
                KAACORE_LOG_TRACE(
                    "Removing node from draw queue: {}", fmt::ptr(node));
                this->draw_queue.enqueue_modification(
                    std::move(*draw_unit_removal));
            }
            delete node;
            continue;
        }

        node->recalculate_model_matrix();

        if (node->_spatial_data.is_dirty) {
            this->spatial_index.update_single(node);
        }

        for (const auto child_node : node->_children) {
            processing_queue.push_back(child_node);
        }
    }
}

void
Scene::process_nodes_drawing()
{
    static std::deque<Node*> processing_queue;
    static std::deque<Node*> rendering_queue;
    KAACORE_LOG_TRACE("Starting process_nodes_drawing()");
    StopwatchStatAutoPusher stopwatch{"scene.nodes_drawing:time"};
    CounterStatAutoPusher dq_inserts_count{"scene.draw_queue:inserts:count"};
    CounterStatAutoPusher dq_updates_count{"scene.draw_queue:updates:count"};
    CounterStatAutoPusher dq_removals_count{"scene.draw_queue:removals:count"};
    processing_queue.clear();
    rendering_queue.clear();
    processing_queue.push_back(&this->root_node);
    while (not processing_queue.empty()) {
        Node* node = processing_queue.front();
        processing_queue.pop_front();
        rendering_queue.push_back(node);

        for (const auto child_node : node->_children) {
            processing_queue.push_back(child_node);
        }
    }

    for (auto* node : rendering_queue) {
        if (node->has_draw_unit_updates()) {
            // KAACORE_LOG_TRACE("Update for node: {}", fmt::ptr(node));
            auto [mod_1, mod_2] = node->calculate_draw_unit_updates();
            node->clear_draw_unit_updates(mod_1.lookup_key);
            this->draw_queue.enqueue_modification(std::move(mod_1));
            switch (mod_1.type) {
                case DrawUnitModification::Type::insert:
                    dq_inserts_count += 1;
                    break;
                case DrawUnitModification::Type::update:
                    dq_updates_count += 1;
                    break;
                case DrawUnitModification::Type::remove:
                    dq_removals_count += 1;
                    break;
            }
            if (mod_2) {
                KAACORE_ASSERT(
                    mod_2->type == DrawUnitModification::Type::remove, "");
                this->draw_queue.enqueue_modification(std::move(*mod_2));
                dq_removals_count += 1;
            }

            // TODO handle removal
        }
    }

    for (auto& view : this->views) {
        get_engine()->renderer->process_view(view);
    }

    this->draw_queue.process_modifications();
    get_engine()->renderer->render_draw_queue(this->draw_queue);
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
Scene::reset_views()
{
    this->views._mark_dirty();
}

} // namespace kaacore

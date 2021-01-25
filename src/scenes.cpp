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
            delete node;
            continue;
        }

        if (node->_lifetime > 0us) {
            if ((node->_lifetime -= std::min(dt, node->_lifetime)) == 0us) {
                // ensure that node is cleaned-up before deletion
                if (not node->_marked_to_delete) {
                    node->_mark_to_delete();
                }
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
    static std::vector<std::pair<uint64_t, Node*>> rendering_queue;

    StopwatchStatAutoPusher stopwatch{"scene.nodes_drawing:time"};
    CounterStatAutoPusher draw_calls_counter{"scene.draw_calls"};

    processing_queue.clear();
    rendering_queue.clear();
    processing_queue.push_back(&this->root_node);
    auto renderer = get_engine()->renderer.get();
    while (not processing_queue.empty()) {
        Node* node = processing_queue.front();
        processing_queue.pop_front();

        if (not node->_visible) {
            continue;
        }

        for (const auto child_node : node->_children) {
            processing_queue.push_back(child_node);
        }

        node->recalculate_render_data();
        node->recalculate_ordering_data();

        rendering_queue.emplace_back(
            std::abs(std::numeric_limits<int16_t>::min()) +
                node->_ordering_data.calculated_z_index,
            node);
    }

    std::stable_sort(
        rendering_queue.begin(), rendering_queue.end(),
        [](const std::pair<uint64_t, Node*> a,
           const std::pair<uint64_t, Node*> b) {
            return std::get<uint64_t>(a) < std::get<uint64_t>(b);
        });

    for (auto& view : this->views) {
        renderer->process_view(view);
    }

    for (const auto& qn : rendering_queue) {
        auto node = std::get<Node*>(qn);
        if (node->_render_data.computed_vertices.empty()) {
            continue;
        }

        node->_ordering_data.calculated_views.each_active_z_index(
            [this, &renderer, &node, &draw_calls_counter](int16_t z_index) {
                auto& view = this->views[z_index];

                draw_calls_counter += 1;
                if (node->type() == NodeType::text) {
                    renderer->render_vertices(
                        view.internal_index(),
                        node->_render_data.computed_vertices,
                        node->_shape.indices, node->_render_data.texture_handle,
                        renderer->sdf_font_program);
                } else {
                    renderer->render_vertices(
                        view.internal_index(),
                        node->_render_data.computed_vertices,
                        node->_shape.indices, node->_render_data.texture_handle,
                        renderer->default_program);
                }
            });
    }
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

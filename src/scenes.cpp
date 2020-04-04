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

namespace kaacore {

Scene::Scene()
{
    this->root_node._scene = this;
    this->views[0].clear_color({0, 0, 0, 1});
}

Scene::~Scene()
{
    while (not this->root_node._children.empty()) {
        delete this->root_node._children[0];
    }
    KAACORE_ASSERT_TERMINATE(this->simulations_registry.empty());
}

Camera&
Scene::camera()
{
    return this->views[0].camera;
}

void
Scene::process_frame(uint32_t dt)
{
    this->process_physics(dt);
    this->process_nodes(dt);
    this->update(dt);
    this->process_nodes_drawing();
}

void
Scene::process_physics(uint32_t dt)
{
    for (Node* space_node : this->simulations_registry) {
        space_node->space.simulate(dt);
    }
}

void
Scene::process_nodes(uint32_t dt)
{
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

        if (node->_lifetime) {
            if ((node->_lifetime -= std::min(dt, node->_lifetime)) == 0) {
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

        node->recalculate_model_matrix();
        node->recalculate_render_data();

        rendering_queue.emplace_back(
            std::abs(std::numeric_limits<int16_t>::min()) + node->_z_index,
            node);
    }

    std::stable_sort(rendering_queue.begin(), rendering_queue.end());

    for (const auto& qn : rendering_queue) {
        auto node = std::get<Node*>(qn);
        if (node->_render_data.computed_vertices.empty()) {
            continue;
        }

        for (auto z_index : node->_views) {
            auto& view = this->views[z_index];
            if (view.is_dirty()) {
                view.refresh();
            }

            renderer->render_vertices(
                view.index(), node->_render_data.computed_vertices,
                node->_shape.indices, node->_render_data.texture_handle);
        }
    }

    this->views._touch();
}

void
Scene::on_attach()
{}

void
Scene::on_enter()
{}

void
Scene::update(uint32_t dt)
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
    KAACORE_ASSERT(node->_type == NodeType::space);
    KAACORE_ASSERT(node->space._cp_space != nullptr);
    if (this->simulations_registry.find(node) ==
        this->simulations_registry.end()) {
        this->simulations_registry.insert(node);
    }
}

void
Scene::unregister_simulation(Node* node)
{
    KAACORE_ASSERT(node->_type == NodeType::space);
    KAACORE_ASSERT(node->space._cp_space != nullptr);
    auto pos = this->simulations_registry.find(node);
    KAACORE_ASSERT(pos != this->simulations_registry.end());
    this->simulations_registry.erase(pos);
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

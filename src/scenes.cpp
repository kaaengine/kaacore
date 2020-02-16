#include <algorithm>
#include <cstdlib>
#include <deque>
#include <limits>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"

#include "kaacore/scenes.h"

namespace kaacore {

Scene::Scene()
{
    this->root_node._scene = this;
}

Scene::~Scene()
{
    while (not this->root_node._children.empty()) {
        delete this->root_node._children[0];
    }
    KAACORE_ASSERT_TERMINATE(this->simulations_registry.empty());
}

void
Scene::process_nodes(uint32_t dt)
{
    static std::deque<Node*> processing_queue;

    processing_queue.clear();
    processing_queue.push_back(&this->root_node);

    for (Node* space_node : this->simulations_registry) {
        space_node->space.simulate(dt);
    }

    while (not processing_queue.empty()) {
        Node* node = processing_queue.front();
        processing_queue.pop_front();

        if (node->_marked_to_delete) {
            delete node;
            continue;
        }

        if (node->_lifetime) {
            if (node->_lifetime <= dt) {
                delete node;
                continue;
            } else {
                KAACORE_ASSERT(node->_lifetime > dt);
                node->_lifetime -= dt;
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
Scene::process_nodes_drawing(uint32_t dt)
{
    static std::deque<Node*> processing_queue;
    static std::vector<std::pair<uint64_t, Node*>> rendering_queue;

    processing_queue.clear();
    rendering_queue.clear();
    processing_queue.push_back(&this->root_node);
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

    std::sort(rendering_queue.begin(), rendering_queue.end());

    for (const auto& qn : rendering_queue) {
        if (std::get<Node*>(qn)->_render_data.computed_vertices.empty()) {
            continue;
        }
        get_engine()->renderer->render_vertices(
            std::get<Node*>(qn)->_render_data.computed_vertices,
            std::get<Node*>(qn)->_shape.indices,
            std::get<Node*>(qn)->_render_data.texture_handle);
    }
}

void
Scene::process_frame(uint32_t dt)
{
    this->process_nodes(dt);
    this->update(dt);
    this->camera.refresh();
    bgfx::setViewTransform(
        0, glm::value_ptr(this->camera.calculated_view),
        glm::value_ptr(get_engine()->renderer->projection_matrix));
    this->process_nodes_drawing(dt);
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

Camera::Camera()
{
    auto virtual_resolution = get_engine()->virtual_resolution();
    this->position = {double(virtual_resolution.x) / 2,
                      double(virtual_resolution.y) / 2};
    this->refresh();
}

void
Camera::refresh()
{
    this->calculated_view = glm::translate(
        glm::rotate(
            glm::scale(
                glm::fmat4(1.0), glm::fvec3(this->scale.x, this->scale.y, 1.)),
            static_cast<float>(this->rotation), glm::fvec3(0., 0., 1.)),
        glm::fvec3(-this->position.x, -this->position.y, 0.));
}

glm::dvec2
Camera::unproject_position(const glm::dvec2& pos)
{
    this->refresh();
    auto virtual_resolution = get_engine()->virtual_resolution();

    // account for virtual_resolution / 2 since we want to get
    // top-left corner of camera 'window'
    glm::fvec4 pos4 = {pos.x - virtual_resolution.x / 2,
                       pos.y - virtual_resolution.y / 2, 0., 1.};
    pos4 = glm::inverse(this->calculated_view) * pos4;
    return {pos4.x, pos4.y};
}

} // namespace kaacore

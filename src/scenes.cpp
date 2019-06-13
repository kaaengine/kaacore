#include <algorithm>
#include <vector>
#include <deque>
#include <limits>
#include <cstdlib>
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "kaacore/engine.h"
#include "kaacore/log.h"
#include "kaacore/exceptions.h"

#include "kaacore/scenes.h"


namespace kaacore {

Scene::Scene()
{
    this->root_node.scene = this;
}


Scene::~Scene() noexcept(false)
{
    auto engine = get_engine();
    if (this == engine->scene || this == engine->next_scene) {
        throw kaacore::exception(
            "An attempt to delete current scene detected. Aborting."
        );
    }

    while (not this->root_node.children.empty()) {
        delete this->root_node.children[0];
    }
    KAACORE_ASSERT(this->simulations_registry.empty());
}


void Scene::process_nodes(uint32_t dt)
{
    static std::deque<Node*> processing_queue;
    static std::vector<std::pair<uint64_t, Node*>> rendering_queue;

    // process simulations before everything else, so collision callbacks
    // won't break nodes tree during processing
    for (Node* space_node : this->simulations_registry) {
        space_node->space.simulate(dt);
    }

    processing_queue.clear();
    rendering_queue.clear();
    processing_queue.push_back(&this->root_node);
    while (not processing_queue.empty()) {
        Node* node = processing_queue.front();
        processing_queue.pop_front();

        for (const auto child_node : node->children) {
            processing_queue.push_back(child_node);
        }

        if (node->type == NodeType::body) {
            node->body.sync_simulation_position();
            node->body.sync_simulation_rotation();
        }

        if (node->sprite and node->sprite.auto_animate) {
            node->sprite.animation_time_step(dt);
        }

        node->recalculate_matrix();
        node->recalculate_render_data();

        rendering_queue.emplace_back(
            std::abs(std::numeric_limits<int16_t>::min()) + node->z_index,
            node
        );
    }

    std::sort(rendering_queue.begin(), rendering_queue.end());

    for (const auto& qn : rendering_queue) {
        if (std::get<Node*>(qn)->render_data.computed_vertices.empty() or
            not std::get<Node*>(qn)->visible) {
            continue;
        }
        get_engine()->renderer->render_vertices(
            std::get<Node*>(qn)->render_data.computed_vertices,
            std::get<Node*>(qn)->shape.indices,
            std::get<Node*>(qn)->render_data.texture_handle
        );
    }
}


void Scene::process_frame(uint32_t dt)
{
    bgfx::setViewTransform(
        0,
        glm::value_ptr(this->camera.calculated_view),
        glm::value_ptr(get_engine()->renderer->projection_matrix)
    );
    this->time += dt;
    this->update(dt);
    this->process_nodes(dt);
}


void Scene::on_enter()
{}


void Scene::update(uint32_t dt)
{}


void Scene::on_exit()
{}


void Scene::register_simulation(Node* node)
{
    KAACORE_ASSERT(node->type == NodeType::space);
    KAACORE_ASSERT(node->space.cp_space != nullptr);
    if (this->simulations_registry.find(node) == this->simulations_registry.end()) {
        this->simulations_registry.insert(node);
    }
}

void Scene::unregister_simulation(Node* node)
{
    KAACORE_ASSERT(node->type == NodeType::space);
    KAACORE_ASSERT(node->space.cp_space != nullptr);
    auto pos = this->simulations_registry.find(node);
    KAACORE_ASSERT(pos != this->simulations_registry.end());
    this->simulations_registry.erase(pos);
}

const std::vector<Event>& Scene::get_events() const
{
    return get_engine()->input_manager->events_queue;
}


Camera::Camera()
{
    this->refresh();
}


void Camera::refresh()
{
    this->calculated_view = \
        glm::translate(
            glm::rotate(
                glm::scale(
                    glm::fmat4(1.0),
                    glm::fvec3(this->scale.x, this->scale.y, 1.)),
                static_cast<float>(this->rotation), glm::fvec3(0., 0., 1.)),
            glm::fvec3(-this->position.x, -this->position.y, 0.)
        );
}

} // namespace kaacore

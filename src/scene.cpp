#include <algorithm>
#include <vector>
#include <deque>
#include <limits>
#include <cstdlib>
#include <utility>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "kaacore/engine.h"

#include "kaacore/scene.h"
#include "kaacore/log.h"


Scene::Scene()
{
    this->root_node.scene = this;
}


void Scene::process_nodes(uint32_t dt)
{
    static std::deque<Node*> processing_queue;
    static std::vector<std::pair<uint64_t, Node*>> rendering_queue;

    processing_queue.clear();
    rendering_queue.clear();
    processing_queue.push_back(&this->root_node);
    while (not processing_queue.empty()) {
        Node* node = processing_queue.front();
        processing_queue.pop_front();

        for (const auto child_node : node->children) {
            processing_queue.push_back(child_node);
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
        if (std::get<Node*>(qn)->render_data.computed_vertices.empty()) {
            continue;
        }
        get_engine()->renderer->render_vertices(
            std::get<Node*>(qn)->render_data.computed_vertices,
            std::get<Node*>(qn)->shape.indices,
            get_engine()->renderer->default_texture
        );
    }
}


void Scene::process_frame(uint32_t dt)
{
    bgfx::setViewTransform(0,
                           glm::value_ptr(this->camera.calculated_view),
                           glm::value_ptr(this->camera.calculated_proj));
    this->time += dt;
    this->update(dt);
    this->process_nodes(dt);
}


void Scene::update(uint32_t dt)
{
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

    this->calculated_proj = glm::ortho(
        -this->size.x / 2, this->size.x / 2,
        this->size.y / 2, -this->size.y / 2
    );
}

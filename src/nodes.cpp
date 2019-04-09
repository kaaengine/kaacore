#include <iostream>
#include <algorithm>
#include <functional>

#include <glm/gtc/matrix_transform.hpp>

#include "kaacore/engine.h"
#include "kaacore/nodes.h"
#include "kaacore/shapes.h"
#include "kaacore/scenes.h"
#include "kaacore/log.h"
#include "kaacore/exceptions.h"


namespace kaacore {

Node::Node(NodeType type) : type(type)
{
    if (type == NodeType::space) {
        this->space.initialize();
    } else if (type == NodeType::body) {
        this->body.initialize();
    } else if (type == NodeType::hitbox) {
        this->hitbox.initialize();
        this->color = {1., 0., 1., 0.5};
        this->z_index = 100;
    }
}

Node::~Node()
{
    if (this->parent != nullptr) {
        auto pos_in_parent = std::find(
            this->parent->children.begin(), this->parent->children.end(), this
        );
        if (pos_in_parent != this->parent->children.end()) {
            this->parent->children.erase(pos_in_parent);
        }
    }

    while (not this->children.empty()) {
        delete this->children[0];
    }

    if (this->type == NodeType::space) {
        this->space.destroy();
    } else if (this->type == NodeType::body) {
        this->body.destroy();
    } else if (this->type == NodeType::hitbox) {
        this->hitbox.destroy();
    }
}

void Node::add_child(Node* child_node)
{
    KAACORE_CHECK(child_node->parent == nullptr);
    child_node->parent = this;
    this->children.push_back(child_node);

    // TODO set root
    // TODO optimize (replace with iterator?)
    std::function<void(Node*)> initialize_node;
    initialize_node = [&initialize_node, this](Node* n)
    {
        n->scene = this->scene;
        if (n->type == NodeType::space) {
            n->scene->register_simulation(n);
        } else if (n->type == NodeType::body) {
            n->body.attach_to_simulation();
        } else if (n->type == NodeType::hitbox) {
            n->hitbox.attach_to_simulation();
        }

        std::for_each(n->children.begin(), n->children.end(),
                      initialize_node);
    };
    initialize_node(child_node);
}

void Node::recalculate_matrix()
{
    static glm::fmat4 identity(1.0);
    glm::fmat4* parent_matrix_p;
    if (this->parent != nullptr) {
        parent_matrix_p = &this->parent->matrix;
    } else {
        parent_matrix_p = &identity;
    }
    this->matrix = \
        glm::scale(
            glm::rotate(
                glm::translate(
                    *parent_matrix_p,
                    glm::fvec3(this->position.x, this->position.y, 0.)
                ),
                static_cast<float>(this->rotation), glm::fvec3(0., 0., 1.)
                ),
            glm::fvec3(this->scale.x, this->scale.y, 1.)
        );
}

void Node::recalculate_render_data()
{
    // TODO optimize
    this->render_data.computed_vertices = this->shape.vertices;
    for (auto& vertex : this->render_data.computed_vertices) {
        glm::dvec4 pos = {vertex.xyz.x, vertex.xyz.y, vertex.xyz.z, 1.};
        pos = this->matrix * pos;
        vertex.xyz = {pos.x, pos.y, pos.z};

        if (this->sprite.has_texture()) {
            auto uv_rect = this->sprite.get_display_rect();
            vertex.uv = glm::mix(
                uv_rect.first, uv_rect.second, vertex.uv
            );
        }

        vertex.rgba *= this->color;
    }

    if (this->sprite.has_texture()) {
        this->render_data.texture_handle = this->sprite.texture->texture_handle;
    } else {
        this->render_data.texture_handle = get_engine()->renderer->default_texture;
    }
}

void Node::set_position(const glm::dvec2& position)
{
    this->position = position;
    if (this->type == NodeType::body) {
        this->body.override_simulation_position();
    }
}

void Node::set_shape(const Shape& shape)
{
    this->shape = shape;
    if (this->type == NodeType::hitbox) {
        this->hitbox.update_physics_shape();
    }
}

void Node::set_sprite(const Sprite& sprite)
{
    this->sprite = sprite;
    if (!this->shape) {
        this->set_shape(Shape::Box(sprite.get_size()));
    }
}

glm::dvec2 Node::get_absolute_position()
{
    this->recalculate_matrix();
    glm::fvec4 pos = {0., 0., 0., 1.};
    pos = this->matrix * pos;
    return {pos.x, pos.y};
}


MyForeignWrapper::MyForeignWrapper() {
    std::cout << "MyForeignWrapper ctor!" << std::endl;
}


MyForeignWrapper::~MyForeignWrapper() {
    std::cout << "MyForeignWrapper dtor!" << std::endl;
}

} // namespace kaacore

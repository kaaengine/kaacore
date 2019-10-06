#include <algorithm>
#include <functional>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/log.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"
#include "kaacore/shapes.h"

namespace kaacore {

Node::Node(NodeType type) : _type(type)
{
    if (type == NodeType::space) {
        new (&this->space) SpaceNode();
    } else if (type == NodeType::body) {
        new (&this->body) BodyNode();
    } else if (type == NodeType::hitbox) {
        new (&this->hitbox) HitboxNode();
        this->_color = {1., 0., 0., 0.};
    } else if (type == NodeType::text) {
        new (&this->text) TextNode();
        this->_origin_alignment = Alignment::center;
    }
}

Node::~Node()
{
    if (this->_parent != nullptr) {
        auto pos_in_parent = std::find(
            this->_parent->_children.begin(), this->_parent->_children.end(),
            this);
        if (pos_in_parent != this->_parent->_children.end()) {
            this->_parent->_children.erase(pos_in_parent);
        }
    }

    while (not this->_children.empty()) {
        delete this->_children[0];
    }

    if (this->_type == NodeType::space) {
        if (this->_scene) {
            this->_scene->unregister_simulation(this);
        }
        this->space.~SpaceNode();
    } else if (this->_type == NodeType::body) {
        this->body.~BodyNode();
    } else if (this->_type == NodeType::hitbox) {
        this->hitbox.~HitboxNode();
    } else if (this->_type == NodeType::text) {
        this->text.~TextNode();
    }
}

void
Node::add_child(Node* child_node)
{
    KAACORE_CHECK(child_node->_parent == nullptr);
    child_node->_parent = this;
    this->_children.push_back(child_node);

    // TODO set root
    // TODO optimize (replace with iterator?)
    std::function<void(Node*)> initialize_node;
    initialize_node = [&initialize_node, this](Node* n) {
        n->_scene = this->_scene;
        if (n->_type == NodeType::space) {
            n->_scene->register_simulation(n);
        } else if (n->_type == NodeType::body) {
            n->body.attach_to_simulation();
        } else if (n->_type == NodeType::hitbox) {
            n->hitbox.update_physics_shape();
        }

        std::for_each(
            n->_children.begin(), n->_children.end(), initialize_node);
    };
    initialize_node(child_node);
}

void
Node::recalculate_matrix()
{
    static glm::fmat4 identity(1.0);
    glm::fmat4* parent_matrix_p;
    if (this->_parent != nullptr) {
        parent_matrix_p = &this->_parent->_matrix;
    } else {
        parent_matrix_p = &identity;
    }
    this->_matrix = glm::scale(
        glm::rotate(
            glm::translate(
                *parent_matrix_p,
                glm::fvec3(this->_position.x, this->_position.y, 0.)),
            static_cast<float>(this->_rotation), glm::fvec3(0., 0., 1.)),
        glm::fvec3(this->_scale.x, this->_scale.y, 1.));
}

void
Node::recalculate_render_data()
{
    // TODO optimize
    glm::fvec2 pos_realignment = calculate_realignment_vector(
        this->_origin_alignment, this->_shape.vertices_bbox);
    this->_render_data.computed_vertices = this->_shape.vertices;
    for (auto& vertex : this->_render_data.computed_vertices) {
        glm::dvec4 pos = {vertex.xyz.x + pos_realignment.x,
                          vertex.xyz.y + pos_realignment.y, vertex.xyz.z, 1.};
        pos = this->_matrix * pos;
        vertex.xyz = {pos.x, pos.y, pos.z};

        if (this->_sprite.has_texture()) {
            auto uv_rect = this->_sprite.get_display_rect();
            vertex.uv = glm::mix(uv_rect.first, uv_rect.second, vertex.uv);
        }

        vertex.rgba *= this->_color;
    }

    if (this->_sprite.has_texture()) {
        this->_render_data.texture_handle =
            this->_sprite.texture->texture_handle;
    } else {
        this->_render_data.texture_handle =
            get_engine()->renderer->default_texture;
    }
}

const NodeType
Node::type() const
{
    return this->_type;
}

const std::vector<Node*>&
Node::children()
{
    return this->_children;
}

glm::dvec2
Node::position()
{
    return this->_position;
}

glm::dvec2
Node::absolute_position()
{
    this->recalculate_matrix();
    glm::fvec4 pos = {0., 0., 0., 1.};
    pos = this->_matrix * pos;
    return {pos.x, pos.y};
}

void
Node::position(const glm::dvec2& position)
{
    this->_position = position;
    if (this->_type == NodeType::body) {
        this->body.override_simulation_position();
    } else if (this->_type == NodeType::hitbox) {
        this->hitbox.update_physics_shape();
    }
}

double
Node::rotation()
{
    return this->_rotation;
}

void
Node::rotation(const double& rotation)
{
    this->_rotation = rotation;
    if (this->_type == NodeType::body) {
        this->body.override_simulation_rotation();
    } else if (this->_type == NodeType::hitbox) {
        this->hitbox.update_physics_shape();
    }
}

glm::dvec2
Node::scale()
{
    return this->_scale;
}

void
Node::scale(const glm::dvec2& scale)
{
    this->_scale = scale;
    if (this->_type == NodeType::body) {
        for (const auto& n : this->_children) {
            if (n->_type == NodeType::hitbox) {
                n->hitbox.update_physics_shape();
            }
        }
    } else if (this->_type == NodeType::hitbox) {
        this->hitbox.update_physics_shape();
    }
}

int16_t
Node::z_index()
{
    return this->_z_index;
}

void
Node::z_index(const int16_t& z_index)
{
    this->_z_index = z_index;
}

Shape
Node::shape()
{
    return this->_shape;
}

void
Node::shape(const Shape& shape)
{
    this->_shape = shape;
    if (this->_type == NodeType::hitbox) {
        this->hitbox.update_physics_shape();
    }
}

Sprite&
Node::sprite_ref()
{
    return this->_sprite;
}

void
Node::sprite(const Sprite& sprite)
{
    this->_sprite = sprite;
    if (!this->_shape) {
        this->shape(Shape::Box(sprite.get_size()));
    }
}

glm::dvec4
Node::color()
{
    return this->_color;
}

void
Node::color(const glm::dvec4& color)
{
    this->_color = color;
}

bool
Node::visible()
{
    return this->_visible;
}

void
Node::visible(const bool& visible)
{
    this->_visible = visible;
}

Alignment
Node::origin_alignment()
{
    return this->_origin_alignment;
}

void
Node::origin_alignment(const Alignment& alignment)
{
    this->_origin_alignment = alignment;
}

NodeTransitionHandle
Node::transition()
{
    return this->_transition.transition_handle;
}

void
Node::transition(const NodeTransitionHandle& transition)
{
    this->_transition.setup(transition);
}

uint32_t
Node::lifetime()
{
    return this->_lifetime;
}

void
Node::lifetime(const uint32_t& lifetime)
{
    this->_lifetime = lifetime;
}

Scene*
Node::scene() const
{
    return this->_scene;
}

Node*
Node::parent() const
{
    return this->_parent;
}

void
Node::setup_wrapper(std::unique_ptr<ForeignNodeWrapper>&& wrapper)
{
    KAACORE_ASSERT(!this->_node_wrapper);
    this->_node_wrapper = std::move(wrapper);
}

ForeignNodeWrapper*
Node::wrapper_ptr() const
{
    return this->_node_wrapper.get();
}

MyForeignWrapper::MyForeignWrapper()
{
    std::cout << "MyForeignWrapper ctor!" << std::endl;
}

MyForeignWrapper::~MyForeignWrapper()
{
    std::cout << "MyForeignWrapper dtor!" << std::endl;
}

} // namespace kaacore

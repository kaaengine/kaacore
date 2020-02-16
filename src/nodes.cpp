#include <algorithm>
#include <functional>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/geometry.h"
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
Node::_mark_dirty()
{
    this->_render_data.is_dirty = true;
    this->_model_matrix.is_dirty = true;
    for (auto child : this->_children) {
        if (not child->_model_matrix.is_dirty) {
            child->_mark_dirty();
        }
    }
}

void
Node::_mark_to_delete()
{
    this->_marked_to_delete = true;
    for (auto child : this->_children) {
        if (not child->_marked_to_delete) {
            child->_mark_to_delete();
        }
    }
}

glm::fmat4
Node::_compute_model_matrix(const glm::fmat4& parent_matrix) const
{
    return glm::scale(
        glm::rotate(
            glm::translate(
                parent_matrix,
                glm::fvec3(this->_position.x, this->_position.y, 0.)),
            static_cast<float>(this->_rotation), glm::fvec3(0., 0., 1.)),
        glm::fvec3(this->_scale.x, this->_scale.y, 1.));
}

glm::fmat4
Node::_compute_model_matrix_cumulative(const Node* const ancestor) const
{
    const Node* pointer = this;
    std::vector<const Node*> inheritance_chain{pointer};
    while ((pointer = pointer->_parent) != ancestor) {
        if (pointer == nullptr) {
            throw kaacore::exception("Can't compute position relative to node "
                                     "that isn't its parent.");
        }
        inheritance_chain.push_back(pointer);
    }

    glm::fmat4 matrix(1.0);
    for (auto it = inheritance_chain.rbegin(); it != inheritance_chain.rend();
         it++) {
        matrix = (*it)->_compute_model_matrix(matrix);
    }
    return matrix;
}

void
Node::_recalculate_model_matrix()
{
    const static glm::fmat4 identity(1.0);
    this->_model_matrix.value = this->_compute_model_matrix(
        this->_parent ? this->_parent->_model_matrix.value : identity);
    this->_model_matrix.is_dirty = false;
}

void
Node::_recalculate_model_matrix_cumulative()
{
    Node* pointer = this;
    std::vector<Node*> inheritance_chain{pointer};
    while ((pointer = pointer->_parent) != nullptr and
           pointer->_model_matrix.is_dirty) {
        inheritance_chain.push_back(pointer);
    }

    for (auto it = inheritance_chain.rbegin(); it != inheritance_chain.rend();
         it++) {
        (*it)->_recalculate_model_matrix();
    }
}

void
Node::_set_position(const glm::dvec2& position)
{
    if (position != this->_position) {
        this->_mark_dirty();
    }
    this->_position = position;
}

void
Node::_set_rotation(const double rotation)
{
    if (rotation != this->_rotation) {
        this->_mark_dirty();
    }
    this->_rotation = rotation;
}

void
Node::add_child(NodeOwnerPtr& child_node)
{
    KAACORE_CHECK(child_node->_parent == nullptr);
    KAACORE_CHECK(child_node._ownership_transferred == false);

    child_node->_parent = this;
    child_node._ownership_transferred = true;
    this->_children.push_back(child_node.get());

    if (child_node->_node_wrapper) {
        child_node->_node_wrapper->on_add_to_parent();
    }

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
    initialize_node(child_node.get());
}

void
Node::recalculate_model_matrix()
{
    if (not this->_model_matrix.is_dirty) {
        return;
    }
    this->_recalculate_model_matrix();
}

void
Node::recalculate_render_data()
{
    if (not this->_render_data.is_dirty) {
        return;
    }

    // TODO optimize
    glm::fvec2 pos_realignment = calculate_realignment_vector(
        this->_origin_alignment, this->_shape.vertices_bbox);
    this->_render_data.computed_vertices = this->_shape.vertices;
    for (auto& vertex : this->_render_data.computed_vertices) {
        glm::dvec4 pos = {vertex.xyz.x + pos_realignment.x,
                          vertex.xyz.y + pos_realignment.y, vertex.xyz.z, 1.};
        pos = this->_model_matrix.value * pos;
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
    this->_render_data.is_dirty = false;
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

void
Node::position(const glm::dvec2& position)
{
    this->_set_position(position);
    if (this->_type == NodeType::body) {
        this->body.override_simulation_position();
    } else if (this->_type == NodeType::hitbox) {
        this->hitbox.update_physics_shape();
    }
}

glm::dvec2
Node::absolute_position()
{
    if (this->_model_matrix.is_dirty) {
        this->_recalculate_model_matrix_cumulative();
    }

    glm::fvec4 pos = {0., 0., 0., 1.};
    pos = this->_model_matrix.value * pos;
    return {pos.x, pos.y};
}

glm::dvec2
Node::get_relative_position(const Node* const ancestor)
{
    if (ancestor == this->_parent) {
        return this->position();
    } else if (ancestor == nullptr) {
        return this->absolute_position();
    } else if (ancestor == this) {
        return {0., 0.};
    }

    glm::fvec4 pos = {0., 0., 0., 1.};
    pos = this->_compute_model_matrix_cumulative(ancestor) * pos;
    return {pos.x, pos.y};
}

double
Node::rotation()
{
    return this->_rotation;
}

double
Node::absolute_rotation()
{
    if (this->_model_matrix.is_dirty) {
        this->_recalculate_model_matrix_cumulative();
    }

    return DecomposedTransformation<float>(this->_model_matrix.value).rotation;
}

void
Node::rotation(const double& rotation)
{
    this->_set_rotation(rotation);
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

glm::dvec2
Node::absolute_scale()
{
    if (this->_model_matrix.is_dirty) {
        this->_recalculate_model_matrix_cumulative();
    }

    return DecomposedTransformation<float>(this->_model_matrix.value).scale;
}

void
Node::scale(const glm::dvec2& scale)
{
    if (scale != this->_scale) {
        this->_mark_dirty();
    }
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

Transformation
Node::absolute_transformation()
{
    if (this->_model_matrix.is_dirty) {
        this->_recalculate_model_matrix_cumulative();
    }
    return Transformation{this->_model_matrix.value};
}

Transformation
Node::get_relative_transformation(const Node* const ancestor)
{
    if (ancestor == nullptr) {
        return this->absolute_transformation();
    } else if (ancestor == this) {
        return Transformation{glm::dmat4(1.)};
    }

    return Transformation{this->_compute_model_matrix_cumulative(ancestor)};
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
    // TODO: check if we aren't setting the same shape before marking it dirty
    this->_render_data.is_dirty = true;
}

Sprite
Node::sprite()
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
    // TODO: check if we aren't setting the same sprite before marking it dirty
    this->_render_data.is_dirty = true;
}

glm::dvec4
Node::color()
{
    return this->_color;
}

void
Node::color(const glm::dvec4& color)
{
    if (color != this->_color) {
        this->_render_data.is_dirty = true;
    }
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
    if (visible and visible != this->_visible) {
        this->_mark_dirty();
    }
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
    if (alignment != this->_origin_alignment) {
        this->_render_data.is_dirty = true;
    }
    this->_origin_alignment = alignment;
}

NodeTransitionHandle
Node::transition()
{
    return this->_transitions_manager.get(default_transition_name);
}

void
Node::transition(const NodeTransitionHandle& transition)
{
    this->_transitions_manager.set(default_transition_name, transition);
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

NodeTransitionsManager&
Node::transitions_manager()
{
    return this->_transitions_manager;
}

Scene* const
Node::scene() const
{
    return this->_scene;
}

NodePtr
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

} // namespace kaacore

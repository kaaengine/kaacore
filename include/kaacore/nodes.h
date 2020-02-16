#pragma once

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "kaacore/fonts.h"
#include "kaacore/geometry.h"
#include "kaacore/node_ptr.h"
#include "kaacore/physics.h"
#include "kaacore/renderer.h"
#include "kaacore/shapes.h"
#include "kaacore/sprites.h"
#include "kaacore/transitions.h"

namespace kaacore {

enum struct NodeType {
    basic = 1,
    space = 2,
    body = 3,
    hitbox = 4,
    text = 5,
};

struct ForeignNodeWrapper {
    ForeignNodeWrapper() = default;
    virtual ~ForeignNodeWrapper() = default;

    virtual void on_add_to_parent() = 0;
};

struct Scene;

class Node {
    friend class _NodePtrBase;
    friend class NodePtr;
    friend class NodeOwnerPtr;
    friend struct Scene;
    friend struct SpaceNode;
    friend struct BodyNode;
    friend struct HitboxNode;

    const NodeType _type = NodeType::basic;
    glm::dvec2 _position = {0., 0.};
    double _rotation = 0.;
    glm::dvec2 _scale = {1., 1.};
    int16_t _z_index = 0;
    Shape _shape;
    Sprite _sprite;
    glm::dvec4 _color = {1., 1., 1., 1.};
    bool _visible = true;
    Alignment _origin_alignment = Alignment::none;
    uint32_t _lifetime = 0;
    NodeTransitionsManager _transitions_manager;

    Scene* _scene = nullptr;
    Node* _parent = nullptr;
    std::vector<Node*> _children;

    std::unique_ptr<ForeignNodeWrapper> _node_wrapper;

    struct {
        glm::fmat4 value;
        bool is_dirty = true;
    } _model_matrix;
    struct {
        std::vector<StandardVertexData> computed_vertices;
        bgfx::TextureHandle texture_handle;
        bool is_dirty = true;
    } _render_data;

    bool _marked_to_delete = false;

    void _mark_dirty();
    void _mark_to_delete();
    glm::fmat4 _compute_model_matrix(const glm::fmat4& parent_matrix) const;
    glm::fmat4 _compute_model_matrix_cumulative(
        const Node* const ancestor = nullptr) const;
    void _recalculate_model_matrix();
    void _recalculate_model_matrix_cumulative();
    void _set_position(const glm::dvec2& position);
    void _set_rotation(const double rotation);

  public:
    union {
        SpaceNode space;
        BodyNode body;
        HitboxNode hitbox;
        TextNode text;
    };

    Node(NodeType type = NodeType::basic);
    ~Node();

    void add_child(NodeOwnerPtr& child_node);
    void recalculate_model_matrix();
    void recalculate_render_data();

    const NodeType type() const;

    const std::vector<Node*>& children();

    glm::dvec2 position();
    glm::dvec2 absolute_position();
    glm::dvec2 get_relative_position(const Node* const ancestor);
    void position(const glm::dvec2& position);

    double rotation();
    double absolute_rotation();
    void rotation(const double& rotation);

    glm::dvec2 scale();
    glm::dvec2 absolute_scale();
    void scale(const glm::dvec2& scale);

    Transformation absolute_transformation();
    Transformation get_relative_transformation(const Node* const ancestor);

    int16_t z_index();
    void z_index(const int16_t& z_index);

    Shape shape();
    void shape(const Shape& shape);

    Sprite sprite();
    void sprite(const Sprite& sprite);

    glm::dvec4 color();
    void color(const glm::dvec4& color);

    bool visible();
    void visible(const bool& visible);

    Alignment origin_alignment();
    void origin_alignment(const Alignment& alignment);

    uint32_t lifetime();
    void lifetime(const uint32_t& lifetime);

    NodeTransitionHandle transition();
    void transition(const NodeTransitionHandle& transition);

    NodeTransitionsManager& transitions_manager();

    Scene* const scene() const;
    NodePtr parent() const;

    void setup_wrapper(std::unique_ptr<ForeignNodeWrapper>&& wrapper);
    ForeignNodeWrapper* wrapper_ptr() const;
};

template<class... Args>
NodeOwnerPtr
make_node(Args&&... args)
{
    return NodeOwnerPtr{new Node(std::forward<Args>(args)...)};
}

} // namespace kaacore

#pragma once

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "kaacore/fonts.h"
#include "kaacore/geometry.h"
#include "kaacore/node_ptr.h"
#include "kaacore/physics.h"
#include "kaacore/renderer.h"
#include "kaacore/shapes.h"
#include "kaacore/spatial_index.h"
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
    virtual void on_attach() = 0;
    virtual void on_detach() = 0;
};

struct Scene;

class Node {
  public:
    union {
        SpaceNode space;
        BodyNode body;
        HitboxNode hitbox;
        TextNode text;
    };

    Node(NodeType type = NodeType::basic);
    ~Node();

    NodePtr add_child(NodeOwnerPtr& child_node);
    void recalculate_model_matrix();
    void recalculate_render_data();
    void recalculate_ordering_data();

    const NodeType type() const;

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

    Transformation transformation();
    void transformation(const Transformation& transformation);

    std::optional<int16_t> z_index();
    void z_index(const std::optional<int16_t>& z_index);

    Shape shape();
    void shape(const Shape& shape);
    void shape(const Shape& shape, bool is_auto_shape);

    Sprite sprite();
    void sprite(const Sprite& sprite);

    glm::dvec4 color();
    void color(const glm::dvec4& color);

    bool visible();
    void visible(const bool& visible);

    Alignment origin_alignment();
    void origin_alignment(const Alignment& alignment);

    Duration lifetime();
    void lifetime(const Duration& lifetime);

    NodeTransitionHandle transition();
    void transition(const NodeTransitionHandle& transition);

    NodeTransitionsManager& transitions_manager();

    Scene* const scene() const;
    NodePtr parent() const;
    const std::vector<Node*>& children();
    bool is_root() const;

    void views(const std::optional<std::unordered_set<int16_t>>& z_indices);
    const std::optional<std::vector<int16_t>> views() const;

    void setup_wrapper(std::unique_ptr<ForeignNodeWrapper>&& wrapper);
    ForeignNodeWrapper* wrapper_ptr() const;

    void indexable(const bool indexable_flag);
    bool indexable() const;

    BoundingBox<double> bounding_box();

  private:
    const NodeType _type = NodeType::basic;
    glm::dvec2 _position = {0., 0.};
    double _rotation = 0.;
    glm::dvec2 _scale = {1., 1.};
    std::optional<int16_t> _z_index = std::nullopt;
    Shape _shape;
    bool _auto_shape = true;
    Sprite _sprite;
    glm::dvec4 _color = {1., 1., 1., 1.};
    bool _visible = true;
    Alignment _origin_alignment = Alignment::none;
    HighPrecisionDuration _lifetime = 0us;
    NodeTransitionsManager _transitions_manager;

    Scene* _scene = nullptr;
    Node* _parent = nullptr;
    std::vector<Node*> _children;
    std::optional<ViewIndexSet> _views = std::nullopt;

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
    struct {
        ViewIndexSet calculated_views;
        int16_t calculated_z_index;
        bool is_dirty = true;
    } _ordering_data;

    bool _indexable = true;
    NodeSpatialData _spatial_data;

    bool _marked_to_delete = false;

    void _mark_dirty();
    void _mark_ordering_dirty();
    void _mark_to_delete();
    glm::fmat4 _compute_model_matrix(const glm::fmat4& parent_matrix) const;
    glm::fmat4 _compute_model_matrix_cumulative(
        const Node* const ancestor = nullptr) const;
    void _recalculate_model_matrix();
    void _recalculate_model_matrix_cumulative();
    void _set_position(const glm::dvec2& position);
    void _set_rotation(const double rotation);

    friend class _NodePtrBase;
    friend class NodePtr;
    friend class NodeOwnerPtr;
    friend struct Scene;
    friend struct SpaceNode;
    friend struct BodyNode;
    friend struct HitboxNode;
    friend struct NodeSpatialData;
    friend class SpatialIndex;
    friend constexpr Node* container_node(const NodeSpatialData*);
};

template<class... Args>
NodeOwnerPtr
make_node(Args&&... args)
{
    return NodeOwnerPtr{new Node(std::forward<Args>(args)...)};
}

} // namespace kaacore

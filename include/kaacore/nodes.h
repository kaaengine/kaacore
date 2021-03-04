#pragma once

#include <deque>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "kaacore/draw_unit.h"
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
    void recalculate_ordering_data();
    void recalculate_visibility_data();
    VerticesIndicesVectorPair recalculate_vertices_indices_data();

    bool has_draw_unit_updates() const;
    std::optional<DrawUnitModification> calculate_draw_unit_removal() const;
    DrawUnitModificationPair calculate_draw_unit_updates();
    void clear_draw_unit_updates(const std::optional<const DrawBucketKey> key);

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
    int16_t effective_z_index();

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
    std::vector<Node*> children();
    bool is_root() const;

    void views(const std::optional<std::unordered_set<int16_t>>& z_indices);
    const std::optional<std::vector<int16_t>> views() const;
    const std::vector<int16_t> effective_views();

    void setup_wrapper(std::unique_ptr<ForeignNodeWrapper>&& wrapper);
    ForeignNodeWrapper* wrapper_ptr() const;

    void indexable(const bool indexable_flag);
    bool indexable() const;

    uint16_t root_distance() const;

    uint64_t scene_tree_id() const;

    BoundingBox<double> bounding_box();

    template<typename Func>
    void recursive_call(Func&& func)
    {
        thread_local std::deque<Node*> nodes_to_process;
        nodes_to_process.clear();

        nodes_to_process.push_back(this);
        while (not nodes_to_process.empty()) {
            Node* node = nodes_to_process.front();
            nodes_to_process.pop_front();
            if constexpr (std::is_same_v<
                              std::invoke_result_t<Func, Node*>, void>) {
                // if provided Func does return void then
                // always proceed with the recursive calling to children
                func(node);
            } else if (not func(node)) {
                // returning false stops recursive calling
                // for this node's children
                continue;
            }
            nodes_to_process.insert(
                nodes_to_process.end(), node->children().begin(),
                node->children().end());
        }
    }

    template<typename Pred>
    const std::vector<Node*>& build_inheritance_chain(Pred&& pred)
    // Builds an inheritance chain vector going up to the root.
    // It continues as long as currently processed node
    // meets the predicate `pred` - if it fails node won't
    // be added to chain and processing stops.
    // Nodes in vector are ordered desecending by `root_distance` -
    // starting node will at the first position (if not empty).

    {
        thread_local std::vector<Node*> inheritance_chain;
        inheritance_chain.clear();

        Node* node = this;
        while (node != nullptr and pred(node)) {
            inheritance_chain.push_back(node);
            node = node->_parent;
        }

        return inheritance_chain;
    }

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
    uint64_t _scene_tree_id = 0;
    Node* _parent = nullptr;
    std::vector<Node*> _children;
    std::optional<ViewIndexSet> _views = std::nullopt;
    uint16_t _root_distance = 0;

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
    struct {
        bool calculated_visible;
        bool is_dirty = true;
    } _visibility_data;
    struct {
        bool updated_bucket_key = true;
        bool updated_vertices_indices_info = true;
        std::optional<DrawBucketKey> current_key;
    } _draw_unit_data;

    bool _indexable = true;
    NodeSpatialData _spatial_data;

    bool _marked_to_delete = false;

    void _mark_dirty();
    void _mark_ordering_dirty();
    void _mark_draw_unit_vertices_indices_dirty();
    void _mark_to_delete();
    glm::fmat4 _compute_model_matrix(const glm::fmat4& parent_matrix) const;
    glm::fmat4 _compute_model_matrix_cumulative(
        const Node* const ancestor = nullptr) const;
    void _recalculate_model_matrix();
    void _recalculate_model_matrix_cumulative();
    void _set_position(const glm::dvec2& position);
    void _set_rotation(const double rotation);

    DrawBucketKey _make_draw_bucket_key() const;

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

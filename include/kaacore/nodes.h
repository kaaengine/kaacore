#pragma once

#include <bitset>
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
#include "kaacore/materials.h"
#include "kaacore/node_ptr.h"
#include "kaacore/physics.h"
#include "kaacore/renderer.h"
#include "kaacore/resources.h"
#include "kaacore/shapes.h"
#include "kaacore/spatial_index.h"
#include "kaacore/sprites.h"
#include "kaacore/transitions.h"
#include "kaacore/viewports.h"

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
    typedef std::bitset<16> DirtyFlagsType;

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

    std::optional<DrawUnitModification> calculate_draw_unit_removal() const;
    DrawUnitModificationPack calculate_draw_unit_updates();
    void clear_draw_unit_updates(const std::optional<const DrawBucketKey> key);

    void set_dirty_flags(const DirtyFlagsType flags);
    void clear_dirty_flags(const DirtyFlagsType flags);
    bool query_dirty_flags(const DirtyFlagsType flags);

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

    ResourceReference<Material>& material();
    void material(const ResourceReference<Material>& material);

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

    void render_passes(
        const std::optional<std::unordered_set<int16_t>>& indices);
    const std::optional<std::vector<int16_t>> render_passes() const;
    const std::vector<int16_t> effective_render_passes();

    void viewports(const std::optional<std::unordered_set<int16_t>>& z_indices);
    const std::optional<std::vector<int16_t>> viewports() const;
    const std::vector<int16_t> effective_viewports();

    void setup_wrapper(std::unique_ptr<ForeignNodeWrapper>&& wrapper);
    ForeignNodeWrapper* wrapper_ptr() const;

    void indexable(const bool indexable_flag);
    bool indexable() const;

    uint16_t root_distance() const;

    uint64_t scene_tree_id() const;

    BoundingBox<double> bounding_box();

    template<typename Func>
    void recursive_call_downstream(Func&& func)
    {
        if constexpr (std::is_same_v<std::invoke_result_t<Func, Node*>, void>) {
            // if provided Func does return void then
            // always proceed with the recursive calling to children
            func(this);
        } else if (not func(this)) {
            // returning false stops recursive calling
            // for this node's children
            return;
        }
        this->recursive_call_downstream_children(func);
    }

    template<typename Func>
    void recursive_call_downstream_children(Func&& func)
    {
        thread_local std::deque<Node*> nodes_to_process;
        nodes_to_process.clear();

        nodes_to_process.insert(
            nodes_to_process.begin(), this->_children.begin(),
            this->_children.end());

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
                nodes_to_process.end(), node->_children.begin(),
                node->_children.end());
        }
    }

    template<typename Func>
    void recursive_call_upstream(Func&& func)
    {
        Node* node = this;
        while (node) {
            if constexpr (std::is_same_v<
                              std::invoke_result_t<Func, Node*>, void>) {
                func(node);
            } else if (not func(node)) {
                break;
            }
            node = node->_parent;
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

    static constexpr size_t DIRTY_FLAGS_SHIFT_RECURSIVE = 8;

    static inline const DirtyFlagsType DIRTY_MODEL_MATRIX = 1u << 0;
    static inline const DirtyFlagsType DIRTY_DRAW_KEYS = 1u << 1;
    static inline const DirtyFlagsType DIRTY_DRAW_VERTICES = 1u << 2;
    static inline const DirtyFlagsType DIRTY_VISIBILITY = 1u << 3;
    static inline const DirtyFlagsType DIRTY_ORDERING = 1u << 4;
    static inline const DirtyFlagsType DIRTY_SPATIAL_INDEX = 1u << 5;

    static inline const DirtyFlagsType DIRTY_MODEL_MATRIX_RECURSIVE =
        DIRTY_MODEL_MATRIX | DIRTY_MODEL_MATRIX << DIRTY_FLAGS_SHIFT_RECURSIVE;
    static inline const DirtyFlagsType DIRTY_DRAW_KEYS_RECURSIVE =
        DIRTY_DRAW_KEYS | DIRTY_DRAW_KEYS << DIRTY_FLAGS_SHIFT_RECURSIVE;
    static inline const DirtyFlagsType DIRTY_DRAW_VERTICES_RECURSIVE =
        DIRTY_DRAW_VERTICES | DIRTY_DRAW_VERTICES
                                  << DIRTY_FLAGS_SHIFT_RECURSIVE;
    static inline const DirtyFlagsType DIRTY_VISIBILITY_RECURSIVE =
        DIRTY_VISIBILITY | DIRTY_VISIBILITY << DIRTY_FLAGS_SHIFT_RECURSIVE;
    static inline const DirtyFlagsType DIRTY_ORDERING_RECURSIVE =
        DIRTY_ORDERING | DIRTY_ORDERING << DIRTY_FLAGS_SHIFT_RECURSIVE;
    static inline const DirtyFlagsType DIRTY_SPATIAL_INDEX_RECURSIVE =
        DIRTY_SPATIAL_INDEX | DIRTY_SPATIAL_INDEX
                                  << DIRTY_FLAGS_SHIFT_RECURSIVE;

    static inline const DirtyFlagsType DIRTY_ANY_RECURSIVE =
        DIRTY_MODEL_MATRIX << DIRTY_FLAGS_SHIFT_RECURSIVE |
        DIRTY_DRAW_KEYS << DIRTY_FLAGS_SHIFT_RECURSIVE |
        DIRTY_DRAW_VERTICES << DIRTY_FLAGS_SHIFT_RECURSIVE |
        DIRTY_VISIBILITY << DIRTY_FLAGS_SHIFT_RECURSIVE |
        DIRTY_ORDERING << DIRTY_FLAGS_SHIFT_RECURSIVE |
        DIRTY_SPATIAL_INDEX << DIRTY_FLAGS_SHIFT_RECURSIVE;

    static inline const DirtyFlagsType DIRTY_ALL =
        DIRTY_MODEL_MATRIX_RECURSIVE | DIRTY_DRAW_KEYS_RECURSIVE |
        DIRTY_DRAW_VERTICES_RECURSIVE | DIRTY_VISIBILITY_RECURSIVE |
        DIRTY_ORDERING_RECURSIVE | DIRTY_SPATIAL_INDEX_RECURSIVE;

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
    ResourceReference<Material> _material;

    Scene* _scene = nullptr;
    uint64_t _scene_tree_id = 0;
    Node* _parent = nullptr;
    std::vector<Node*> _children;
    std::optional<RenderPassIndexSet> _render_passes = std::nullopt;
    std::optional<ViewportIndexSet> _viewports = std::nullopt;
    uint16_t _root_distance = 0;

    std::unique_ptr<ForeignNodeWrapper> _node_wrapper;

    struct {
        glm::fmat4 value;
    } _model_matrix;
    struct {
        RenderPassIndexSet calculated_render_passes;
        ViewportIndexSet calculated_viewports;
        int16_t calculated_z_index;
    } _ordering_data;
    struct {
        bool calculated_visible;
    } _visibility_data;
    struct {
        std::optional<DrawBucketKey> current_key;
    } _draw_unit_data;

    bool _indexable = false;
    NodeSpatialData _spatial_data;

    bool _marked_to_delete = false;
    bool _in_hitbox_chain = false;
    DirtyFlagsType _dirty_flags = DIRTY_ALL;

    void _mark_to_delete();
    glm::fmat4 _compute_model_matrix(const glm::fmat4& parent_matrix) const;
    glm::fmat4 _compute_model_matrix_cumulative(
        const Node* const ancestor = nullptr) const;
    void _recalculate_model_matrix();
    void _recalculate_model_matrix_cumulative();
    void _set_position(const glm::dvec2& position);
    void _set_rotation(const double rotation);
    void _update_hitboxes();

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

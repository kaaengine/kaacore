#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <bgfx/bgfx.h>

#include "kaacore/renderer.h"
#include "kaacore/geometry.h"
#include "kaacore/shapes.h"
#include "kaacore/physics.h"
#include "kaacore/fonts.h"
#include "kaacore/sprites.h"


namespace kaacore {

enum struct NodeType {
    basic = 1,
    space = 2,
    body = 3,
    hitbox = 4,
    text = 5,
};


struct NodeRenderData {
    std::vector<StandardVertexData> computed_vertices;
    bgfx::TextureHandle texture_handle;
};


struct ForeignNodeWrapper {
    ForeignNodeWrapper() = default;
    virtual ~ForeignNodeWrapper() = default;
};


struct MyForeignWrapper : ForeignNodeWrapper {
    MyForeignWrapper();
    ~MyForeignWrapper();
};


struct Scene;

class Node {
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

    Scene* _scene = nullptr;
    Node* _parent = nullptr;
    std::vector<Node*> _children;

    std::unique_ptr<ForeignNodeWrapper> _node_wrapper;

    NodeRenderData _render_data;
    glm::fmat4 _matrix;

    public:
    union {
        SpaceNode space;
        BodyNode body;
        HitboxNode hitbox;
        TextNode text;
    };

    Node(NodeType type = NodeType::basic);
    ~Node();

    void add_child(Node* child_node);
    void recalculate_matrix();
    void recalculate_render_data();

    const NodeType type() const;

    const std::vector<Node*>& children();

    glm::dvec2 position();
    glm::dvec2 absolute_position();
    void position(const glm::dvec2& position);

    double rotation();
    void rotation(const double& rotation);

    glm::dvec2 scale();
    void scale(const glm::dvec2& scale);

    int16_t z_index();
    void z_index(const int16_t& z_index);

    Shape shape();
    void shape(const Shape& shape);

    Sprite& sprite_ref();
    void sprite(const Sprite& sprite);

    glm::dvec4 color();
    void color(const glm::dvec4& color);

    bool visible();
    void visible(const bool& visible);

    Alignment origin_alignment();
    void origin_alignment(const Alignment& alignment);

    uint32_t lifetime();
    void lifetime(const uint32_t& lifetime);

    Scene* scene() const;
    Node* parent() const;

    void setup_wrapper(std::unique_ptr<ForeignNodeWrapper>&& wrapper);
    ForeignNodeWrapper* wrapper_ptr() const;
};

} // namespace kaacore

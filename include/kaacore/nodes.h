#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include "kaacore/renderer.h"
#include "kaacore/shape.h"


struct NodeRenderData {
    std::vector<StandardVertexData> computed_vertices;
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

struct Node {
    glm::dvec2 position = {0., 0.};
    double rotation = 0.;
    glm::dvec2 scale = {1., 1.};
    int16_t z_index = 0;
    Shape shape;

    Scene* scene = nullptr;
    Node* parent = nullptr;
    std::vector<Node*> children;

    std::unique_ptr<ForeignNodeWrapper> node_wrapper;

    NodeRenderData render_data;
    glm::fmat4 matrix;

    Node() = default;
    Node(double x, double y);
    ~Node();

    void add_child(Node* child_node);
    void recalculate_matrix();
    void recalculate_render_data();

    glm::dvec2 get_absolute_position();

    void repr() const;
};

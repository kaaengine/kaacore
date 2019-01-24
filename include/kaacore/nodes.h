#pragma once

#include <vector>
#include <memory>

#include "kaacore/renderer.h"


struct dvec2 {
    double x, y;
};

struct Shape {
    std::vector<double> points;
    std::vector<uint16_t> indices;
    std::vector<StandardVertexData> vertices;

};

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


struct Node {
    dvec2 position;
    double rotation;

    Node* parent;
    std::vector<Node*> children;

    std::unique_ptr<ForeignNodeWrapper> node_wrapper;

    Node() = default;
    Node(double x, double y);

    void add_child(Node& child_node);

    void repr() const;
};

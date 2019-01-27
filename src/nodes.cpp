#include <iostream>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include "kaacore/nodes.h"


Node::Node(double _x, double _y) {
    this->position.x = _x;
    this->position.y = _y;
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

    for (Node* child_node : this->children) {
        delete child_node;
    }
}

void Node::repr() const {
    std::cout << "X: " << this->position.x << ", Y: " << this->position.y << std::endl;
}

void Node::add_child(Node* child_node)
{
    assert(child_node->parent == nullptr);
    child_node->parent = this;
    this->children.push_back(child_node);
    child_node->scene = this->scene;
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
    glm::dvec4 pos;
    for (auto& vertex : this->render_data.computed_vertices) {
        pos = {vertex.xyz.x, vertex.xyz.y, vertex.xyz.z, 1.};
        pos = this->matrix * pos;
        vertex.xyz.x = pos.x;
        vertex.xyz.y = pos.y;
        vertex.xyz.z = pos.z;
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

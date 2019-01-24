#include <iostream>

#include "kaacore/nodes.h"

Node::Node(double _x, double _y) {
    this->position.x = _x;
    this->position.y = _y;
}

void Node::repr() const {
    std::cout << "X: " << this->position.x << ", Y: " << this->position.y << std::endl;
}


MyForeignWrapper::MyForeignWrapper() {
    std::cout << "MyForeignWrapper ctor!" << std::endl;
}


MyForeignWrapper::~MyForeignWrapper() {
    std::cout << "MyForeignWrapper dtor!" << std::endl;
}

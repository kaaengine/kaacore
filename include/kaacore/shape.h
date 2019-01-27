#pragma once

#include <glm/glm.hpp>

#include "kaacore/renderer.h"


enum struct ShapeType {
    none = 0,
    segment,
    circle,
    polygon,
    freeform,
};


struct Shape {
    ShapeType type;
    std::vector<glm::dvec2> points;
    double radius;

    std::vector<VertexIndex> indices;
    std::vector<StandardVertexData> vertices;

    Shape()
    : type(ShapeType::none) {};
    Shape(const ShapeType type, const std::vector<glm::dvec2> points,
          const double radius,
          const std::vector<VertexIndex> indices,
          const std::vector<StandardVertexData> vertices)
    : type(type), points(points), radius(radius),
      indices(indices), vertices(vertices) {};

    static Shape Segment(const glm::dvec2 a, const glm::dvec2 b);
    static Shape Circle(const glm::dvec2 center, const double radius);
    static Shape Box(const glm::dvec2 size);
};
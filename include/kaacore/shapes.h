#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "kaacore/renderer.h"


namespace kaacore {

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
    Shape(const ShapeType type, const std::vector<glm::dvec2>& points,
          const double radius,
          const std::vector<VertexIndex>& indices,
          const std::vector<StandardVertexData>& vertices)
    : type(type), points(points), radius(radius),
      indices(indices), vertices(vertices) {};

    inline operator bool() const {return this->type != ShapeType::none;}

    static Shape Segment(const glm::dvec2 a, const glm::dvec2 b);
    static Shape Circle(const double radius, const glm::dvec2 center);
    static Shape Circle(const double radius);
    static Shape Box(const glm::dvec2 size);
    static Shape Polygon(const std::vector<glm::dvec2>& points);
    static Shape Freeform(const std::vector<VertexIndex>& indices,
                          const std::vector<StandardVertexData>& vertices);
};

} // namespace kaacore

#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "kaacore/geometry.h"
#include "kaacore/renderer.h"
#include "kaacore/utils.h"

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
    BoundingBox<double> vertices_bbox;
    std::vector<glm::dvec2> bounding_points;

    Shape() : type(ShapeType::none){};
    Shape(
        const ShapeType type, const std::vector<glm::dvec2>& points,
        const double radius, const std::vector<VertexIndex>& indices,
        const std::vector<StandardVertexData>& vertices,
        const std::vector<glm::dvec2>& bounding_points);

    inline operator bool() const { return this->type != ShapeType::none; }
    bool operator==(const Shape& other);

    static Shape Segment(const glm::dvec2 a, const glm::dvec2 b);
    static Shape Circle(const double radius, const glm::dvec2 center);
    static Shape Circle(const double radius);
    static Shape Box(const glm::dvec2 size);
    static Shape Polygon(const std::vector<glm::dvec2>& points);
    static Shape Freeform(
        const std::vector<VertexIndex>& indices,
        const std::vector<StandardVertexData>& vertices);

    Shape transform(const Transformation& transformation) const;
    bool contains_point(const glm::dvec2 point) const;
};

} // namespace kaacore

namespace std {
using kaacore::hash_combined;
using kaacore::hash_iterable;
using kaacore::Shape;
using kaacore::StandardVertexData;
using kaacore::VertexIndex;

template<>
struct hash<Shape> {
    size_t operator()(const Shape& shape) const
    {
        return hash_combined(
            shape.type,
            hash_iterable<glm::dvec2, std::vector<glm::dvec2>::const_iterator>(
                shape.points.begin(), shape.points.end()),
            hash_iterable<
                VertexIndex, std::vector<VertexIndex>::const_iterator>(
                shape.indices.begin(), shape.indices.end()),
            hash_iterable<
                StandardVertexData,
                std::vector<StandardVertexData>::const_iterator>(
                shape.vertices.begin(), shape.vertices.end()),
            shape.radius);
    }
};
}

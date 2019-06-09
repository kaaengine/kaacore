#include <algorithm>
#include <tuple>

#include <glm/glm.hpp>

#include "kaacore/geometry.h"
#include "kaacore/exceptions.h"

#include "kaacore/shapes.h"


namespace kaacore {

Shape::Shape(const ShapeType type, const std::vector<glm::dvec2>& points,
             const double radius,
             const std::vector<VertexIndex>& indices,
             const std::vector<StandardVertexData>& vertices)
    : type(type), points(points), radius(radius),
      indices(indices), vertices(vertices)
{
    for (const auto& vt : vertices) {
        this->vertices_bbox.add_point(vt.xyz);
    }
};

Shape Shape::Segment(const glm::dvec2 a, const glm::dvec2 b)
{
    const double radius = 1;

    const glm::dvec2 distance = b - a;
    // 90-degrees rotation (counter-clockwise)
    const glm::dvec2 rot = glm::normalize(glm::dvec2(distance.y, -distance.x));

    const glm::dvec2 a0 = a + rot * radius;
    const glm::dvec2 a1 = a - rot * radius;
    const glm::dvec2 b0 = b + rot * radius;
    const glm::dvec2 b1 = b - rot * radius;

    const std::vector<glm::dvec2> points = {a, b};

    const std::vector<StandardVertexData> vertices = {
        StandardVertexData(a0.x, a0.y, 0., 0.),
        StandardVertexData(b0.x, b0.y, 1., 0.),
        StandardVertexData(b1.x, b1.y, 1., 1.),
        StandardVertexData(a1.x, a1.y, 0., 1.)
    };

    const std::vector<VertexIndex> indices = {0, 2, 1, 0, 3, 2};

    return Shape(ShapeType::segment, points, radius, indices, vertices);
}

Shape Shape::Circle(const double radius, const glm::dvec2 center)
{
    const std::vector<glm::dvec2> points = {center};

    const std::vector<StandardVertexData> vertices = {
        StandardVertexData::XY_UV_MN(center.x - radius, center.y - radius, 0., 0., -0.5, -0.5),
        StandardVertexData::XY_UV_MN(center.x + radius, center.y - radius, 1., 0., +0.5, -0.5),
        StandardVertexData::XY_UV_MN(center.x + radius, center.y + radius, 1., 1., +0.5, +0.5),
        StandardVertexData::XY_UV_MN(center.x - radius, center.y + radius, 0., 1., -0.5, +0.5)
    };

    const std::vector<VertexIndex> indices = {0, 2, 1, 0, 3, 2};

    return Shape(ShapeType::circle, points, radius, indices, vertices);
}

Shape Shape::Circle(const double radius)
{
    return Shape::Circle(radius, glm::dvec2(0., 0.));
}

Shape Shape::Box(const glm::dvec2 size)
{
    const std::vector<glm::dvec2> points = {
        {-0.5 * size.x, -0.5 * size.y},
        {+0.5 * size.x, -0.5 * size.y},
        {+0.5 * size.x, +0.5 * size.y},
        {-0.5 * size.x, +0.5 * size.y}
    };

    const std::vector<StandardVertexData> vertices = {
        StandardVertexData::XY_UV(-0.5 * size.x, -0.5 * size.y, 0., 0.),
        StandardVertexData::XY_UV(+0.5 * size.x, -0.5 * size.y, 1., 0.),
        StandardVertexData::XY_UV(+0.5 * size.x, +0.5 * size.y, 1., 1.),
        StandardVertexData::XY_UV(-0.5 * size.x, +0.5 * size.y, 0., 1.)
    };

    const std::vector<VertexIndex> indices = {0, 2, 1, 0, 3, 2};

    return Shape(ShapeType::polygon, points, 0., indices, vertices);
}

Shape Shape::Polygon(const std::vector<glm::dvec2>& points)
{
    auto polygon_type = classify_polygon(points);
    KAACORE_CHECK(polygon_type != PolygonType::not_convex);

    std::vector<glm::dvec2> polygon_points = points;

    if (polygon_points.front() == polygon_points.back()) {
        polygon_points.pop_back();
    }

    if (polygon_type == PolygonType::convex_cw) {
        std::reverse(polygon_points.begin(), polygon_points.end());
    }

    auto points_count = polygon_points.size();
    std::vector<StandardVertexData> vertices;
    vertices.reserve(points_count + 1);
    std::vector<VertexIndex> indices;
    indices.reserve(points_count * 3);
    glm::dvec2 center_point = find_points_center(points);
    VertexIndex center_point_index = points_count;

    glm::dvec2 min_pt, max_pt;
    std::tie(min_pt, max_pt) = find_points_minmax(polygon_points);

    for (VertexIndex idx = 0 ; idx < points_count ; idx++) {
        const auto& pt = polygon_points[idx];
        vertices.emplace_back(pt.x, pt.y);
        indices.push_back(idx);
        indices.push_back((idx + 1) % points_count);
        indices.push_back(center_point_index);
    }

    vertices.emplace_back(center_point.x, center_point.y);

    // unlerp vertices uv values to correspond to xy positions inside polygon
    for (auto& vertex : vertices) {
        vertex.uv.x = (vertex.xyz.x - min_pt.x) / (max_pt.x - min_pt.x);
        vertex.uv.y = (vertex.xyz.y - min_pt.y) / (max_pt.y - min_pt.y);
    }

    return Shape(ShapeType::polygon, polygon_points, 0., indices, vertices);
}

Shape Shape::Freeform(const std::vector<VertexIndex>& indices,
                      const std::vector<StandardVertexData>& vertices)
{
    return Shape(ShapeType::freeform, {}, 0., indices, vertices);
}

} // namespace kaacore

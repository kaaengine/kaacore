#include <glm/glm.hpp>

#include "kaacore/shapes.h"


namespace kaacore {

Shape Shape::Segment(const glm::dvec2 a, const glm::dvec2 b)
{
    const double radius = 0.01;

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
    // FIXME We assume that polygon is convex, make a check for that
    auto points_count = points.size();
    glm::dvec2 points_sum = {0., 0.};
    double min_x = points[0].x;
    double max_x = points[0].x;
    double min_y = points[0].y;
    double max_y = points[0].y;
    std::vector<StandardVertexData> vertices;
    vertices.reserve(points_count + 1);
    std::vector<VertexIndex> indices;
    indices.reserve(points_count * 3);
    VertexIndex center_point_index = points_count;

    for (VertexIndex idx = 0 ; idx < points_count ; idx++) {
        const auto& pt = points[idx];
        points_sum += pt;
        min_x = glm::min(min_x, pt.x);
        max_x = glm::max(max_x, pt.x);
        min_y = glm::min(min_y, pt.y);
        max_y = glm::max(max_y, pt.y);
        vertices.emplace_back(pt.x, pt.y);
        indices.push_back(idx);
        indices.push_back((idx + 1) % points_count);
        indices.push_back(center_point_index);
    }

    glm::dvec2 center_point = {points_sum.x / points_count,
                               points_sum.y / points_count};
    vertices.emplace_back(center_point.x, center_point.y);

    // unlerp vertices uv values to correspond to xy positions inside polygon
    for (auto& vertex : vertices) {
        vertex.uv.x = (vertex.xyz.x - min_x) / (max_x - min_x);
        vertex.uv.y = (vertex.xyz.y - min_y) / (max_y - min_y);
    }

    return Shape(ShapeType::polygon, points, 0., indices, vertices);
}

Shape Shape::Freeform(const std::vector<VertexIndex>& indices,
                      const std::vector<StandardVertexData>& vertices)
{
    return Shape(ShapeType::freeform, {}, 0., indices, vertices);
}

} // namespace kaacore

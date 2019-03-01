#include <glm/glm.hpp>

#include "kaacore/shape.h"


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

Shape Shape::Circle(const glm::dvec2 center, const double radius)
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

Shape Shape::Freeform(const std::vector<VertexIndex>& indices,
                      const std::vector<StandardVertexData>& vertices)
{
    return Shape(ShapeType::freeform, {}, 0., indices, vertices);
}

} // namespace kaacore

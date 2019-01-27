#include <glm/glm.hpp>

#include "kaacore/shape.h"


Shape Shape::Circle(const glm::dvec2 center, const double radius)
{
    const std::vector<glm::dvec2> points = {center};

    const std::vector<StandardVertexData> vertices = {
        StandardVertexData(center.x - radius, center.y - radius, 0., 0., 0., -0.5, -0.5),
        StandardVertexData(center.x + radius, center.y - radius, 0., 1., 0., +0.5, -0.5),
        StandardVertexData(center.x + radius, center.y + radius, 0., 1., 1., +0.5, +0.5),
        StandardVertexData(center.x - radius, center.y + radius, 0., 0., 1., -0.5, +0.5)
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
        StandardVertexData(-0.5 * size.x, -0.5 * size.y, 0., 0.),
        StandardVertexData(+0.5 * size.x, -0.5 * size.y, 1., 0.),
        StandardVertexData(+0.5 * size.x, +0.5 * size.y, 1., 1.),
        StandardVertexData(-0.5 * size.x, +0.5 * size.y, 0., 1.)
    };

    const std::vector<VertexIndex> indices = {0, 2, 1, 0, 3, 2};

    return Shape(ShapeType::polygon, points, 0., indices, vertices);
}

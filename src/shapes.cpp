#include <algorithm>
#include <tuple>

#include <glm/glm.hpp>

#include "kaacore/exceptions.h"
#include "kaacore/geometry.h"

#include "kaacore/shapes.h"

namespace kaacore {

constexpr int circle_shape_generated_points_count = 24;

Shape::Shape(
    const ShapeType type, const std::vector<glm::dvec2>& points,
    const double radius, const std::vector<VertexIndex>& indices,
    const std::vector<StandardVertexData>& vertices,
    const std::vector<glm::dvec2>& bounding_points)
    : type(type), points(points), radius(radius), indices(indices),
      vertices(vertices),
      vertices_bbox(BoundingBox<double>::from_points(bounding_points)),
      bounding_points(bounding_points)
{
    KAACORE_ASSERT(
        classify_polygon(this->bounding_points) == PolygonType::convex_ccw,
        "Invalid shape - expected convex counterclockwise polygon.");
};

bool
Shape::operator==(const Shape& other)
{
    return (
        this->type == other.type and this->points == other.points and
        this->radius == other.radius and this->indices == other.indices and
        this->vertices == other.vertices);
}

BoundingBox<double>
Shape::bounding_box() const
{
    return this->vertices_bbox;
}

Shape
Shape::Segment(const glm::dvec2 a, const glm::dvec2 b)
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
        StandardVertexData::xy_uv(a0.x, a0.y, 0., 0.),
        StandardVertexData::xy_uv(b0.x, b0.y, 1., 0.),
        StandardVertexData::xy_uv(b1.x, b1.y, 1., 1.),
        StandardVertexData::xy_uv(a1.x, a1.y, 0., 1.)};

    const std::vector<VertexIndex> indices = {0, 2, 1, 0, 3, 2};

    const std::vector<glm::dvec2> bounding_points = {a0, b0, b1, a1};

    return Shape(
        ShapeType::segment, points, radius, indices, vertices, bounding_points);
}

Shape
Shape::Circle(const double radius, const glm::dvec2 center)
{
    const std::vector<glm::dvec2> points = {center};

    const std::vector<StandardVertexData> vertices = {
        StandardVertexData::xy_uv_mn(
            center.x - radius, center.y - radius, 0., 0., -0.5, -0.5),
        StandardVertexData::xy_uv_mn(
            center.x + radius, center.y - radius, 1., 0., +0.5, -0.5),
        StandardVertexData::xy_uv_mn(
            center.x + radius, center.y + radius, 1., 1., +0.5, +0.5),
        StandardVertexData::xy_uv_mn(
            center.x - radius, center.y + radius, 0., 1., -0.5, +0.5)};

    const std::vector<VertexIndex> indices = {0, 2, 1, 0, 3, 2};

    std::vector<glm::dvec2> bounding_points;
    bounding_points.reserve(circle_shape_generated_points_count);
    for (int i = circle_shape_generated_points_count - 1; i >= 0; i--) {
        double t = (2 * M_PI / circle_shape_generated_points_count) * i;
        bounding_points.push_back(glm::dvec2{center.x + radius * std::sin(t),
                                             center.y + radius * std::cos(t)});
    }

    return Shape(
        ShapeType::circle, points, radius, indices, vertices, bounding_points);
}

Shape
Shape::Circle(const double radius)
{
    return Shape::Circle(radius, glm::dvec2(0., 0.));
}

Shape
Shape::Box(const glm::dvec2 size)
{
    const std::vector<glm::dvec2> points = {{-0.5 * size.x, -0.5 * size.y},
                                            {+0.5 * size.x, -0.5 * size.y},
                                            {+0.5 * size.x, +0.5 * size.y},
                                            {-0.5 * size.x, +0.5 * size.y}};

    const std::vector<StandardVertexData> vertices = {
        StandardVertexData::xy_uv(-0.5 * size.x, -0.5 * size.y, 0., 0.),
        StandardVertexData::xy_uv(+0.5 * size.x, -0.5 * size.y, 1., 0.),
        StandardVertexData::xy_uv(+0.5 * size.x, +0.5 * size.y, 1., 1.),
        StandardVertexData::xy_uv(-0.5 * size.x, +0.5 * size.y, 0., 1.)};

    const std::vector<VertexIndex> indices = {0, 2, 1, 0, 3, 2};

    return Shape(ShapeType::polygon, points, 0., indices, vertices, points);
}

Shape
Shape::Polygon(const std::vector<glm::dvec2>& points)
{
    auto polygon_type = classify_polygon(points);
    KAACORE_CHECK(
        polygon_type != PolygonType::not_convex, "Convex polygon required.");

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

    for (VertexIndex idx = 0; idx < points_count; idx++) {
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

    return Shape(
        ShapeType::polygon, polygon_points, 0., indices, vertices,
        polygon_points);
}

Shape
Shape::Freeform(
    const std::vector<VertexIndex>& indices,
    const std::vector<StandardVertexData>& vertices)
{
    std::vector<glm::dvec2> vertices_points;
    vertices_points.reserve(vertices.size());
    for (const auto& vt : vertices) {
        vertices_points.push_back({vt.xyz.x, vt.xyz.y});
    }
    auto [min_pt, max_pt] = find_points_minmax(vertices_points);
    const std::vector<glm::dvec2> bounding_points = {{min_pt.x, min_pt.y},
                                                     {max_pt.x, min_pt.y},
                                                     {max_pt.x, max_pt.y},
                                                     {min_pt.x, max_pt.y}};
    return Shape(
        ShapeType::freeform, {}, 0., indices, vertices, bounding_points);
}

Shape
Shape::transform(const Transformation& transformation) const
{
    auto points = this->points;
    for (auto& pt : points) {
        pt = pt | transformation;
    }

    auto radius = this->radius;
    if (radius != 0.) {
        glm::dvec2 scale_ratio = glm::abs(transformation.decompose().scale);
        if (glm::epsilonNotEqual<double>(scale_ratio.x, scale_ratio.y, 1e-10)) {
            throw kaacore::exception(
                "Cannot transform shape radius by non-equal scale");
        }
        radius *= scale_ratio.x;
    }

    auto vertices = this->vertices;
    for (auto& vt : vertices) {
        auto tmp_pt = glm::dvec2(vt.xyz.x, vt.xyz.y);
        tmp_pt |= transformation;
        vt.xyz.x = tmp_pt.x;
        vt.xyz.y = tmp_pt.y;
    }

    std::vector<glm::dvec2> new_bounding_points;
    new_bounding_points.resize(this->bounding_points.size());
    std::transform(
        this->bounding_points.begin(), this->bounding_points.end(),
        new_bounding_points.begin(),
        [&transformation](const glm::dvec2 pt) -> glm::dvec2 {
            return pt | transformation;
        });

    return Shape(
        this->type, points, radius, this->indices, vertices,
        new_bounding_points);
}

bool
Shape::contains_point(const glm::dvec2 point) const
{
    return check_point_in_polygon(this->bounding_points, point);
}

} // namespace kaacore

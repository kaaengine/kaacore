#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "kaacore/exceptions.h"

#include "kaacore/geometry.h"

namespace kaacore {

Transformation::Transformation() : Transformation(glm::dmat4(1.)) {}

Transformation::Transformation(const glm::dmat4& matrix) : _matrix(matrix) {}

Transformation
Transformation::translate(const glm::dvec2& tr)
{
    return Transformation{glm::translate(glm::dvec3(tr.x, tr.y, 0.))};
}

Transformation
Transformation::scale(const glm::dvec2& sc)
{
    return Transformation{glm::scale(glm::dvec3(sc.x, sc.y, 1.))};
}

Transformation
Transformation::rotate(const double& r)
{
    return Transformation{glm::rotate(r, glm::dvec3(0., 0., 1.))};
}

Transformation
Transformation::inverse() const
{
    return Transformation{glm::inverse(this->_matrix)};
}

double
Transformation::at(const size_t col, const size_t row) const
{
    KAACORE_ASSERT(col < 4 and col >= 0);
    KAACORE_ASSERT(row < 4 and row >= 0);
    return this->_matrix[col][row];
}

const DecomposedTransformation<double>
Transformation::decompose() const
{
    return DecomposedTransformation<double>{this->_matrix};
}

Transformation
operator|(const Transformation& left, const Transformation& right)
{
    return Transformation{right._matrix * left._matrix};
}

glm::dvec2
operator|(const glm::dvec2& position, const Transformation& transformation)
{
    return transformation._matrix * glm::dvec4(position, 0., 1.);
}

Transformation&
operator|=(Transformation& transformation, const Transformation& other)
{
    transformation = Transformation{other._matrix * transformation._matrix};
    return transformation;
}

glm::dvec2&
operator|=(glm::dvec2& position, const Transformation& transformation)
{
    position = transformation._matrix * glm::dvec4(position, 0., 1.);
    return position;
}

int8_t
compare_points(const glm::dvec2 p, const glm::dvec2 q)
{
    if (p.x < q.x) {
        return -1;
    } else if (p.x > q.x) {
        return 1;
    } else if (p.y < q.y) {
        return -1;
    } else if (p.y > q.y) {
        return 1;
    } else {
        return 0;
    }
}

int8_t
detect_turn(const glm::dvec2 p, const glm::dvec2 q, const glm::dvec2 r)
{
    // equivalent to: ((p-q) x (q-r)).z
    double norm_z = (p.x - q.x) * (q.y - r.y) - (p.y - q.y) * (q.x - r.x);
    if (norm_z < 0) {
        return -1; // CW turn
    } else if (norm_z > 0) {
        return 1; // CCW turn
    } else {
        return 0; // no turn
    }
}

PolygonType
classify_polygon(const std::vector<glm::dvec2>& points)
{
    auto points_count = points.size();
    int8_t cur_dir = 0;
    int8_t angle_sign = 0;
    uint8_t dir_changes = 0;

    for (size_t idx = 0; idx < points_count; idx++) {
        const glm::dvec2& pt_1 = points[idx % points_count];
        const glm::dvec2& pt_2 = points[(idx + 1) % points_count];
        const glm::dvec2& pt_3 = points[(idx + 2) % points_count];

        auto this_dir = compare_points(pt_2, pt_3);
        if (this_dir == -cur_dir) {
            dir_changes++;
            if (dir_changes > 2) {
                return PolygonType::not_convex;
            }
        }
        cur_dir = this_dir;

        auto this_sign = detect_turn(pt_1, pt_2, pt_3);
        if (this_sign) {
            if (this_sign == -angle_sign) {
                return PolygonType::not_convex;
            }
            angle_sign = this_sign;
        }
    }

    if (angle_sign > 0) {
        return PolygonType::convex_ccw;
    } else if (angle_sign < 0) {
        return PolygonType::convex_cw;
    } else {
        return PolygonType::not_convex;
    }
}

glm::dvec2
find_points_center(const std::vector<glm::dvec2>& points)
{
    auto sum =
        std::accumulate(points.begin(), points.end(), glm::dvec2{0., 0.});
    return sum * (1. / points.size());
}

std::pair<glm::dvec2, glm::dvec2>
find_points_minmax(const std::vector<glm::dvec2>& points)
{
    KAACORE_CHECK(points.size() > 0);
    glm::dvec2 min_pt = points[0];
    glm::dvec2 max_pt = points[0];
    for (const auto& pt : points) {
        min_pt.x = glm::min(min_pt.x, pt.x);
        max_pt.x = glm::max(max_pt.x, pt.x);
        min_pt.y = glm::min(min_pt.y, pt.y);
        max_pt.y = glm::max(max_pt.y, pt.y);
    }
    return std::make_pair(min_pt, max_pt);
}

} // namespace kaacore

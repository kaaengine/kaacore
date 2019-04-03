#pragma once

#include <utility>
#include <vector>

#include <glm/glm.hpp>


namespace kaacore {

enum struct PolygonType {
    convex_cw = 1,
    convex_ccw = 2,
    not_convex = 10
};

PolygonType classify_polygon(const std::vector<glm::dvec2>& points);
glm::dvec2 find_points_center(const std::vector<glm::dvec2>& points);

} // namespace kaacore

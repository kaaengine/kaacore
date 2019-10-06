#pragma once

#include <cmath>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

namespace kaacore {

enum struct PolygonType { convex_cw = 1, convex_ccw = 2, not_convex = 10 };

// 0bXX.. (bits 3-4) - alignment of X axis
// 0b..XX (bits 1-2) - alignment of Y axis

const uint8_t Alignment_x_coord_mask = 0b1100;
const uint8_t Alignment_y_coord_mask = 0b0011;

enum struct Alignment {
    // alignment values:
    // - 01 - align to minimal value (left or top side)
    // - 10 - align to maximal value (right or bottom side)
    // - 11 - align to mean value
    none = 0b0000,
    top = 0b1110,
    bottom = 0b1101,
    left = 0b1011,
    right = 0b0111,
    top_left = 0b1010,
    bottom_left = 0b1001,
    top_right = 0b0110,
    bottom_right = 0b0101,
    center = 0b1111,
};

inline constexpr uint8_t operator&(const Alignment alignment, uint8_t mask)
{
    return uint8_t(alignment) & mask;
}

template<typename T>
struct BoundingBox {
    T min_x, min_y, max_x, max_y;

    BoundingBox()
        : min_x(std::nan("")), max_x(std::nan("")), min_y(std::nan("")),
          max_y(std::nan(""))
    {}

    void add_point(glm::vec<2, T> point)
    {
        if (std::isnan(this->min_x) or this->min_x > point.x) {
            this->min_x = point.x;
        }
        if (std::isnan(this->max_x) or this->max_x < point.x) {
            this->max_x = point.x;
        }
        if (std::isnan(this->min_y) or this->min_y > point.y) {
            this->min_y = point.y;
        }
        if (std::isnan(this->max_y) or this->max_y < point.y) {
            this->max_y = point.y;
        }
    }

    operator bool() const
    {
        return (
            !std::isnan(this->min_x) and !std::isnan(this->max_x) and
            !std::isnan(this->min_y) and !std::isnan(this->max_y));
    }
};

template<typename T>
glm::vec<2, T>
calculate_realignment_vector(Alignment alignment, const BoundingBox<T>& bbox)
{
    if (alignment == Alignment::none or not bbox) {
        return {0., 0.};
    }

    T align_x;
    switch (alignment & Alignment_x_coord_mask) {
        case (Alignment::center & Alignment_x_coord_mask):
            align_x = -(bbox.min_x + bbox.max_x) / 2.;
            break;
        case (Alignment::left & Alignment_x_coord_mask):
            align_x = -bbox.min_x;
            break;
        case (Alignment::right & Alignment_x_coord_mask):
            align_x = -bbox.max_x;
            break;
    }

    T align_y;
    switch (alignment & Alignment_y_coord_mask) {
        case (Alignment::center & Alignment_y_coord_mask):
            align_y = -(bbox.min_y + bbox.max_y) / 2.;
            break;
        case (Alignment::top & Alignment_y_coord_mask):
            align_y = -bbox.min_y;
            break;
        case (Alignment::bottom & Alignment_y_coord_mask):
            align_y = -bbox.max_y;
            break;
    }

    return {align_x, align_y};
}

PolygonType
classify_polygon(const std::vector<glm::dvec2>& points);
glm::dvec2
find_points_center(const std::vector<glm::dvec2>& points);
std::pair<glm::dvec2, glm::dvec2>
find_points_minmax(const std::vector<glm::dvec2>& points);

} // namespace kaacore

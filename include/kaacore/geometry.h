#pragma once

#include <utility>
#include <vector>
#include <cmath>

#include <glm/glm.hpp>


namespace kaacore {

enum struct PolygonType {
    convex_cw = 1,
    convex_ccw = 2,
    not_convex = 10
};


enum struct Alignment {
    // 0b..XX (bits 1-2) - alignment of X axis
    // 0bXX.. (bits 3-4) - alignment of Y axis
    // alignment values:
    // - 01 - align to minimal value (left or top side)
    // - 10 - align to maximal value (right or bottom side)
    // - 11 - align to mean value
    none = 0b0000,
    top = 0b1011,
    bottom = 0b0111,
    left = 0b1110,
    right = 0b1101,
    top_left = 0b1010,
    bottom_left = 0b0110,
    top_right = 0b1001,
    bottom_right = 0b0101,
    center = 0b1111,
};


template <typename T>
struct BoundingBox {
    T min_x, min_y, max_x, max_y;

    BoundingBox()
    : min_x(std::nan("")),
      max_x(std::nan("")),
      min_y(std::nan("")),
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
        return (!std::isnan(this->min_x) and
                !std::isnan(this->max_x) and
                !std::isnan(this->min_y) and
                !std::isnan(this->max_y));
    }
};


template <typename T>
glm::vec<2, T> calculate_realignment_vector(Alignment alignment, const BoundingBox<T>& bbox)
{
    if (alignment == Alignment::none or not bbox) {
        return {0., 0.};
    }

    T align_x;
    if ((uint8_t(alignment) & 0b0011) == 0b0010) {
        align_x = -bbox.min_x;
    } else if ((uint8_t(alignment) & 0b0011) == 0b0001) {
        align_x = -bbox.max_x;
    } else {
        align_x = -(bbox.min_x + bbox.max_x) / 2.;
    }

    T align_y;
    if ((uint8_t(alignment) & 0b1100) == 0b1000) {
        align_y = -bbox.min_y;
    } else if ((uint8_t(alignment) & 0b1100) == 0b0100) {
        align_y = -bbox.max_y;
    } else {
        align_y = -(bbox.min_y + bbox.max_y) / 2.;
    }
    return {align_x, align_y};
}


PolygonType classify_polygon(const std::vector<glm::dvec2>& points);
glm::dvec2 find_points_center(const std::vector<glm::dvec2>& points);
std::pair<glm::dvec2, glm::dvec2> find_points_minmax(const std::vector<glm::dvec2>& points);

} // namespace kaacore

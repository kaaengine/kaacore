#pragma once

#include <cmath>
#include <utility>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "kaacore/utils.h"

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

enum struct AngleSign {
    mixed = 1,
    negative = 2,
    positive = 3,
};

inline constexpr uint8_t operator&(
    const Alignment alignment, const uint8_t mask)
{
    return uint8_t(alignment) & mask;
}

template<typename T>
struct DecomposedTransformation {
    glm::tvec2<T> scale;
    double rotation;
    glm::tvec2<T> translation;

    DecomposedTransformation(
        const glm::tmat4x4<T>& matrix = glm::tmat4x4<T>(1.))
    {
        glm::tvec3<T> _scale;
        glm::tvec3<T> _translation;
        glm::tquat<T> _rotation_quat;
        glm::tvec3<T> _skew;
        glm::tvec4<T> _perspective;
        glm::decompose(
            matrix, _scale, _rotation_quat, _translation, _skew, _perspective);
        this->scale = _scale;
        this->rotation = glm::eulerAngles(_rotation_quat).z;
        this->translation = _translation;
    }
};

class Transformation {
  public:
    Transformation();
    Transformation(const glm::dmat4& matrix);
    bool operator==(Transformation const& other) const;

    static Transformation translate(const glm::dvec2& tr);
    static Transformation scale(const glm::dvec2& sc);
    static Transformation rotate(const double& r);

    Transformation inverse() const;

    double at(const size_t col, const size_t row) const;
    const DecomposedTransformation<double> decompose() const;

  private:
    glm::dmat4 _matrix;

    friend Transformation operator|(
        const Transformation& left, const Transformation& right);
    friend glm::dvec2 operator|(
        const glm::dvec2& position, const Transformation& transformation);
    friend Transformation& operator|=(
        Transformation& left, const Transformation& right);
    friend glm::dvec2& operator|=(
        glm::dvec2& position, const Transformation& transformation);
};

Transformation
operator|(const Transformation& left, const Transformation& right);
glm::dvec2
operator|(const glm::dvec2& position, const Transformation& transformation);

Transformation&
operator|=(Transformation& left, const Transformation& right);
glm::dvec2&
operator|=(glm::dvec2& position, const Transformation& transformation);

template<typename T>
struct BoundingBox {
    T min_x, min_y, max_x, max_y;

    BoundingBox()
        : BoundingBox(std::nan(""), std::nan(""), std::nan(""), std::nan(""))
    {}

    BoundingBox(const T min_x, const T min_y, const T max_x, const T max_y)
        : min_x(min_x), min_y(min_y), max_x(max_x), max_y(max_y)
    {}

    bool operator==(const BoundingBox<T>& other) const
    {
        return (
            this->min_x == other.min_x and this->min_y == other.min_y and
            this->max_x == other.max_x and this->max_y == other.max_y);
    }

    bool is_nan() const
    {
        return (
            std::isnan(this->min_x) or std::isnan(this->max_x) or
            std::isnan(this->min_y) or std::isnan(this->max_y));
    }

    BoundingBox<T> merge(const BoundingBox<T>& other) const
    {
        return BoundingBox<T>{glm::min(this->min_x, other.min_x),
                              glm::min(this->min_y, other.min_y),
                              glm::max(this->max_x, other.max_x),
                              glm::max(this->max_y, other.max_y)};
    }

    bool contains(const BoundingBox<T>& bbox) const
    {
        return (
            this->min_x <= bbox.min_x and this->max_x >= bbox.max_x and
            this->min_y <= bbox.min_y and this->max_y >= bbox.max_y);
    }

    bool contains(const glm::tvec2<T> point) const
    {
        return (
            this->min_x <= point.x and this->max_x >= point.x and
            this->min_y <= point.y and this->max_y >= point.y);
    }

    bool intersects(const BoundingBox<T>& other) const
    {
        return (
            this->min_x <= other.max_x and other.min_x <= this->max_x and
            this->min_y <= other.max_y and other.min_y <= this->max_y);
    }

    BoundingBox intersection(const BoundingBox<T>& other) const
    {
        auto min_x = glm::max<T>(this->min_x, other.min_x);
        auto min_y = glm::max<T>(this->min_y, other.min_y);

        auto max_x = glm::min<T>(this->max_x, other.max_x);
        auto max_y = glm::min<T>(this->max_y, other.max_y);

        if (min_x > max_x || min_y > max_y) {
            return BoundingBox<T>();
        }
        return BoundingBox<T>(min_x, min_y, max_x, max_y);
    }

    BoundingBox<T> grow(glm::tvec2<T> vec) const
    {
        return BoundingBox<T>{this->min_x - vec.x, this->min_y - vec.y,
                              this->max_x + vec.x, this->max_y + vec.y};
    }

    glm::tvec2<T> center() const
    {
        return {(this->max_x + this->min_x) / 2,
                (this->max_y + this->min_y) / 2};
    }

    glm::tvec2<T> dimensions() const
    {
        return {this->max_x - this->min_x, this->max_y - this->min_y};
    }

    static BoundingBox<T> single_point(glm::tvec2<T> pt)
    {
        return BoundingBox<T>{pt.x, pt.y, pt.x, pt.y};
    }

    static BoundingBox<T> from_points(const std::vector<glm::tvec2<T>>& points)
    {
        BoundingBox<T> bbox;
        for (const auto& pt : points) {
            if (std::isnan(bbox.min_x) or bbox.min_x > pt.x) {
                bbox.min_x = pt.x;
            }
            if (std::isnan(bbox.max_x) or bbox.max_x < pt.x) {
                bbox.max_x = pt.x;
            }
            if (std::isnan(bbox.min_y) or bbox.min_y > pt.y) {
                bbox.min_y = pt.y;
            }
            if (std::isnan(bbox.max_y) or bbox.max_y < pt.y) {
                bbox.max_y = pt.y;
            }
        }

        return bbox;
    }
};

template<typename T>
glm::vec<2, T>
calculate_realignment_vector(Alignment alignment, const BoundingBox<T>& bbox)
{
    if (alignment == Alignment::none or bbox.is_nan()) {
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

bool
check_point_in_polygon(
    const std::vector<glm::dvec2>& polygon_points, const glm::dvec2 point);

PolygonType
classify_polygon(const std::vector<glm::dvec2>& points);
glm::dvec2
find_points_center(const std::vector<glm::dvec2>& points);
std::pair<glm::dvec2, glm::dvec2>
find_points_minmax(const std::vector<glm::dvec2>& points);

double
normalize_angle(const double value, const AngleSign sign = AngleSign::mixed);
double
normalize_angle_degrees(
    const double value, const AngleSign sign = AngleSign::mixed);

} // namespace kaacore

namespace std {

template<typename T>
struct hash<kaacore::BoundingBox<T>> {
    size_t operator()(const kaacore::BoundingBox<T>& bbox) const
    {
        return kaacore::hash_combined(
            bbox.min_x, bbox.min_y, bbox.max_x, bbox.max_y);
    }
};

} // namespace std

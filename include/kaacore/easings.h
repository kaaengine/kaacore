#pragma once

#include <glm/glm.hpp>

namespace kaacore {

enum class Easing {
    none = 0,
    back_in,
    back_in_out,
    back_out,
    bounce_in,
    bounce_in_out,
    bounce_out,
    circular_in,
    circular_in_out,
    circular_out,
    cubic_in,
    cubic_in_out,
    cubic_out,
    elastic_in,
    elastic_in_out,
    elastic_out,
    exponential_in,
    exponential_in_out,
    exponential_out,
    quadratic_in,
    quadratic_in_out,
    quadratic_out,
    quartic_in,
    quartic_in_out,
    quartic_out,
    quintic_in,
    quintic_in_out,
    quintic_out,
    sine_in,
    sine_in_out,
    sine_out,
};

double
apply_easing_function(const Easing easing, const double t);

template<typename T>
T
apply_easing_function(const Easing easing, const double t, const T a, const T b)
{
    const auto eased_t = apply_easing_function(easing, t);
    return glm::mix(a, b, eased_t);
}

} // namespace kaacore

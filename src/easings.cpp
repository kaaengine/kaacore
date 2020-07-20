#include <glm/gtx/easing.hpp>

#include "kaacore/easings.h"

namespace kaacore {

double
apply_easing_function(const Easing easing, const double t)
{
    switch (easing) {
        case Easing::none:
            return t;
        case Easing::back_in:
            return glm::backEaseIn(t);
        case Easing::back_in_out:
            return glm::backEaseInOut(t);
        case Easing::back_out:
            return glm::backEaseOut(t);
        case Easing::bounce_in:
            return glm::bounceEaseIn(t);
        case Easing::bounce_in_out:
            return glm::bounceEaseInOut(t);
        case Easing::bounce_out:
            return glm::bounceEaseOut(t);
        case Easing::circular_in:
            return glm::circularEaseIn(t);
        case Easing::circular_in_out:
            return glm::circularEaseInOut(t);
        case Easing::circular_out:
            return glm::circularEaseOut(t);
        case Easing::cubic_in:
            return glm::cubicEaseIn(t);
        case Easing::cubic_in_out:
            return glm::cubicEaseInOut(t);
        case Easing::cubic_out:
            return glm::cubicEaseOut(t);
        case Easing::elastic_in:
            return glm::elasticEaseIn(t);
        case Easing::elastic_in_out:
            return glm::elasticEaseInOut(t);
        case Easing::elastic_out:
            return glm::elasticEaseOut(t);
        case Easing::exponential_in:
            return glm::exponentialEaseIn(t);
        case Easing::exponential_in_out:
            return glm::exponentialEaseInOut(t);
        case Easing::exponential_out:
            return glm::exponentialEaseOut(t);
        case Easing::quadratic_in:
            return glm::quadraticEaseIn(t);
        case Easing::quadratic_in_out:
            return glm::quadraticEaseInOut(t);
        case Easing::quadratic_out:
            return glm::quadraticEaseOut(t);
        case Easing::quartic_in:
            return glm::quarticEaseIn(t);
        case Easing::quartic_in_out:
            return glm::quarticEaseInOut(t);
        case Easing::quartic_out:
            return glm::quarticEaseOut(t);
        case Easing::quintic_in:
            return glm::quinticEaseIn(t);
        case Easing::quintic_in_out:
            return glm::quinticEaseInOut(t);
        case Easing::quintic_out:
            return glm::quinticEaseOut(t);
        case Easing::sine_in:
            return glm::sineEaseIn(t);
        case Easing::sine_in_out:
            return glm::sineEaseInOut(t);
        case Easing::sine_out:
            return glm::sineEaseOut(t);
    }
}

} // namespace kaacore

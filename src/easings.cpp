#include <glm/gtx/easing.hpp>

#include "kaacore/easings.h"

namespace kaacore {

double
ease(const Easing easing, const double progress)
{
    switch (easing) {
        case Easing::back_in:
            return glm::backEaseIn(progress);
        case Easing::back_in_out:
            return glm::backEaseInOut(progress);
        case Easing::back_out:
            return glm::backEaseOut(progress);
        case Easing::bounce_in:
            return glm::bounceEaseIn(progress);
        case Easing::bounce_in_out:
            // XXX workaround for a buggy glm implementation,
            // remove when it get's fixed upstream
            if (progress < 0.5) {
                return (1. - glm::bounceEaseOut(1. - 2 * progress)) * 0.5;
            }
            return glm::bounceEaseInOut(progress);
        case Easing::bounce_out:
            return glm::bounceEaseOut(progress);
        case Easing::circular_in:
            return glm::circularEaseIn(progress);
        case Easing::circular_in_out:
            return glm::circularEaseInOut(progress);
        case Easing::circular_out:
            return glm::circularEaseOut(progress);
        case Easing::cubic_in:
            return glm::cubicEaseIn(progress);
        case Easing::cubic_in_out:
            return glm::cubicEaseInOut(progress);
        case Easing::cubic_out:
            return glm::cubicEaseOut(progress);
        case Easing::elastic_in:
            return glm::elasticEaseIn(progress);
        case Easing::elastic_in_out:
            return glm::elasticEaseInOut(progress);
        case Easing::elastic_out:
            return glm::elasticEaseOut(progress);
        case Easing::exponential_in:
            return glm::exponentialEaseIn(progress);
        case Easing::exponential_in_out:
            return glm::exponentialEaseInOut(progress);
        case Easing::exponential_out:
            return glm::exponentialEaseOut(progress);
        case Easing::quadratic_in:
            return glm::quadraticEaseIn(progress);
        case Easing::quadratic_in_out:
            return glm::quadraticEaseInOut(progress);
        case Easing::quadratic_out:
            return glm::quadraticEaseOut(progress);
        case Easing::quartic_in:
            return glm::quarticEaseIn(progress);
        case Easing::quartic_in_out:
            return glm::quarticEaseInOut(progress);
        case Easing::quartic_out:
            return glm::quarticEaseOut(progress);
        case Easing::quintic_in:
            return glm::quinticEaseIn(progress);
        case Easing::quintic_in_out:
            return glm::quinticEaseInOut(progress);
        case Easing::quintic_out:
            return glm::quinticEaseOut(progress);
        case Easing::sine_in:
            return glm::sineEaseIn(progress);
        case Easing::sine_in_out:
            return glm::sineEaseInOut(progress);
        case Easing::sine_out:
            return glm::sineEaseOut(progress);
        default:
            return progress;
    }
}

} // namespace kaacore

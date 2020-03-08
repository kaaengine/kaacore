#include <random>

#include "kaacore/utils.h"

namespace kaacore {

std::default_random_engine&
get_random_engine()
{
    thread_local std::default_random_engine random_engine{
        std::random_device{}()};
    return random_engine;
}

} // namespace kaacore

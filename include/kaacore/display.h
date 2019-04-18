
#pragma once

#include <string>

#include <glm/glm.hpp>

namespace kaacore {

struct Display {
    uint32_t index;
    std::string name;
    glm::uvec2 position;
    glm::uvec2 size;
};

} // namespace kaacore

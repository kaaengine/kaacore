
#pragma once

#include <string>

#include <glm/glm.hpp>

namespace kaacore {

struct Display {
    uint32_t index;
    std::string name;
    glm::ivec2 position;
    glm::ivec2 size;
};

} // namespace kaacore

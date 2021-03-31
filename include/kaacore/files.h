#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <bgfx/bgfx.h>
#include <bimg/decode.h>

namespace kaacore {

class File {
  public:
    const std::string path;
    std::vector<uint8_t> content;

    File(const std::string path) noexcept(false);
};

} // namespace kaacore

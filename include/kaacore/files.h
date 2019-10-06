#pragma once

#include <string>
#include <vector>

#include <bgfx/bgfx.h>
#include <bimg/decode.h>

namespace kaacore {

struct RawFile {
    std::string path;
    std::vector<uint8_t> content;

    RawFile(const std::string file_path) noexcept(false);
};

} // namespace kaacore

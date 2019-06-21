#pragma once

#include <vector>
#include <string>

#include <bimg/decode.h>
#include <bgfx/bgfx.h>


namespace kaacore {

struct RawFile {
    std::string path;
    std::vector<uint8_t> content;

    RawFile(const std::string file_path) noexcept(false);
};

} // namespace kaacore

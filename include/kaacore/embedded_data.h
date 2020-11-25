#pragma once

#include <string>
#include <utility>

#include <cmrc/cmrc.hpp>

#include "kaacore/exceptions.h"

namespace kaacore {

extern const cmrc::embedded_filesystem embedded_assets_filesystem;
extern const cmrc::embedded_filesystem embedded_shaders_filesystem;

struct embedded_file_error : kaacore::exception {
    using kaacore::exception::exception;
};

std::pair<const uint8_t*, size_t>
get_embedded_file_content(
    const cmrc::embedded_filesystem& filesystem, const std::string& path);

} // namespace kaacore

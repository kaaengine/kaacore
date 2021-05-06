#pragma once

#include <string>
#include <utility>

#include <cmrc/cmrc.hpp>

#include "kaacore/exceptions.h"
#include "kaacore/memory.h"

namespace kaacore {

extern const cmrc::embedded_filesystem embedded_assets_filesystem;
extern const cmrc::embedded_filesystem embedded_shaders_filesystem;

struct embedded_file_error : kaacore::exception {
    using kaacore::exception::exception;
};

Memory
get_embedded_file_content(
    const cmrc::embedded_filesystem& filesystem, const std::string& path);

} // namespace kaacore

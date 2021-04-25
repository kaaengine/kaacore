#pragma once

#include <string>

namespace kaacore {

enum class PlatformType { linux, osx, windows, unsupported };

PlatformType
get_platform();
std::string
get_platform_name();

} // namespace kaacore

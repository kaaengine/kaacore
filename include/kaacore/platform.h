#pragma once

#include <string>

namespace kaacore {

enum class PlatforType { linux, osx, windows, unsupported };

PlatforType
get_platform();
std::string
get_platform_name();

} // namespace kaacore

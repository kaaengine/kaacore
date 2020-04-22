#pragma once

#include <utility>

namespace kaacore {

std::pair<const uint8_t*, size_t>
get_embedded_file_content(const std::string& path);

} // namespace kaacore

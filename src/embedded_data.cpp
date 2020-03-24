#include <utility>

#include <cmrc/cmrc.hpp>

#include "kaacore/embedded_data.h"

CMRC_DECLARE(kaacore_embedded_assets);

namespace kaacore {

std::pair<const uint8_t*, size_t>
get_embedded_file_content(const std::string& path)
{
    auto embedded_fs = cmrc::kaacore_embedded_assets::get_filesystem();
    auto file = embedded_fs.open(path);
    return {reinterpret_cast<const uint8_t*>(file.cbegin()), file.size()};
}

} // namespace kaacore

#include <mutex>
#include <utility>

#include <cmrc/cmrc.hpp>

#include "kaacore/log.h"

#include "kaacore/embedded_data.h"

CMRC_DECLARE(kaacore_embedded_assets);
CMRC_DECLARE(kaacore_embedded_shaders);

namespace kaacore {

static std::mutex embedded_resource_load_mutex;
const cmrc::embedded_filesystem embedded_assets_filesystem =
    cmrc::kaacore_embedded_assets::get_filesystem();
const cmrc::embedded_filesystem embedded_shaders_filesystem =
    cmrc::kaacore_embedded_shaders::get_filesystem();

std::pair<const uint8_t*, size_t>
get_embedded_file_content(
    const cmrc::embedded_filesystem& filesystem, const std::string& path)
{
    std::lock_guard lock{embedded_resource_load_mutex};
    if (not filesystem.exists(path)) {
        throw embedded_file_error{
            fmt::format("Requested embedded file not found: '{}'", path)};
    }
    auto file = filesystem.open(path);
    return {reinterpret_cast<const uint8_t*>(file.cbegin()), file.size()};
}

} // namespace kaacore

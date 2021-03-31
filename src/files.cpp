#include <functional>
#include <string_view>

#include "kaacore/exceptions.h"
#include "kaacore/log.h"

#include "kaacore/files.h"

namespace kaacore {

File::File(const std::string path) noexcept(false) : path(path)
{
    KAACORE_LOG_INFO("Reading file: {}", path);
    std::ifstream f(path, std::ifstream::binary);
    if (f.fail()) {
        throw std::ios_base::failure("Failed to open file: " + path);
    }
    f.seekg(0, std::ios::end);
    auto len = f.tellg();
    f.seekg(0, std::ios::beg);
    this->content.resize(len);
    f.read(reinterpret_cast<char*>(this->content.data()), len);
}
} // namespace kaacore

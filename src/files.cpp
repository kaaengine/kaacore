#include <fstream>
#include <string>
#include <vector>

#include "kaacore/exceptions.h"
#include "kaacore/log.h"

#include "kaacore/files.h"

namespace kaacore {

RawFile::RawFile(const std::string file_path) noexcept(false) : path(file_path)
{
    KAACORE_LOG_INFO("Reading file: {}", file_path);
    std::ifstream f(file_path, std::ifstream::binary);
    if (f.fail()) {
        throw exception("Failed to open file: " + this->path);
    }
    f.seekg(0, std::ios::end);
    auto len = f.tellg();
    f.seekg(0, std::ios::beg);
    this->content.resize(len);
    f.read(reinterpret_cast<char*>(this->content.data()), len);
}

} // namespace kaacore

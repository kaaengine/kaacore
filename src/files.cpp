#include <vector>
#include <string>
#include <fstream>

#include "kaacore/files.h"


RawFile::RawFile(const std::string file_path) : path(file_path)
{
    std::ifstream f(file_path, std::ifstream::binary);
    f.seekg(0, std::ios::end);
    auto len = f.tellg();
    f.seekg(0, std::ios::beg);
    this->content.resize(len);
    f.read(reinterpret_cast<char*>(this->content.data()), len);
}

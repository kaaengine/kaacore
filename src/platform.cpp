#include <cstring>

#include <SDL.h>

#include "kaacore/platform.h"

namespace kaacore {

PlatformType
get_platform()
{
    auto name = get_platform_name();
    if (name == "Linux") {
        return PlatformType::linux;
    } else if (name == "OSX") {
        return PlatformType::osx;
    } else if (name == "Windows") {
        return PlatformType::windows;
    } else {
        return PlatformType::unsupported;
    }
}

std::string
get_platform_name()
{
#if __LINUX__
    return "Linux";
#elif __MACOSX__
    return "OSX";
#elif __WIN32__ or __WINRT__
    return "Windows";
#else
    return "Unsupported";
#endif
}

}

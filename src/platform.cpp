#include <cstring>

#include <SDL.h>

#include "kaacore/platform.h"

namespace kaacore {

PlatformType
get_platform()
{
#if __LINUX__
    return PlatformType::linux;
#elif __MACOSX__
    return PlatformType::osx;
#elif __WIN32__ or __WINRT__
    return PlatformType::windows;
#else
    return PlatformType::unsupported;
#endif
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

} // namespace kaacore

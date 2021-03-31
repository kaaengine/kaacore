#include <cstring>

#include <SDL.h>

#include "kaacore/platform.h"

namespace kaacore {

PlatforType
get_platform()
{
    auto name = get_platform_name();
    if (name == "Linux") {
        return PlatforType::linux;
    } else if (name == "OSX") {
        return PlatforType::osx;
    } else if (name == "Windows") {
        return PlatforType::windows;
    } else {
        return PlatforType::unsupported;
    }
}

std::string
get_platform_name()
{
    std::string name = SDL_GetPlatform();
    if (name == "Linux") {
        return name;
    } else if (name == "Mac OS X") {
        return "OSX";
    } else if (name == "Windows" or name == "WinRT") {
        return "Windows";
    } else {
        return "Unsupported";
    }
}

}

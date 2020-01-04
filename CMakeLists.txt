cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)

project(kaacore)

option(KAACORE_SHARED "Build kaacore-shared" OFF)
option(KAACORE_BUILD_DEMOS "Build kaacore demos" ON)

add_subdirectory(third_party/bgfx)
target_compile_options(bgfx PUBLIC -DBGFX_CONFIG_MULTITHREADED=0)
if (TARGET astc-codec)
    set_target_properties(astc-codec PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

set(SDL_STATIC OFF CACHE BOOL "")
add_subdirectory(third_party/sdl2)
target_compile_options(SDL2 PUBLIC -DSDL_DISABLE_ANALYZE_MACROS)

set(SDL_MIXER_SHARED OFF CACHE BOOL "")
add_subdirectory(third_party/sdl_mixer)

set(BUILD_SHARED OFF CACHE BOOL "")
set(BUILD_DEMOS OFF CACHE BOOL "")
add_subdirectory(third_party/chipmunk2d)
unset(BUILD_SHARED)
unset(BUILD_DEMOS)

set(GLM_TEST_ENABLE OFF CACHE BOOL "")
add_subdirectory(third_party/glm)

add_subdirectory(third_party/stb)

set_target_properties(
    bgfx bimg bx chipmunk_static
    PROPERTIES POSITION_INDEPENDENT_CODE ON
)

add_subdirectory(src)

if (KAACORE_BUILD_DEMOS)
    add_subdirectory(demos)
endif()

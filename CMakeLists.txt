cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)

project(kaacore)

option(KAACORE_SHARED "Build kaacore-shared" OFF)
option(KAACORE_BUILD_DEMOS "Build kaacore demos" ON)

add_subdirectory(third_party/bgfx)

set(SDL_STATIC OFF CACHE BOOL "")
add_subdirectory(third_party/sdl2)

set(SDL_MIXER_SHARED OFF CACHE BOOL "")
add_subdirectory(third_party/sdl_mixer)

set(BUILD_SHARED OFF CACHE BOOL "")
set(BUILD_DEMOS OFF CACHE BOOL "")
add_subdirectory(third_party/chipmunk2d)
unset(BUILD_SHARED)
unset(BUILD_DEMOS)

set(GLM_TEST_ENABLE OFF CACHE BOOL "")
add_subdirectory(third_party/glm)

set_target_properties(
    bgfx bimg bx chipmunk_static
    PROPERTIES POSITION_INDEPENDENT_CODE ON
)

add_subdirectory(src)

if (KAACORE_BUILD_DEMOS)
    add_subdirectory(demos)
endif()

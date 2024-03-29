cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(CMakeRC)

project(kaacore)

option(KAACORE_SHARED "Build kaacore-shared" OFF)
option(KAACORE_BUILD_DEMOS "Build kaacore demos" ON)
option(KAACORE_BUILD_DEMOS_WITH_TSAN "Build kaacore demos with thread-sanitizer" OFF)
option(KAACORE_MULTITHREADING_MODE "Build kaacore in multithreading mode" ON)
option(KAACORE_BUILD_TESTS "Build kaacore tests" ON)

set(KAACORE_MAX_RENDER_PASSES 32 CACHE STRING "")
set(KAACORE_MAX_VIEWPORTS 32 CACHE STRING "")
set(BGFX_MAX_VIEWS 64 CACHE STRING "")

# force dynamic SDL build
set(SDL_SHARED_ENABLED_BY_DEFAULT ON)

add_subdirectory(third_party/bgfx)
target_compile_options(
    bgfx PUBLIC
    -DBGFX_CONFIG_MAX_VIEWS=${BGFX_MAX_VIEWS}
    -DBGFX_CONFIG_MULTITHREADED=$<BOOL:${KAACORE_MULTITHREADING_MODE}>
)
if (TARGET astc-codec)
    set_target_properties(astc-codec PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
if (TARGET tinyexr)
    set_target_properties(tinyexr PROPERTIES POSITION_INDEPENDENT_CODE ON)
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
add_subdirectory(third_party/kissnet)

add_subdirectory(third_party/spdlog)
set_target_properties(spdlog PROPERTIES POSITION_INDEPENDENT_CODE ON)

set_target_properties(
    bgfx bimg bx chipmunk_static
    PROPERTIES POSITION_INDEPENDENT_CODE ON
)

add_subdirectory(src)
target_link_libraries(kaacore PUBLIC bx bimg)

add_subdirectory(shaders)
target_link_libraries(kaacore PUBLIC kaacore_embedded_shaders)

cmrc_add_resource_library(
    kaacore_embedded_assets
    embedded_resources/font_munro/munro.ttf
)

set_target_properties(
    kaacore_embedded_assets
    PROPERTIES POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(kaacore PUBLIC kaacore_embedded_assets)

if (KAACORE_BUILD_TESTS)
    add_subdirectory(third_party/catch2)
    add_subdirectory(tests)
endif()

if (KAACORE_BUILD_DEMOS)
    add_subdirectory(demos)
endif()

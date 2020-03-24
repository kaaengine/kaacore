cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0077 NEW)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(CMakeRC)

project(kaacore)

option(KAACORE_SHARED "Build kaacore-shared" OFF)
option(KAACORE_BUILD_DEMOS "Build kaacore demos" ON)

# force static bgfx build
set(BUILD_SHARED_LIBS OFF)
# force dynamic SDL build
set(SDL_SHARED_ENABLED_BY_DEFAULT ON)

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

cmrc_add_resource_library(
    kaacore_embedded_assets

    shaders/binary/default_glsl_fragment_shader.bin
    shaders/binary/default_glsl_vertex_shader.bin
    shaders/binary/default_hlsl_d3d9_fragment_shader.bin
    shaders/binary/default_hlsl_d3d9_vertex_shader.bin
    shaders/binary/default_hlsl_d3d11_fragment_shader.bin
    shaders/binary/default_hlsl_d3d11_vertex_shader.bin

    embedded_resources/font_munro/munro.ttf
)

target_link_libraries(kaacore PUBLIC kaacore_embedded_assets)

if (KAACORE_BUILD_DEMOS)
    add_subdirectory(demos)
endif()

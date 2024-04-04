cmake_minimum_required(VERSION 3.13)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(KAACORE_DEFAULT_LOGGING_LEVEL "debug"
        CACHE STRING "Default level for loggers")
else()
    set(KAACORE_DEFAULT_LOGGING_LEVEL "info"
        CACHE STRING "Default level for loggers")
endif()

option(KAACORE_PROTECT_ASSERTS "Enable exceptions for asserts" ON)
option(KAACORE_PROTECT_CHECKS "Enable exceptions for checks" ON)

configure_file(
    ../include/kaacore/config.h.in
    ../include/kaacore/config.h @ONLY
)

set(SRC_CXX_FILES
    nodes.cpp
    node_ptr.cpp
    engine.cpp
    files.cpp
    log.cpp
    renderer.cpp
    textures.cpp
    input.cpp
    audio.cpp
    scenes.cpp
    shapes.cpp
    physics.cpp
    resources.cpp
    resources_manager.cpp
    sprites.cpp
    window.cpp
    geometry.cpp
    fonts.cpp
    timers.cpp
    transitions.cpp
    camera.cpp
    render_passes.cpp
    render_targets.cpp
    viewports.cpp
    spatial_index.cpp
    threading.cpp
    utils.cpp
    embedded_data.cpp
    easings.cpp
    shaders.cpp
    clock.cpp
    uniforms.cpp
    materials.cpp
    memory.cpp
    platform.cpp
    statistics.cpp
    draw_unit.cpp
    draw_queue.cpp
    vertex_layout.cpp
    stencil.cpp
    unicode_buffer.cpp
)

set(SRC_H_FILES
    ../include/kaacore/nodes.h
    ../include/kaacore/node_ptr.h
    ../include/kaacore/engine.h
    ../include/kaacore/files.h
    ../include/kaacore/log.h
    ../include/kaacore/renderer.h
    ../include/kaacore/textures.h
    ../include/kaacore/input.h
    ../include/kaacore/audio.h
    ../include/kaacore/scenes.h
    ../include/kaacore/shapes.h
    ../include/kaacore/physics.h
    ../include/kaacore/resources.h
    ../include/kaacore/resources_manager.h
    ../include/kaacore/sprites.h
    ../include/kaacore/window.h
    ../include/kaacore/geometry.h
    ../include/kaacore/display.h
    ../include/kaacore/fonts.h
    ../include/kaacore/timers.h
    ../include/kaacore/transitions.h
    ../include/kaacore/node_transitions.h
    ../include/kaacore/camera.h
    ../include/kaacore/render_passes.h
    ../include/kaacore/render_targets.h
    ../include/kaacore/viewports.h
    ../include/kaacore/spatial_index.h
    ../include/kaacore/threading.h
    ../include/kaacore/easings.h
    ../include/kaacore/shaders.h
    ../include/kaacore/clock.h
    ../include/kaacore/uniforms.h
    ../include/kaacore/materials.h
    ../include/kaacore/memory.h
    ../include/kaacore/platform.h
    ../include/kaacore/statistics.h
    ../include/kaacore/draw_unit.h
    ../include/kaacore/draw_queue.h
    ../include/kaacore/vertex_layout.h
    ../include/kaacore/stencil.h
    ../include/kaacore/unicode_buffer.h

    ../include/kaacore/utils.h
    ../include/kaacore/embedded_data.h
)

set(SRC_FILES ${SRC_CXX_FILES} ${SRC_H_FILES})
add_library(kaacore OBJECT ${SRC_FILES})
set_property(TARGET kaacore PROPERTY POSITION_INDEPENDENT_CODE 1)

if (MINGW)
target_link_libraries(kaacore PUBLIC -lmingw32)
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(kaacore PUBLIC /permissive-)
endif()
if (WIN32)
    target_link_libraries(kaacore PUBLIC SDL2main)
endif()

if (WIN32)
    # avoid including min max macros from Windows.h
    target_compile_definitions(kaacore PUBLIC NOMINMAX)
endif()

get_target_property(CHIPMUNK_SOURCE_DIR chipmunk_static SOURCE_DIR)
target_include_directories(kaacore PUBLIC
    $<BUILD_INTERFACE:${CHIPMUNK_SOURCE_DIR}/../include>
)

get_target_property(SDL_BINARY_DIR SDL2 BINARY_DIR)
target_include_directories(kaacore PUBLIC
    $<BUILD_INTERFACE:${SDL_BINARY_DIR}/include>
)

set_target_properties(
    kaacore
    PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(kaacore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>/../include
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../include
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(
    kaacore PUBLIC
    SDL2 SDL_mixer bgfx chipmunk_static glm $<TARGET_OBJECTS:SDL_mixer>
    stb $<TARGET_OBJECTS:stb> spdlog kissnet
)

if (KAACORE_SHARED)
    add_library(kaacore-shared SHARED $<TARGET_OBJECTS:kaacore>)
    set_target_properties(kaacore-shared PROPERTIES OUTPUT_NAME kaacore)
endif()

add_library(kaacore-static STATIC $<TARGET_OBJECTS:kaacore>)
set_target_properties(kaacore-static PROPERTIES OUTPUT_NAME kaacore)

if (KAACORE_SHARED)
    install(TARGETS kaacore-shared
            EXPORT kaacore-config
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
endif()

install(TARGETS kaacore-static
        EXPORT kaacore-config
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(FILES ${SRC_H_FILES} DESTINATION include/kaacore)
install(EXPORT kaacore-config DESTINATION lib/cmake/kaacore)

if (KAACORE_SHARED)
    export(TARGETS kaacore-shared FILE kaacore-config.cmake)
endif()
export(TARGETS kaacore-static FILE kaacore-config.cmake)

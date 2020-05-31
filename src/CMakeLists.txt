cmake_minimum_required(VERSION 3.13)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(_DEFAULT_KAACORE_LOGLEVEL "DEBUG")
else()
    set(_DEFAULT_KAACORE_LOGLEVEL "INFO")
endif()

set(KAACORE_LOGLEVEL_OVERRIDE ""
    CACHE STRING "Override all logging levels")

macro(kaacore_option_loglevel)
    if ("${KAACORE_LOGLEVEL_OVERRIDE}" STREQUAL "")
        set(${ARGV0}
            "${_DEFAULT_KAACORE_LOGLEVEL}"
            CACHE STRING "${ARGV1}")
    else()
        set(${ARGV0}
            "${KAACORE_LOGLEVEL_OVERRIDE}"
            CACHE STRING "${ARGV1}" FORCE)
    endif()

    if (NOT (
        "${${ARGV0}}" STREQUAL "DEBUG" OR
        "${${ARGV0}}" STREQUAL "INFO" OR
        "${${ARGV0}}" STREQUAL "WARN" OR
        "${${ARGV0}}" STREQUAL "ERROR" OR
        "${${ARGV0}}" STREQUAL "CRITICAL"
        )
    )
        message(
            FATAL_ERROR
            "Invalid logging option for ${ARGV0}: ${${ARGV0}}, "
            "Should be one of: DEBUG, INFO, WARN, ERROR, CRITICAL"
        )
    endif()
endmacro()

kaacore_option_loglevel(
    KAACORE_LOGLEVEL_ENGINE "Logging level for: engine"
)
kaacore_option_loglevel(
    KAACORE_LOGLEVEL_RENDERER "Logging level for: renderer"
)
kaacore_option_loglevel(
    KAACORE_LOGLEVEL_INPUT "Logging level for: input"
)
kaacore_option_loglevel(
    KAACORE_LOGLEVEL_AUDIO "Logging level for: audio"
)
kaacore_option_loglevel(
    KAACORE_LOGLEVEL_NODES "Logging level for: nodes"
)
kaacore_option_loglevel(
    KAACORE_LOGLEVEL_PHYSICS "Logging level for: physics"
)
kaacore_option_loglevel(
    KAACORE_LOGLEVEL_MISC "Logging level for: misc"
)

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
    images.cpp
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
    node_transitions.cpp
    camera.cpp
    views.cpp
    spatial_index.cpp
    threading.cpp
    utils.cpp
    embedded_data.cpp
)

set(SRC_H_FILES
    ../include/kaacore/nodes.h
    ../include/kaacore/node_ptr.h
    ../include/kaacore/engine.h
    ../include/kaacore/files.h
    ../include/kaacore/log.h
    ../include/kaacore/renderer.h
    ../include/kaacore/images.h
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
    ../include/kaacore/views.h
    ../include/kaacore/spatial_index.h
    ../include/kaacore/threading.h

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
    stb $<TARGET_OBJECTS:stb>
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

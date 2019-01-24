cmake_minimum_required(VERSION 3.12)

set(SRC_CXX_FILES
    nodes.cpp
    engine.cpp
    files.cpp
    log.cpp
    renderer.cpp
    texture_loader.cpp
)

set(SRC_H_FILES
    ../include/kaacore/nodes.h
    ../include/kaacore/engine.h
    ../include/kaacore/files.h
    ../include/kaacore/log.h
    ../include/kaacore/renderer.h
    ../include/kaacore/texture_loader.h

    ../include/kaacore/embedded_data.h
)

set(SRC_FILES ${SRC_CXX_FILES} ${SRC_H_FILES})
add_library(kaacore OBJECT ${SRC_FILES})
set_property(TARGET kaacore PROPERTY POSITION_INDEPENDENT_CODE 1)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(kaacore PUBLIC /permissive-)
endif()

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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../include
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(kaacore PUBLIC SDL2 SDL_mixer bgfx chipmunk_static glm_static)

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

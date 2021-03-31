cmake_minimum_required(VERSION 3.13)

if (NOT DEFINED KAACORE_SHADERC_PLATFORMS)
    if(MSVC)
        set(KAACORE_SHADERC_PLATFORMS dx9 dx11 glsl spirv)
    elseif(APPLE)
        set(KAACORE_SHADERC_PLATFORMS metal glsl spirv)
    else()
        set(KAACORE_SHADERC_PLATFORMS glsl spirv)
    endif()
endif()

cmrc_add_resource_library(
    kaacore_embedded_shaders
)
set_target_properties(
    kaacore_embedded_shaders
    PROPERTIES POSITION_INDEPENDENT_CODE ON
)

macro(add_embedded_shader)
    set(ARG_FILENAME ${ARGV0})
    set(ARG_SHADER_TYPE ${ARGV1})
    add_shader(
        ${ARG_FILENAME} ${ARG_SHADER_TYPE}
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}
        PLATFORMS ${KAACORE_SHADERC_PLATFORMS}
    )
    get_filename_component(FILENAME_NO_EXT "${ARG_FILENAME}" NAME_WE)
    foreach(SHADER_PLATFORM ${KAACORE_SHADERC_PLATFORMS})
        cmrc_add_resources(
            kaacore_embedded_shaders
            WHENCE ${CMAKE_CURRENT_BINARY_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_PLATFORM}/${FILENAME_NO_EXT}.bin
        )
    endforeach()
endmacro()

add_embedded_shader(vs_default.sc VERTEX)
add_embedded_shader(fs_default.sc FRAGMENT)
add_embedded_shader(fs_sdf_font.sc FRAGMENT)

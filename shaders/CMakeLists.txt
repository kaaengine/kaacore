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

function( add_shader ARG_FILE )
	# Parse arguments
	cmake_parse_arguments( ARG "FRAGMENT;VERTEX;COMPUTE" "OUTPUT;GLSL_VERSION;DX9_MODEL;DX11_MODEL" "INCLUDES" "PLATFORMS" ${ARGN} )

	# Get filename
	get_filename_component( FILENAME "${ARG_FILE}" NAME_WE )

	# Determine if fragment or vertex or compute
	if( ARG_FRAGMENT AND ARG_VERTEX AND ARG_COMPUTE )
		message( SEND_ERROR "add_shader cannot be called with all FRAGMENT and VERTEX and COMPUTE." )
		return()
	elseif( ARG_FRAGMENT AND ARG_VERTEX )
		message( SEND_ERROR "add_shader cannot be called with both FRAGMENT and VERTEX." )
		return()
	elseif( ARG_FRAGMENT AND ARG_COMPUTE )
		message( SEND_ERROR "add_shader cannot be called with both FRAGMENT and COMPUTE." )
		return()
	elseif( ARG_VERTEX AND ARG_COMPUTE )
		message( SEND_ERROR "add_shader cannot be called with both VERTEX and COMPUTE." )
		return()
	endif()

	if( ARG_FRAGMENT )
		set( TYPE "FRAGMENT" )
		set( D3D_PREFIX "ps" )
	elseif( ARG_VERTEX )
		set( TYPE "VERTEX" )
		set( D3D_PREFIX "vs" )
	elseif( ARG_COMPUTE )
		set( TYPE "COMPUTE" )
		set( D3D_PREFIX "cs" )
	else()
		message( SEND_ERROR "add_shader must be called with either FRAGMENT or VERTEX or COMPUTE." )
 		return()
	endif()

	# Determine compatible platforms
	if( ARG_PLATFORMS )
		set( PLATFORMS ${ARG_PLATFORMS} )
	else()
		if( MSVC )
			set( PLATFORMS dx9 dx11 glsl essl asm.js spirv )
		elseif( APPLE )
			set( PLATFORMS metal glsl essl asm.js spirv )
		else()
			set( PLATFORMS glsl essl asm.js spirv )
		endif()
	endif()

	# Build options
	set( BASE_OPTIONS
		FILE ${ARG_FILE}
		${TYPE}
		INCLUDES ${BGFX_DIR}/src ${ARG_INCLUDES}
	)

	# Parse profiles
	set( DX9_PROFILE PROFILE ${D3D_PREFIX}_3_0 )
	if( ARG_DX9_MODEL )
		set( DX9_PROFILE PROFILE ${D3D_PREFIX}_${ARG_DX9_MODEL} )
	endif()
	set( DX11_PROFILE PROFILE ${D3D_PREFIX}_5_0 )
	if( ARG_DX11_MODEL )
		set( DX11_PROFILE PROFILE ${D3D_PREFIX}_${ARG_DX11_MODEL} )
	endif()
	set( GLSL_PROFILE PROFILE 120 )
	if( ARG_COMPUTE )
		set( GLSL_PROFILE PROFILE 430 )
	endif()
	if( ARG_GLSL_VERSION )
		set( GLSL_PROFILE PROFILE ${ARG_GLSL_VERSION} )
	endif()
	set( SPIRV_PROFILE PROFILE spirv )

	# Add commands
	set( OUTPUTS "" )
	set( COMMANDS "" )
	foreach( PLATFORM ${PLATFORMS} )
		set( OPTIONS ${BASE_OPTIONS} )
		set( OUTPUT "${ARG_OUTPUT}/${PLATFORM}/${FILENAME}.bin" )
		get_filename_component( OUTPUT "${OUTPUT}" ABSOLUTE )

		if( "${PLATFORM}" STREQUAL "dx9" )
			list( APPEND OPTIONS
				WINDOWS
				${DX9_PROFILE}
				OUTPUT ${OUTPUT}
			)
		elseif( "${PLATFORM}" STREQUAL "dx11" )
			list( APPEND OPTIONS
				WINDOWS
				${DX11_PROFILE}
				OUTPUT ${OUTPUT}
			)
		elseif( "${PLATFORM}" STREQUAL "metal" )
			list( APPEND OPTIONS
				OSX
				PROFILE metal
				OUTPUT ${OUTPUT}
			)
		elseif( "${PLATFORM}" STREQUAL "glsl" )
			list( APPEND OPTIONS
				LINUX
				${GLSL_PROFILE}
				OUTPUT ${OUTPUT}
			)
		elseif( "${PLATFORM}" STREQUAL "essl" )
			list( APPEND OPTIONS
				ANDROID
				OUTPUT ${OUTPUT}
			)
		elseif( "${PLATFORM}" STREQUAL "asm.js" )
			list( APPEND OPTIONS
				ASM_JS
				OUTPUT ${OUTPUT}
			)
		elseif( "${PLATFORM}" STREQUAL "spirv" )
			list( APPEND OPTIONS
				LINUX
				${SPIRV_PROFILE}
				OUTPUT ${OUTPUT}
			)
		else()
			message( SEND_ERROR "add_shader given bad platform: ${PLATFORM}" )
			return()
		endif()

		list( APPEND OUTPUTS ${OUTPUT} )
		shaderc_parse( CMD ${OPTIONS} )
		list( APPEND COMMANDS COMMAND "${CMAKE_COMMAND}" -E make_directory "${ARG_OUTPUT}/${PLATFORM}" )
		list( APPEND COMMANDS COMMAND "$<TARGET_FILE:shaderc>" ${CMD} )
	endforeach()

	# Add command
	add_custom_command(
		MAIN_DEPENDENCY
		${ARG_FILE}
		OUTPUT
		${OUTPUTS}
		${COMMANDS}
		COMMENT "Compiling shader ${ARG_FILE}"
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)

	# Add to custom filter
	source_group( "Shader Files" FILES ${ARG_FILE} )
endfunction()

macro(add_embedded_shader)
    set(ARG_FILENAME ${ARGV0})
    set(ARG_SHADER_TYPE ${ARGV1})

    add_shader(
        ${ARG_FILENAME} ${ARG_SHADER_TYPE}
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}
        INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include
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

add_embedded_shader(vs_effect.sc VERTEX)
add_embedded_shader(vs_default.sc VERTEX)
add_embedded_shader(fs_default.sc FRAGMENT)
add_embedded_shader(fs_sdf_font.sc FRAGMENT)

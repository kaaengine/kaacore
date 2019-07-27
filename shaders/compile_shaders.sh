#!/bin/bash

set -xe

cd `dirname $0`

[[ -z ${SHADERC_BIN} ]] && SHADERC_BIN="shaderc"

[[ ! -d out ]] && mkdir out

compile_shader()
{
	${SHADERC_BIN} --verbose -i ../third_party/bgfx/bgfx/src/ $@
}

compile_vs()
{
	NAME=$1
	shift
	compile_shader --type vertex -f default.vs -o out/_${NAME}.h --bin2c ${NAME} $@
}

compile_fs()
{
	NAME=$1
	shift
	compile_shader --type fragment -f default.fs -o out/_${NAME}.h --bin2c ${NAME} $@
}

compile_vs "default_glsl_vertex_shader" --platform linux -p 120
compile_fs "default_glsl_fragment_shader" --platform linux -p 120

if [ "$1" == "windows" ]
then
	compile_vs "default_hlsl_d3d9_vertex_shader" --platform windows -p vs_3_0 -O 3
	compile_fs "default_hlsl_d3d9_fragment_shader" --platform windows -p ps_3_0 -O 3

	compile_vs "default_hlsl_d3d11_vertex_shader" --platform windows -p vs_5_0 -O 3
	compile_fs "default_hlsl_d3d11_fragment_shader" --platform windows -p ps_5_0 -O 3
fi

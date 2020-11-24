$input a_position, a_color0, a_texcoord0, a_texcoord1
$output v_color0, v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>

void main()
{
    // mat4 projViewWorld = mul(mul(u_proj, u_view), u_model[0]);
    mat4 projView = mul(u_proj, u_view);
	gl_Position = mul(projView, vec4(a_position, 1.0));

	v_color0 = a_color0;
	v_texcoord0 = a_texcoord0;
	v_texcoord1 = a_texcoord1;
}

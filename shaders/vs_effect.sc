$input a_position, a_color0, a_texcoord0, a_texcoord1
$output v_color0, v_texcoord0

#include <kaa.sh>

void main()
{
	gl_Position = vec4(a_position, 1.0);
	v_color0 = a_color0;
	v_texcoord0 = a_texcoord0;
}

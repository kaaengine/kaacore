$input v_color0, v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);

void main()
{
	vec2 tmp = abs(v_texcoord1);
	tmp = tmp * tmp;
	gl_FragColor = texture2D(s_texture, v_texcoord0).rgba * v_color0;
	if (tmp.x + tmp.y > 0.26) {
		discard;
	} else if (tmp.x + tmp.y > 0.24) {
		float alpha_mod = 1. - smoothstep(0.24, 0.26, tmp.x + tmp.y);
		gl_FragColor.a = gl_FragColor.a * alpha_mod;
	}
}

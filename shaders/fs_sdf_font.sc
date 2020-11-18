$input v_color0, v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);

void main()
{
    float outline_cut = 0.7;
    vec4 bg = vec4(0., 0., 0., 0.);
    vec4 fg = vec4(1., 1., 1., 1.);
    float d = texture2D(s_texture, v_texcoord0).r;
    float s = d - outline_cut;

    float v =  s / fwidth(s);
    float a = clamp(v + outline_cut, 0.0, 1.0);
    gl_FragColor = mix(bg, fg, a);
}

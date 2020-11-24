$input v_color0, v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);


// value based on font_sdf_edge_value from kaacore/fonts.h
const float edge_value = 180 / 255.;

vec4 sample_sub16(vec2 coord)
{
    vec2 offset_x = abs(dFdx(coord.xy) / 4.);
    vec2 offset_y = abs(dFdy(coord.xy) / 4.);
    float matched_samples = 0.0;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            vec2 coord_neighbor = coord + (offset_x * (-1.5 + float(i))
                                           + offset_y * (-1.5 + float(j)));
            float d = texture2D(s_texture, coord_neighbor).r;
            matched_samples += step(edge_value, d);
        }
    }

    return vec4(1., 1., 1., matched_samples / 16.);
}

void main()
{
    gl_FragColor = sample_sub16(v_texcoord0) * v_color0;
}

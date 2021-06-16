#ifndef KAA_H_HEADER_GUARD
#define KAA_H_HEADER_GUARD

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);
uniform vec4 u_vec4_slot1;
#define u_dt u_vec4_slot1.x
#define u_time u_vec4_slot1.y

#endif // KAA_H_HEADER_GUARD

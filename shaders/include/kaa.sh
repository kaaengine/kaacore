#ifndef KAA_H_HEADER_GUARD
#define KAA_H_HEADER_GUARD

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);
uniform vec4 u_view_rect;
uniform mat4 u_view_matrix;
uniform mat4 u_proj_matrix;
uniform mat4 u_view_proj_matrix;
uniform mat4 u_inv_view_matrix;
uniform mat4 u_inv_proj_matrix;
uniform mat4 u_inv_view_proj_matrix;
uniform vec4 u_vec4_slot1;
#define u_dt u_vec4_slot1.x
#define u_time u_vec4_slot1.y

#endif // KAA_H_HEADER_GUARD

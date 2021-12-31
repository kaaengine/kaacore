#ifndef KAA_H_HEADER_GUARD
#define KAA_H_HEADER_GUARD

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);
uniform vec4 u_viewportRect;
uniform mat4 u_viewMat;
uniform mat4 u_projMat;
uniform mat4 u_viewProjMat;
uniform mat4 u_invViewMat;
uniform mat4 u_invProjMat;
uniform mat4 u_invViewProjMat;
uniform vec4 u_vec4Slot1;
#define u_dt u_vec4Slot1.x
#define u_time u_vec4Slot1.y

#endif // KAA_H_HEADER_GUARD

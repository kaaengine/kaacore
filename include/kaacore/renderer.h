#pragma once

#include <vector>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "kaacore/files.h"


typedef uint16_t VertexIndex;

struct StandardVertexData {
    glm::fvec3 xyz;
    glm::fvec2 uv;
    glm::fvec2 mn;
    glm::fvec4 rgba;


    StandardVertexData(float x = 0., float y = 0., float z = 0.,
                       float u = 0., float v = 0.,
                       float m = 0., float n = 0.,
                       float r = 1., float g = 1., float b = 1., float a = 1.)
    : xyz(x, y, z), uv(u, v), mn(m, n), rgba(r, g, b, a) {};
};


struct Renderer {
    uint32_t reset_flags;
    bgfx::VertexDecl vertex_decl;
    bgfx::UniformHandle texture_uniform;
    bgfx::ProgramHandle default_program;
    bgfx::TextureHandle default_texture;

    Renderer();
    ~Renderer();

    void begin_frame();
    void end_frame();

    void render_vertices(std::vector<StandardVertexData> vertices,
                         std::vector<VertexIndex> indices,
                         bgfx::TextureHandle texture);
};

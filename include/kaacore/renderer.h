#pragma once

#include <vector>

#include <bgfx/bgfx.h>

#include "kaacore/files.h"


struct StandardVertexData {
    float x, y, z;
    float u, v;
    float m, n;
    float r, g, b, a;
};


struct Renderer {
    uint32_t reset_flags;
    bgfx::VertexDecl vertex_decl;
    bgfx::UniformHandle texture_uniform;
    bgfx::ProgramHandle default_program;
    bgfx::TextureHandle default_texture;

    Renderer();
    ~Renderer();

    void render_vertices(std::vector<StandardVertexData> vertices,
                         std::vector<uint16_t> indices,
                         bgfx::TextureHandle texture);
};

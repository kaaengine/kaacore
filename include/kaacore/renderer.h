#pragma once

#include <vector>
#include <memory>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "kaacore/files.h"
#include "kaacore/texture_loader.h"


namespace kaacore {

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

    static inline StandardVertexData XY_UV(float x, float y, float u, float v)
    {
        return StandardVertexData(x, y, 0., u, v);
    }

    static inline StandardVertexData XY_UV_MN(float x, float y, float u, float v, float m, float n)
    {
        return StandardVertexData(x, y, 0., u, v, m, n);
    }
};


struct Renderer {
    uint32_t clear_flags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH;
    uint32_t reset_flags = BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X2;

    bgfx::VertexDecl vertex_decl;

    std::unique_ptr<Image> default_image;

    bgfx::UniformHandle texture_uniform;
    bgfx::ProgramHandle default_program;
    // TODO replace with default_image
    bgfx::TextureHandle default_texture;

    Renderer(const glm::ivec2& window_size);
    ~Renderer();

    void begin_frame();
    void end_frame();

    inline void reset(int32_t width, int32_t height)
    {
        bgfx::reset(width, height, this->reset_flags);
        // TODO: add support for multiple views
        bgfx::setViewRect(0, 0, 0, width, height);
    }

    void render_vertices(const std::vector<StandardVertexData>& vertices,
                         const std::vector<VertexIndex>& indices,
                         const bgfx::TextureHandle texture) const;
};

} // namespace kaacore

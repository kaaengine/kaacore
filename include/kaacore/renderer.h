#pragma once

#include <memory>
#include <vector>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "kaacore/files.h"
#include "kaacore/images.h"
#include "kaacore/log.h"
#include "kaacore/resources.h"
#include "kaacore/shaders.h"
#include "kaacore/utils.h"
#include "kaacore/views.h"

namespace kaacore {

typedef uint16_t VertexIndex;

struct StandardVertexData {
    glm::fvec3 xyz;
    glm::fvec2 uv;
    glm::fvec2 mn;
    glm::fvec4 rgba;

    StandardVertexData(
        float x = 0., float y = 0., float z = 0., float u = 0., float v = 0.,
        float m = 0., float n = 0., float r = 1., float g = 1., float b = 1.,
        float a = 1.)
        : xyz(x, y, z), uv(u, v), mn(m, n), rgba(r, g, b, a){};

    static inline StandardVertexData XY_UV(float x, float y, float u, float v)
    {
        return StandardVertexData(x, y, 0., u, v);
    }

    static inline StandardVertexData XY_UV_MN(
        float x, float y, float u, float v, float m, float n)
    {
        return StandardVertexData(x, y, 0., u, v, m, n);
    }

    inline bool operator==(const StandardVertexData& other) const
    {
        return (
            this->xyz == other.xyz and this->uv == other.uv and
            this->mn == other.mn and this->rgba == other.rgba);
    }
};

struct Renderer {
    bgfx::VertexLayout vertex_layout;

    std::unique_ptr<Image> default_image;

    bgfx::UniformHandle texture_uniform;
    ResourceReference<Program> default_program;
    ResourceReference<Program> sdf_font_program;
    // TODO replace with default_image
    bgfx::TextureHandle default_texture;

    glm::uvec2 view_size;
    glm::uvec2 border_size;
    uint32_t border_color = 0x000000ff;

    Renderer(bgfx::Init bgfx_init_data, const glm::uvec2& window_size);
    ~Renderer();

    bgfx::TextureHandle make_texture(
        std::shared_ptr<bimg::ImageContainer> image_container,
        const uint64_t flags) const;
    void destroy_texture(const bgfx::TextureHandle& handle) const;
    void begin_frame();
    void end_frame();
    void reset();
    void process_view(View& view) const;
    void render_vertices(
        const uint16_t view_index,
        const std::vector<StandardVertexData>& vertices,
        const std::vector<VertexIndex>& indices,
        const bgfx::TextureHandle texture,
        const ResourceReference<Program>& program) const;

  private:
    uint32_t _reset_flags = BGFX_RESET_VSYNC;
};

} // namespace kaacore

namespace std {
using kaacore::hash_combined;
using kaacore::StandardVertexData;

template<>
struct hash<StandardVertexData> {
    size_t operator()(const StandardVertexData& svd) const
    {
        return hash_combined(svd.xyz, svd.uv, svd.mn, svd.rgba);
    }
};
}

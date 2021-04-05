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
#include "kaacore/materials.h"
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

enum class RendererType {
    noop = bgfx::RendererType::Noop,
    dx9 = bgfx::RendererType::Direct3D9,
    dx11 = bgfx::RendererType::Direct3D11,
    dx12 = bgfx::RendererType::Direct3D12,
    metal = bgfx::RendererType::Metal,
    opengl = bgfx::RendererType::OpenGL,
    vulkan = bgfx::RendererType::Vulkan,
    unsupported
};

class Renderer {
  public:
    bgfx::VertexLayout vertex_layout;
    std::unique_ptr<Image> default_image;
    bgfx::UniformHandle texture_uniform;
    ResourceReference<Material> default_material;
    ResourceReference<Material> sdf_font_material;
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
    RendererType type() const;
    ShaderModel shader_model() const;

    void destroy_texture(const bgfx::TextureHandle& handle) const;
    void begin_frame();
    void end_frame();
    void push_statistics() const;
    void reset();
    void process_view(View& view) const;
    void render_vertices(
        const uint16_t view_index,
        const std::vector<StandardVertexData>& vertices,
        const std::vector<VertexIndex>& indices,
        const bgfx::TextureHandle texture,
        const ResourceReference<Material>& material) const;

  private:
    bool _vertical_sync = true;

    uint32_t _calculate_reset_flags() const;
    bgfx::RendererType::Enum _choose_renderer(const std::string& name) const;
    void _bind_material(
        const ResourceReference<Material>& material,
        bgfx::TextureHandle texture) const;

    friend class Engine;
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

#pragma once

#include <memory>
#include <vector>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "kaacore/draw_queue.h"
#include "kaacore/draw_unit.h"
#include "kaacore/files.h"
#include "kaacore/images.h"
#include "kaacore/log.h"
#include "kaacore/materials.h"
#include "kaacore/resources.h"
#include "kaacore/shaders.h"
#include "kaacore/utils.h"
#include "kaacore/views.h"

namespace kaacore {

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

class DefaultShadingContext : public ShadingContext {
  public:
    DefaultShadingContext() = default;
    DefaultShadingContext(const UniformSpecificationMap& uniforms);
    DefaultShadingContext& operator=(DefaultShadingContext&& other);
    void destroy();
    void set_uniform_texture(
        const std::string& name, const Image* texture, const uint8_t stage,
        const uint32_t flags = std::numeric_limits<uint32_t>::max());
};

class Renderer {
  public:
    DefaultShadingContext shading_context;
    bgfx::VertexLayout vertex_layout;
    std::unique_ptr<Image> default_image;
    ResourceReference<Material> default_material;
    ResourceReference<Material> sdf_font_material;

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
    void render_draw_unit(const DrawBucketKey& key, const DrawUnit& draw_unit);
    void render_draw_bucket(
        const DrawBucketKey& key, const DrawBucket& draw_bucket);
    void render_draw_queue(const DrawQueue& draw_queue);
    void set_global_uniforms(const float last_dt, const float scene_time);
    static std::unordered_set<std::string>& reserved_uniform_names();

  private:
    bool _vertical_sync = true;

    uint32_t _calculate_reset_flags() const;
    void _submit_draw_bucket_state(const DrawBucketKey& key);
    bgfx::RendererType::Enum _choose_bgfx_renderer(
        const std::string& name) const;

    friend class Engine;
};

} // namespace kaacore

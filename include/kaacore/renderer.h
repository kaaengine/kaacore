#pragma once

#include <memory>
#include <vector>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "kaacore/draw_queue.h"
#include "kaacore/engine.h"
#include "kaacore/materials.h"
#include "kaacore/render_passes.h"
#include "kaacore/resources.h"
#include "kaacore/shaders.h"
#include "kaacore/textures.h"
#include "kaacore/utils.h"
#include "kaacore/viewports.h"

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

class Renderer;

class DefaultShadingContext : public ShadingContext {
  public:
    DefaultShadingContext() = default;
    DefaultShadingContext(const UniformSpecificationMap& uniforms);
    DefaultShadingContext& operator=(DefaultShadingContext&& other);
    void destroy();

    friend class Renderer;
};

struct FrameContext {
    Duration last_dt;
    Duration total_time;
    glm::uvec2 virtual_resolution;
    ViewportStateArray viewport_states;
    RenderPassStateArray render_pass_states;
};

struct RenderState {
    Texture* texture;
    Material* material;
    uint64_t state_flags;
    uint32_t stencil_flags;
};

struct DrawCall {
    RenderState state;
    uint32_t sorting_hint = 0;
    bgfx::TransientVertexBuffer vertices;
    bgfx::TransientIndexBuffer indices;

    static DrawCall allocate(
        const RenderState& state, const uint32_t sorting_hint,
        const size_t vertices_count, const size_t indices_count);

    static DrawCall create(
        const RenderState& state, const uint32_t sorting_hint,
        const std::vector<StandardVertexData>& vertices,
        const std::vector<VertexIndex>& indices);

    void bind_buffers() const;
};

struct DrawCommand {
    uint16_t pass;
    uint16_t viewport;
    DrawCall call;
};

struct RenderBatch {
    RenderState state;
    uint32_t sorting_hint;
    GeometryStream geometry_stream;

    template<typename Func>
    void each_draw_call(Func&& func) const
    {
        auto range = this->geometry_stream.find_range();
        while (not range.empty()) {
            auto call = DrawCall::allocate(
                this->state, this->sorting_hint, range.vertices_count,
                range.indices_count);
            this->geometry_stream.copy_range(
                range, call.vertices, call.indices);
            func(call);
            range = this->geometry_stream.find_range(range.end);
        }
    }

    static RenderBatch from_bucket(
        const DrawBucketKey& key, const DrawBucket& bucket);
};

struct RendererCapabilities {
    struct GpuInfo {
        uint16_t vendor_id;
        uint16_t device_id;
    };

    bool homogeneous_depth;
    bool origin_bottom_left;
    uint32_t max_draw_calls;
    uint32_t max_texture_size;
    uint32_t max_texture_layers;
    uint32_t max_render_passes;
    uint32_t max_render_targets;
    uint32_t max_programs;
    uint32_t max_shaders;
    uint32_t max_textures;
    uint32_t max_samplers;
    uint32_t max_uniforms;
    std::vector<GpuInfo> gpus;
};

enum class VirtualResolutionMode;

class Renderer {
  public:
    DefaultShadingContext shading_context;
    std::unique_ptr<Texture> default_texture;
    ResourceReference<Material> default_material;
    ResourceReference<Material> sdf_font_material;

    glm::uvec2 view_size;
    glm::uvec2 border_size;
    uint32_t border_color = 0x000000ff;

    Renderer(
        bgfx::Init bgfx_init_data, const glm::uvec2 window_size,
        glm::uvec2 virtual_resolution, VirtualResolutionMode mode);
    ~Renderer();

    bgfx::TextureHandle make_texture(
        std::shared_ptr<bimg::ImageContainer> image_container,
        const uint64_t flags) const;
    void destroy_texture(const bgfx::TextureHandle& handle) const;
    RendererType type() const;
    ShaderModel shader_model() const;
    const RendererCapabilities capabilities() const;
    void set_frame_context(
        const Duration last_dt, const Duration total_time,
        const RenderPassStateArray& render_pass_states,
        const ViewportStateArray& viewport_states);
    void begin_frame();
    void end_frame();
    void push_statistics() const;
    void reset(
        const glm::uvec2 windows_size, glm::uvec2 virtual_resolution,
        VirtualResolutionMode mode);
    void set_global_uniforms();
    void set_render_state(
        const RenderState& render_state, const ViewportState& viewport_state,
        const RenderPassState& pass_state);
    void render_batch(
        const RenderBatch& batch, const RenderPassIndexSet render_passes,
        const ViewportIndexSet viewports);
    void render_effect(const Effect& effect, const uint16_t pass_index);
    void render_draw_command(const DrawCommand& command);
    void render_draw_call(
        const DrawCall& call, const RenderPassState& pass_state,
        const ViewportState& viewport_state);
    static std::unordered_set<std::string>& reserved_uniform_names();

  private:
    bool _vertical_sync = true;
    FrameContext _frame_context;

    uint32_t _calculate_reset_flags() const;
    bgfx::ProgramHandle _get_program_handle(const Material* material);
    bgfx::RendererType::Enum _choose_bgfx_renderer(
        const std::string& name) const;

    friend class Engine;
};

} // namespace kaacore

#include <cstring>
#include <iterator>
#include <tuple>
#include <unordered_set>

#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>

#include "kaacore/exceptions.h"
#include "kaacore/files.h"
#include "kaacore/log.h"
#include "kaacore/platform.h"
#include "kaacore/renderer.h"
#include "kaacore/scenes.h"
#include "kaacore/statistics.h"
#include "kaacore/textures.h"

namespace kaacore {

bgfx::VertexLayout _vertex_layout;
constexpr uint16_t _internal_view_index = 0;
constexpr uint16_t _views_reserved_offset = 1;
constexpr uint8_t _internal_sampler_stage_index = 0;
const UniformSpecificationMap _default_uniforms = {
    {"s_texture", UniformSpecification(UniformType::sampler)},
    {"u_vec4_slot1", UniformSpecification(UniformType::vec4)},
    {"u_view_rect", UniformSpecification(UniformType::vec4)},
    {"u_view_matrix", UniformSpecification(UniformType::mat4)},
    {"u_proj_matrix", UniformSpecification(UniformType::mat4)},
    {"u_view_proj_matrix", UniformSpecification(UniformType::mat4)},
    {"u_inv_view_matrix", UniformSpecification(UniformType::mat4)},
    {"u_inv_proj_matrix", UniformSpecification(UniformType::mat4)},
    {"u_inv_view_proj_matrix", UniformSpecification(UniformType::mat4)},
};

// Since the memory that is used to load texture to bgfx should be available
// for at least two frames, we bump up its ref count by storing it in a set.
std::unordered_set<std::shared_ptr<bimg::ImageContainer>> _used_containers;

void
_release_used_container(void* _data, void* image_container)
{
    // Image containers are passed as raw pointers because of void*
    // but at this point we have to construct shared_ptr.
    // Use aliasing constructor to create non-owning ptr that we will
    // use as a key
    std::shared_ptr<bimg::ImageContainer> key(
        std::shared_ptr<Texture>(),
        static_cast<bimg::ImageContainer*>(image_container));
    _used_containers.erase(key);
}

std::pair<ResourceReference<Shader>, ResourceReference<Shader>>
load_embedded_shaders(
    const std::string vertex_shader_name,
    const std::string fragment_shader_name)
{
    KAACORE_LOG_DEBUG(
        "Loading embedded shaders: {}, {}", vertex_shader_name,
        fragment_shader_name);
    KAACORE_LOG_TRACE("Detected platform : {}", get_platform_name());
    return {load_embedded_shader(vertex_shader_name, ShaderType::vertex),
            load_embedded_shader(fragment_shader_name, ShaderType::fragment)};
}

std::unique_ptr<ImageTexture>
load_default_texture()
{
    // 1x1 white texture
    static const std::vector<uint8_t> image_content{0xFF, 0xFF, 0xFF, 0xFF};
    auto image_container =
        load_raw_image(bimg::TextureFormat::Enum::RGBA8, 1, 1, image_content);
    auto texture =
        std::unique_ptr<ImageTexture>(new ImageTexture(image_container));
    bgfx::setName(texture->handle(), "DEFAULT TEXTURE");
    return texture;
}

ResourceReference<Program>
load_embedded_program(
    const std::string& vertex_shader_name,
    const std::string& fragment_shader_name)
{
    auto [vs, fs] =
        load_embedded_shaders(vertex_shader_name, fragment_shader_name);
    return Program::create(vs, fs);
}

DefaultShadingContext::DefaultShadingContext(
    const UniformSpecificationMap& uniforms)
    : ShadingContext(uniforms)
{}

DefaultShadingContext&
DefaultShadingContext::operator=(DefaultShadingContext&& other)
{
    if (this == &other) {
        return *this;
    }
    this->_uniforms = std::move(other._uniforms);
    return *this;
}

void
DefaultShadingContext::destroy()
{
    this->_uninitialize();
    this->_uniforms.clear();
}

RenderState
RenderState::from_bucket_key(const DrawBucketKey& key)
{
    return {key.texture, key.material, key.state_flags, key.stencil_flags};
}

DrawCall
DrawCall::allocate(
    const RenderState& state, const uint32_t sorting_hint,
    const size_t vertices_count, const size_t indices_count)
{
    // TODO exception?
    KAACORE_LOG_TRACE(
        "Available transient vertex/index buffer size: {}/{}",
        bgfx::getAvailTransientVertexBuffer(0xFFFFFFFF, _vertex_layout),
        bgfx::getAvailTransientIndexBuffer(0xFFFFFFFF));

    bgfx::TransientVertexBuffer vertices_buffer;
    bgfx::TransientIndexBuffer indices_buffer;
    bgfx::allocTransientVertexBuffer(
        &vertices_buffer, vertices_count, _vertex_layout);
    bgfx::allocTransientIndexBuffer(&indices_buffer, indices_count);
    return DrawCall{state, sorting_hint, vertices_buffer, indices_buffer};
}

DrawCall
DrawCall::create(
    const RenderState& state, const uint32_t sorting_hint,
    const std::vector<StandardVertexData>& vertices,
    const std::vector<VertexIndex>& indices)
{
    auto call = DrawCall::allocate(
        state, sorting_hint, vertices.size(), indices.size());
    auto vertices_data_size = sizeof(StandardVertexData) * vertices.size();
    auto indices_data_size = sizeof(VertexIndex) * indices.size();
    std::memcpy(call.vertices.data, vertices.data(), vertices_data_size);
    std::memcpy(call.indices.data, indices.data(), indices_data_size);
    return call;
}

void
DrawCall::bind_buffers() const
{
    bgfx::setVertexBuffer(0, &this->vertices);
    bgfx::setIndexBuffer(&this->indices);
}

RenderBatch
RenderBatch::from_bucket(const DrawBucketKey& key, const DrawBucket& bucket)
{
    uint32_t sorting_hint =
        (std::abs(std::numeric_limits<int16_t>::min()) + key.z_index);
    sorting_hint <<= 16;
    sorting_hint |= key.root_distance;
    auto state = RenderState::from_bucket_key(key);
    return {state, sorting_hint, bucket.geometry_stream()};
}

Renderer::Renderer(
    bgfx::Init bgfx_init_data, const glm::uvec2 window_size,
    glm::uvec2 virtual_resolution, VirtualResolutionMode mode)
{
    KAACORE_LOG_INFO("Initializing bgfx.");
    bgfx_init_data.resolution.width = window_size.x;
    bgfx_init_data.resolution.height = window_size.y;

    if (auto renderer_name = SDL_getenv("KAACORE_RENDERER")) {
        bgfx_init_data.type = this->_choose_bgfx_renderer(renderer_name);
    }

    bgfx::init(bgfx_init_data);
    KAACORE_LOG_INFO("Initializing bgfx completed.");
    KAACORE_LOG_INFO("Initializing renderer.");
    _vertex_layout = StandardVertexData::init();
    this->reset(window_size, virtual_resolution, mode);
    this->default_texture = load_default_texture();
    KAACORE_LOG_INFO("Loading embedded default shader.");
    auto default_program = load_embedded_program("vs_default", "fs_default");
    KAACORE_LOG_INFO("Loading embedded sdf_font shader.");
    auto sdf_font_program = load_embedded_program("vs_default", "fs_sdf_font");
    this->default_material = Material::create(default_program);
    this->sdf_font_material = Material::create(sdf_font_program);
    this->shading_context = std::move(DefaultShadingContext(_default_uniforms));
}

Renderer::~Renderer()
{
    KAACORE_LOG_INFO("Destroying renderer");
    this->default_texture.reset();
    this->shading_context.destroy();

    // since default shaders are embeded and not present
    // in registry, free them manually
    if (this->default_material) {
        this->default_material.get()
            ->program.get()
            ->vertex_shader->_uninitialize();
        this->default_material.get()
            ->program.get()
            ->fragment_shader->_uninitialize();
        this->default_material.get()->program.get()->_uninitialize();
    }

    if (this->sdf_font_material) {
        this->sdf_font_material.get()
            ->program.get()
            ->vertex_shader->_uninitialize();
        this->sdf_font_material.get()
            ->program.get()
            ->fragment_shader->_uninitialize();
        this->sdf_font_material.get()->program.get()->_uninitialize();
    }

    bgfx::shutdown();
}

bgfx::TextureHandle
Renderer::make_texture(
    std::shared_ptr<bimg::ImageContainer> image_container,
    const uint64_t flags) const
{
    KAACORE_ASSERT(
        bgfx::isTextureValid(
            0, false, image_container->m_numLayers,
            bgfx::TextureFormat::Enum(image_container->m_format), flags),
        "Invalid texture.");

    const bgfx::Memory* memory = bgfx::makeRef(
        image_container->m_data, image_container->m_size,
        _release_used_container, image_container.get());

    auto handle = bgfx::createTexture2D(
        uint16_t(image_container->m_width), uint16_t(image_container->m_height),
        1 < image_container->m_numMips, image_container->m_numLayers,
        bgfx::TextureFormat::Enum(image_container->m_format), flags, memory);

    KAACORE_ASSERT(bgfx::isValid(handle), "Failed to create texture.");
    _used_containers.insert(std::move(image_container));
    return handle;
}

RendererType
Renderer::type() const
{
    switch (bgfx::getRendererType()) {
        case bgfx::RendererType::Noop:
            return RendererType::noop;
        case bgfx::RendererType::Direct3D9:
            return RendererType::dx9;
        case bgfx::RendererType::Direct3D11:
            return RendererType::dx11;
        case bgfx::RendererType::Direct3D12:
            return RendererType::dx12;
        case bgfx::RendererType::Metal:
            return RendererType::metal;
        case bgfx::RendererType::OpenGL:
            return RendererType::opengl;
        case bgfx::RendererType::Vulkan:
            return RendererType::vulkan;
        default:
            return RendererType::unsupported;
    }
}

ShaderModel
Renderer::shader_model() const
{
    switch (this->type()) {
        case RendererType::dx9:
            return ShaderModel::hlsl_dx9;
        case RendererType::dx11:
        case RendererType::dx12:
            return ShaderModel::hlsl_dx11;
        case RendererType::metal:
            return ShaderModel::metal;
        case RendererType::opengl:
            return ShaderModel::glsl;
        case RendererType::vulkan:
            return ShaderModel::spirv;
        default:
            return ShaderModel::unknown;
    }
}

const RendererCapabilities
Renderer::capabilities() const
{
    auto caps = bgfx::getCaps();
    std::vector<RendererCapabilities::GpuInfo> gpus;
    for (int i = 0; i < caps->numGPUs; ++i) {
        auto gpu = caps->gpu[i];
        gpus.push_back({gpu.vendorId, gpu.deviceId});
    }

    return {caps->homogeneousDepth,
            caps->originBottomLeft,
            caps->limits.maxDrawCalls,
            caps->limits.maxTextureSize,
            caps->limits.maxTextureLayers,
            caps->limits.maxViews,
            caps->limits.maxFBAttachments,
            caps->limits.maxPrograms,
            caps->limits.maxShaders,
            caps->limits.maxTextures,
            caps->limits.maxShaders,
            caps->limits.maxUniforms,
            gpus};
}

void
Renderer::destroy_texture(const bgfx::TextureHandle& handle) const
{
    KAACORE_ASSERT_TERMINATE(
        bgfx::isValid(handle), "Invalid handle - texture can't be destroyed.");
    bgfx::destroy(handle);
}

void
Renderer::begin_frame()
{}

void
Renderer::end_frame()
{
    bgfx::frame();
}

void
Renderer::push_statistics() const
{
    auto& stats_manager = get_global_statistics_manager();
    auto* bgfx_stats = bgfx::getStats();

    stats_manager.push_value("bgfx.draw_calls:count", bgfx_stats->numDraw);
    stats_manager.push_value(
        "bgfx.textures:memory",
        float(bgfx_stats->textureMemoryUsed) / (1024. * 1024.));
    stats_manager.push_value(
        "bgfx.transient_vb:memory",
        float(bgfx_stats->transientVbUsed) / (1024. * 1024.));
    stats_manager.push_value(
        "bgfx.transient_ib:memory",
        float(bgfx_stats->transientIbUsed) / (1024. * 1024.));
    stats_manager.push_value(
        "bgfx.cpu_frame:time",
        float(bgfx_stats->cpuTimeFrame) / bgfx_stats->cpuTimerFreq);
    stats_manager.push_value(
        "bgfx.wait_submit:time",
        float(bgfx_stats->waitSubmit) / bgfx_stats->cpuTimerFreq);
    stats_manager.push_value(
        "bgfx.wait_render:time",
        float(bgfx_stats->waitRender) / bgfx_stats->cpuTimerFreq);
}

void
Renderer::reset(
    const glm::uvec2 window_size, glm::uvec2 virtual_resolution,
    VirtualResolutionMode mode)
{
    KAACORE_LOG_DEBUG("Calling Renderer::reset()");

    bgfx::reset(window_size.x, window_size.y, this->_calculate_reset_flags());
    glm::uvec2 view_size, border_size;
    if (mode == VirtualResolutionMode::adaptive_stretch) {
        double aspect_ratio =
            double(virtual_resolution.x) / double(virtual_resolution.y);
        double window_aspect_ratio =
            double(window_size.x) / double(window_size.y);

        if (aspect_ratio < window_aspect_ratio) {
            view_size = {window_size.y * aspect_ratio, window_size.y};
        } else if (aspect_ratio > window_aspect_ratio) {
            view_size = {window_size.x, window_size.x * (1. / aspect_ratio)};
        } else {
            view_size = window_size;
        }
        border_size = {(window_size.x - view_size.x) / 2,
                       (window_size.y - view_size.y) / 2};
    } else if (mode == VirtualResolutionMode::aggresive_stretch) {
        view_size = window_size;
        border_size = {0, 0};
    } else if (mode == VirtualResolutionMode::no_stretch) {
        view_size = virtual_resolution;
        border_size = {
            window_size.x > view_size.x ? (window_size.x - view_size.x) / 2 : 0,
            window_size.y > view_size.y ? (window_size.y - view_size.y) / 2
                                        : 0};
    } else {
        throw exception("Unrecognized virtual resolution");
    }
    this->view_size = view_size;
    this->border_size = border_size;

    // internal pass
    bgfx::setViewClear(
        _internal_view_index, BGFX_CLEAR_COLOR, this->border_color);
    bgfx::setViewRect(_internal_view_index, 0, 0, bgfx::BackbufferRatio::Equal);

    // user available passes
    for (auto pass_index = _internal_view_index + _views_reserved_offset;
         pass_index <= KAACORE_MAX_RENDER_PASSES; ++pass_index) {
        bgfx::setViewRect(
            pass_index, border_size.x, border_size.y, view_size.x, view_size.y);
    }
}

void
Renderer::set_global_uniforms(const float last_dt, const float scene_time)
{
    glm::vec4 u_vec4_slot1{last_dt, scene_time, 0, 0};
    this->shading_context.set_uniform_value<glm::fvec4>(
        "u_vec4_slot1", u_vec4_slot1);
    this->shading_context.bind("u_vec4_slot1");
}

void
Renderer::set_pass_state(const RenderPassState& state)
{
    if (not state.requires_clean) {
        return;
    }

    auto pass_index = state.index + _views_reserved_offset;
    if (state.active_attachments_number) {
        for (auto i = 0; i < state.clear_colors.size(); ++i) {
            auto color_value = reinterpret_cast<const float*>(
                glm::value_ptr(state.clear_colors[i]));
            bgfx::setPaletteColor(i, color_value);
        }
        bgfx::setViewClear(
            pass_index, state.clear_flags, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7);
    } else {
        uint32_t r, g, b, a;
        auto clear_color = state.clear_colors[0];
        a = static_cast<uint32_t>(clear_color.a * 255.0 + 0.5);
        b = static_cast<uint32_t>(clear_color.b * 255.0 + 0.5) << 8;
        g = static_cast<uint32_t>(clear_color.g * 255.0 + 0.5) << 16;
        r = static_cast<uint32_t>(clear_color.r * 255.0 + 0.5) << 24;
        auto clear_color_hex = a + b + g + r;
        bgfx::setViewClear(pass_index, state.clear_flags, clear_color_hex);
    }
}

void
Renderer::set_viewport_state(const ViewportState& state)
{
    auto view_projection_matrix = state.projection_matrix * state.view_matrix;
    this->shading_context.set_uniform_value<glm::fvec4>(
        "u_view_rect", state.view_rect);
    this->shading_context.set_uniform_value<glm::fmat4>(
        "u_view_matrix", state.view_matrix);
    this->shading_context.set_uniform_value<glm::fmat4>(
        "u_proj_matrix", state.projection_matrix);
    this->shading_context.set_uniform_value<glm::fmat4>(
        "u_view_proj_matrix", view_projection_matrix);
    this->shading_context.set_uniform_value<glm::fmat4>(
        "u_inv_view_matrix", glm::inverse(state.view_matrix));
    this->shading_context.set_uniform_value<glm::fmat4>(
        "u_inv_proj_matrix", glm::inverse(state.projection_matrix));
    this->shading_context.set_uniform_value<glm::fmat4>(
        "u_inv_view_proj_matrix", glm::inverse(view_projection_matrix));

    this->shading_context.bind("u_view_rect");
    this->shading_context.bind("u_view_matrix");
    this->shading_context.bind("u_proj_matrix");
    this->shading_context.bind("u_view_proj_matrix");
    this->shading_context.bind("u_inv_view_matrix");
    this->shading_context.bind("u_inv_proj_matrix");
    this->shading_context.bind("u_inv_view_proj_matrix");

    bgfx::setScissor(
        static_cast<uint16_t>(state.view_rect.x),
        static_cast<uint16_t>(state.view_rect.y),
        static_cast<uint16_t>(state.view_rect.z),
        static_cast<uint16_t>(state.view_rect.w));
}

bgfx::ProgramHandle
Renderer::set_render_state(const RenderState& state)
{
    auto texture = state.texture ? state.texture : this->default_texture.get();
    auto material =
        state.material ? state.material : this->default_material.get_valid();
    this->shading_context._set_uniform_texture(
        "s_texture", texture, _internal_sampler_stage_index);
    this->shading_context.bind("s_texture");
    material->bind();
    bgfx::setState(
        BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
        BGFX_STATE_MSAA | BGFX_STATE_BLEND_ALPHA | state.state_flags);
    bgfx::setStencil(state.stencil_flags);
    return material->program->_handle;
}

void
Renderer::render_scene(Scene* scene)
{
    this->set_global_uniforms(
        scene->_last_dt.count(), scene->_total_time.count());

    auto viewport_states = scene->viewports.take_snapshot();
    auto render_pass_states = scene->render_passes.take_snapshot();

    // drawing of nodes tree
    for (const auto& [key, bucket] : scene->draw_queue) {
        auto batch = RenderBatch::from_bucket(key, bucket);
        if (batch.geometry_stream.empty()) {
            continue;
        }

        this->render_batch(
            batch, key.render_passes, key.viewports, render_pass_states,
            viewport_states);
    }

    // drawing of custom draw calls
    for (auto& draw_command : scene->_draw_commands) {
        this->render_draw_call(
            draw_command.call, render_pass_states[draw_command.pass],
            viewport_states[draw_command.viewport]);
    }

    // effects
    bgfx::touch(_internal_view_index);
    for (auto& render_pass : scene->render_passes) {
        bgfx::touch(render_pass._index + _views_reserved_offset);
        if (not render_pass.effect) {
            continue;
        }

        ViewportState viewport_state{{this->border_size, this->view_size},
                                     glm::fmat4(1.f),
                                     glm::fmat4(1.f)};
        this->render_draw_call(
            render_pass.effect->draw_call(),
            render_pass_states[render_pass._index], viewport_state);
    }

    scene->render_passes._reset();
}

void
Renderer::render_batch(
    const RenderBatch& batch, const RenderPassIndexSet target_render_passes,
    const ViewportIndexSet target_viewports,
    const RenderPassStateArray& render_pass_states,
    const ViewportStateArray& viewport_states)
{
    batch.each_draw_call([=, &render_pass_states,
                          &viewport_states](const DrawCall& call) {
        // iterate over viewports first to minimalize state changes
        target_viewports.each_active_index([=, &call, &render_pass_states,
                                            &viewport_states](
                                               uint16_t viewport_index) {
            call.bind_buffers();
            auto program_handle = this->set_render_state(call.state);
            this->set_viewport_state(viewport_states[viewport_index]);
            target_render_passes.each_active_index(
                [this, program_handle, viewport_index,
                 sorting_hint = call.sorting_hint,
                 &render_pass_states](uint16_t render_pass_index) {
                    this->set_pass_state(render_pass_states[render_pass_index]);
                    auto depth = sorting_hint | (viewport_index << 24);
                    bgfx::submit(
                        render_pass_index + _views_reserved_offset,
                        program_handle, depth, BGFX_DISCARD_NONE);
                });
            this->discard_render_state();
        });
    });
}

void
Renderer::render_draw_call(
    const DrawCall& call, const RenderPassState& pass_state,
    const ViewportState& viewport_state)
{
    call.bind_buffers();
    this->set_pass_state(pass_state);
    this->set_viewport_state(viewport_state);
    auto program_handle = this->set_render_state(call.state);
    bgfx::submit(
        pass_state.index + _views_reserved_offset, program_handle,
        call.sorting_hint, BGFX_DISCARD_ALL);
}

std::unordered_set<std::string>&
Renderer::reserved_uniform_names()
{
    static std::unordered_set<std::string> reserved_names;
    if (not reserved_names.size()) {
        for (auto& kv_pair : _default_uniforms) {
            reserved_names.insert(kv_pair.first);
        }
    }
    return reserved_names;
}

inline void
Renderer::discard_render_state()
{
    bgfx::discard(BGFX_DISCARD_ALL);
}

uint32_t
Renderer::_calculate_reset_flags() const
{
    return this->_vertical_sync ? BGFX_RESET_VSYNC : 0;
}

bgfx::RendererType::Enum
Renderer::_choose_bgfx_renderer(const std::string& renderer_name) const
{
    if (renderer_name == "noop") {
        return bgfx::RendererType::Noop;
    } else if (renderer_name == "dx9") {
        return bgfx::RendererType::Direct3D9;
    } else if (renderer_name == "dx11") {
        return bgfx::RendererType::Direct3D11;
    } else if (renderer_name == "dx12") {
        return bgfx::RendererType::Direct3D12;
    } else if (renderer_name == "metal") {
        return bgfx::RendererType::Metal;
    } else if (renderer_name == "opengl") {
        return bgfx::RendererType::OpenGL;
    } else if (renderer_name == "vulkan") {
        return bgfx::RendererType::Vulkan;
    } else {
        throw exception(
            fmt::format("Unsupported renderer: {}.\n", renderer_name));
    }
}

} // namespace kaacore

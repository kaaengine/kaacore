#include <array>
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
    {"u_vec4Slot1", UniformSpecification(UniformType::vec4)},
    {"u_viewportRect", UniformSpecification(UniformType::vec4)},
    {"u_viewMat", UniformSpecification(UniformType::mat4)},
    {"u_projMat", UniformSpecification(UniformType::mat4)},
    {"u_viewProjMat", UniformSpecification(UniformType::mat4)},
    {"u_invViewMat", UniformSpecification(UniformType::mat4)},
    {"u_invProjMat", UniformSpecification(UniformType::mat4)},
    {"u_invViewProjMat", UniformSpecification(UniformType::mat4)},
};
constexpr std::array<const std::string_view, 12> _bgfx_reserved_uniforms = {
    "u_viewRect", "u_viewTexel", "u_view",          "u_invView",
    "u_proj",     "u_invProj",   "u_viewProj",      "u_invViewProj",
    "u_model",    "u_modelView", "u_modelViewProj", "u_alphaRef4",
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
    return {
        EmbeddedShader::load(ShaderType::fragment, fragment_shader_name),
        EmbeddedShader::load(ShaderType::vertex, vertex_shader_name),
    };
}

std::unique_ptr<MemoryTexture>
load_default_texture()
{
    // 1x1 white texture
    static const std::vector<uint8_t> image_content{0xFF, 0xFF, 0xFF, 0xFF};
    auto image_container =
        load_raw_image(bimg::TextureFormat::Enum::RGBA8, 1, 1, image_content);
    auto texture =
        std::unique_ptr<MemoryTexture>(new MemoryTexture(image_container));

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
    RenderState state{key.texture, key.material, key.state_flags,
                      key.stencil_flags};
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
    auto start_index = _internal_view_index + _views_reserved_offset;
    for (auto view_index = start_index;
         view_index < start_index + KAACORE_MAX_RENDER_PASSES; ++view_index) {
        bgfx::setViewMode(view_index, bgfx::ViewMode::DepthAscending);
    }
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

void
Renderer::destroy_texture(const bgfx::TextureHandle& handle) const
{
    KAACORE_ASSERT_TERMINATE(
        bgfx::isValid(handle), "Invalid handle - texture can't be destroyed.");
    bgfx::destroy(handle);
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
Renderer::set_frame_context(
    const Duration last_dt, const Duration total_time,
    const RenderPassStateArray& render_pass_states,
    const ViewportStateArray& viewport_states)
{
    this->_frame_context.last_dt = last_dt;
    this->_frame_context.total_time = total_time;
    this->_frame_context.viewport_states = viewport_states;
    this->_frame_context.render_pass_states = render_pass_states;
}

void
Renderer::begin_frame()
{
    this->set_global_uniforms();
    bgfx::touch(_internal_view_index);
    for (auto pass_index = 0; pass_index < KAACORE_MAX_RENDER_PASSES;
         ++pass_index) {
        bgfx::touch(pass_index + _views_reserved_offset);
        this->set_render_pass_state(
            this->_frame_context.render_pass_states[pass_index]);
    }
}

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
    this->_frame_context.virtual_resolution = virtual_resolution;

    bgfx::setViewClear(
        _internal_view_index, BGFX_CLEAR_COLOR, this->border_color);
    bgfx::setViewRect(_internal_view_index, 0, 0, bgfx::BackbufferRatio::Equal);
}

void
Renderer::set_global_uniforms()
{
    glm::vec4 u_vec4_slot1{this->_frame_context.last_dt.count(),
                           this->_frame_context.total_time.count(), 0, 0};
    this->shading_context.set_uniform_value<glm::fvec4>(
        "u_vec4Slot1", u_vec4_slot1);
    this->shading_context.bind("u_vec4Slot1");
}

void
Renderer::set_render_state(
    const RenderState& render_state, const RenderPassState& pass_state,
    const ViewportState& viewport_state)
{
    auto view_index = pass_state.index + _views_reserved_offset;
    bgfx::setState(
        BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
        BGFX_STATE_MSAA | BGFX_STATE_BLEND_ALPHA | render_state.state_flags);
    bgfx::setStencil(render_state.stencil_flags);
    // rect clipped to drawable area - used for scissor test
    auto view_rect = viewport_state.view_rect;
    // user defined rect - no cliping applied
    auto viewport_rect = viewport_state.viewport_rect;
    auto projection_matrix = viewport_state.projection_matrix;
    if (pass_state.has_custom_framebuffer()) {
        // render target is always size of a drawable area size
        // therefore project onto full available area
        float x = this->_frame_context.virtual_resolution.x;
        float y = this->_frame_context.virtual_resolution.y;
        projection_matrix = glm::ortho(-x / 2, x / 2, y / 2, -y / 2);
        if (bgfx::getCaps()->originBottomLeft) {
            // adjust for NDC origin being at the bottom left
            projection_matrix = glm::scale(projection_matrix, {1., -1., 1.});
        }
    } else {
        // view_rect and viewport_rect weren't adjusted for borders yet
        auto offset = glm::fvec4({this->border_size, 0, 0});
        view_rect += offset;
        viewport_rect += offset;
    }

    bgfx::setScissor(
        static_cast<uint16_t>(view_rect.x), static_cast<uint16_t>(view_rect.y),
        static_cast<uint16_t>(view_rect.z), static_cast<uint16_t>(view_rect.w));

    auto view_matrix = viewport_state.view_matrix;
    auto texture = render_state.texture ? render_state.texture
                                        : this->default_texture.get();
    auto view_projection_matrix = projection_matrix * view_matrix;
    this->shading_context._set_uniform_texture(
        "s_texture", texture, _internal_sampler_stage_index);
    this->shading_context.set_uniform_value<glm::fvec4>(
        "u_viewportRect", viewport_rect);
    this->shading_context.set_uniform_value<glm::fmat4>(
        "u_viewMat", viewport_state.view_matrix);
    this->shading_context.set_uniform_value<glm::fmat4>(
        "u_projMat", projection_matrix);
    this->shading_context.set_uniform_value<glm::fmat4>(
        "u_viewProjMat", view_projection_matrix);
    this->shading_context.set_uniform_value<glm::fmat4>(
        "u_invViewMat", glm::inverse(view_matrix));
    this->shading_context.set_uniform_value<glm::fmat4>(
        "u_invProjMat", glm::inverse(projection_matrix));
    this->shading_context.set_uniform_value<glm::fmat4>(
        "u_invViewProjMat", glm::inverse(view_projection_matrix));

    this->shading_context.bind("s_texture");
    this->shading_context.bind("u_viewportRect");
    this->shading_context.bind("u_viewMat");
    this->shading_context.bind("u_projMat");
    this->shading_context.bind("u_viewProjMat");
    this->shading_context.bind("u_invViewMat");
    this->shading_context.bind("u_invProjMat");
    this->shading_context.bind("u_invViewProjMat");

    auto material = render_state.material ? render_state.material
                                          : this->default_material.get_valid();
    material->bind();
}

void
Renderer::set_render_pass_state(const RenderPassState& state)
{
    auto view_index = state.index + _views_reserved_offset;
    bgfx::setViewFrameBuffer(view_index, state.frame_buffer);
    if (state.has_custom_framebuffer()) {
        // render target is always size of a drawable area
        bgfx::setViewRect(
            view_index, 0, 0, this->view_size.x, this->view_size.y);
    } else {
        // back buffer case, adjust for borders
        bgfx::setViewRect(
            view_index, this->border_size.x, this->border_size.y,
            this->view_size.x, this->view_size.y);
    }

    if (not state.requires_clear) {
        return;
    }

    if (state.has_custom_framebuffer()) {
        auto flags = state.clear_flags;
        for (auto i = 0; i < state.clear_colors.size(); ++i) {
            glm::fvec4 clear_color = state.clear_colors[i];
            bgfx::setPaletteColor(i, glm::value_ptr(clear_color));
        }
        bgfx::setViewClear(view_index, flags, 1.f, 0, 0, 1, 2, 3, 4, 5, 6, 7);
    } else {
        glm::fvec4 clear_color = state.clear_colors[0];
        if (clear_color.a) {
            bgfx::setPaletteColor(0, glm::value_ptr(clear_color));
            bgfx::setViewClear(view_index, state.clear_flags, 1.f, 0, 0);
        } else {
            bgfx::setViewClear(view_index, BGFX_CLEAR_NONE);
        }
    }
}

void
Renderer::render_batch(
    const RenderBatch& batch, const RenderPassIndexSet target_render_passes,
    const ViewportIndexSet target_viewports)
{
    batch.each_draw_call([this, target_viewports,
                          target_render_passes](const DrawCall& call) {
        target_render_passes.each_active_index([this, target_viewports,
                                                &call](uint16_t pass_index) {
            target_viewports.each_active_index(
                [this, pass_index, &call](uint16_t viewport_index) {
                    auto& ctx = this->_frame_context;
                    auto pass_state = ctx.render_pass_states[pass_index];
                    auto viewport_state = ctx.viewport_states[viewport_index];
                    this->render_draw_call(call, pass_state, viewport_state);
                });
        });
    });
}

void
Renderer::render_effect(const Effect& effect, const uint16_t pass_index)
{
    glm::dvec4 view_rect = {0, 0, this->view_size};
    uint16_t max_viewport_index = KAACORE_MAX_VIEWPORTS - 1;
    ViewportState viewport_state{max_viewport_index, view_rect, view_rect,
                                 glm::fmat4(1.f), glm::fmat4(1.f)};
    auto pass_state = this->_frame_context.render_pass_states[pass_index];
    this->render_draw_call(effect.draw_call(), pass_state, viewport_state);
}

void
Renderer::render_draw_command(const DrawCommand& command)
{
    uint16_t pass_index = command.pass, viewport_index = command.viewport;
    auto pass_state = this->_frame_context.render_pass_states[pass_index];
    auto viewport_state = this->_frame_context.viewport_states[viewport_index];
    this->render_draw_call(command.call, pass_state, viewport_state);
}

void
Renderer::render_draw_call(
    const DrawCall& call, const RenderPassState& pass_state,
    const ViewportState& viewport_state)
{
    call.bind_buffers();
    this->set_render_state(call.state, pass_state, viewport_state);
    uint32_t depth = call.sorting_hint | (viewport_state.index << 24);
    bgfx::submit(
        pass_state.index + _views_reserved_offset,
        this->_get_program_handle(call.state.material), depth,
        BGFX_DISCARD_ALL);
}

const std::unordered_set<std::string>&
Renderer::reserved_uniform_names()
{
    static std::unordered_set<std::string> reserved_names;
    if (not reserved_names.size()) {
        for (auto& kv_pair : _default_uniforms) {
            reserved_names.insert(kv_pair.first);
        }
        for (auto& name : _bgfx_reserved_uniforms) {
            reserved_names.insert(std::string(name));
        }
    }
    return reserved_names;
}

uint32_t
Renderer::_calculate_reset_flags() const
{
    return this->_vertical_sync ? BGFX_RESET_VSYNC : 0;
}

bgfx::ProgramHandle
Renderer::_get_program_handle(const Material* material)
{
    auto ptr = material ? material : this->default_material.get_valid();
    return ptr->program->_handle;
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

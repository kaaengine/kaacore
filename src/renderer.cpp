#include <cstring>
#include <iterator>
#include <tuple>
#include <unordered_set>

#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>

#include "kaacore/embedded_data.h"
#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/files.h"
#include "kaacore/log.h"
#include "kaacore/memory.h"
#include "kaacore/platform.h"
#include "kaacore/renderer.h"
#include "kaacore/statistics.h"
#include "kaacore/textures.h"

namespace kaacore {

constexpr uint16_t _internal_view_index = 0;
constexpr uint8_t _internal_sampler_stage_index = 0;
const UniformSpecificationMap _default_uniforms = {
    {"s_texture", UniformSpecification(UniformType::sampler)},
    {"u_vec4_slot1", UniformSpecification(UniformType::vec4)},
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

std::string
get_shader_model_tag(ShaderModel model)
{
    switch (model) {
        case ShaderModel::glsl:
            return "glsl";
        case ShaderModel::spirv:
            return "spirv";
        case ShaderModel::metal:
            return "metal";
        case ShaderModel::hlsl_dx9:
            return "dx9";
        case ShaderModel::hlsl_dx11:
            return "dx11";
        default:
            return "unknown";
    }
}

Memory
load_embedded_shader(const std::string& path)
{
    try {
        return get_embedded_file_content(embedded_shaders_filesystem, path);
    } catch (embedded_file_error& err) {
        KAACORE_LOG_ERROR(
            "Failed to load embedded binary shader: {} ({})", path, err.what());
        throw;
    }
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

    std::vector<ShaderModel> models;
    switch (get_platform()) {
        case PlatformType::linux:
            models = {ShaderModel::glsl, ShaderModel::spirv};
            break;
        case PlatformType::osx:
            models = {ShaderModel::metal, ShaderModel::glsl,
                      ShaderModel::spirv};
            break;
        case PlatformType::windows:
            models = {ShaderModel::hlsl_dx9, ShaderModel::hlsl_dx11,
                      ShaderModel::glsl, ShaderModel::spirv};
            break;
        default:
            KAACORE_LOG_ERROR(
                "Unsupported platform! Can't load embedded shaders.");
    }

    std::string path;
    ShaderModelMemoryMap vertex_memory_map;
    ShaderModelMemoryMap fragment_memory_map;
    for (auto model : models) {
        auto shader_model_tag = get_shader_model_tag(model);
        path = fmt::format("{}/{}.bin", shader_model_tag, vertex_shader_name);
        vertex_memory_map[model] = load_embedded_shader(path);
        path = fmt::format("{}/{}.bin", shader_model_tag, fragment_shader_name);
        fragment_memory_map[model] = load_embedded_shader(path);
    }
    return {Shader::create(ShaderType::vertex, vertex_memory_map),
            Shader::create(ShaderType::fragment, fragment_memory_map)};
}

std::unique_ptr<Texture>
load_default_texture()
{
    // 1x1 white texture
    static const std::vector<uint8_t> image_content{0xFF, 0xFF, 0xFF, 0xFF};
    auto image_container =
        load_raw_image(bimg::TextureFormat::Enum::RGBA8, 1, 1, image_content);
    auto texture = std::unique_ptr<Texture>(new Texture(image_container));
    bgfx::setName(texture->handle, "DEFAULT TEXTURE");
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
DefaultShadingContext::set_uniform_texture(
    const std::string& name, const Texture* texture, const uint8_t stage,
    const uint32_t flags)
{
    KAACORE_CHECK(
        this->_name_in_registry(name), "Unknown uniform name: {}.", name);
    std::get<Sampler>(this->_uniforms[name]).set(texture, stage, flags);
}

void
DefaultShadingContext::destroy()
{
    this->_uninitialize();
    this->_uniforms.clear();
}

Renderer::Renderer(bgfx::Init bgfx_init_data, const glm::uvec2& window_size)
{
    KAACORE_LOG_INFO("Initializing bgfx.");
    bgfx_init_data.callback = &this->_renderer_callbacks;
    bgfx_init_data.resolution.width = window_size.x;
    bgfx_init_data.resolution.height = window_size.y;

    if (auto renderer_name = SDL_getenv("KAACORE_RENDERER")) {
        bgfx_init_data.type = this->_choose_bgfx_renderer(renderer_name);
    }

    bgfx::init(bgfx_init_data);
    KAACORE_LOG_INFO("Initializing bgfx completed.");
    KAACORE_LOG_INFO("Initializing renderer.");
    this->vertex_layout.begin()
        .add(bgfx::Attrib::Enum::Position, 3, bgfx::AttribType::Enum::Float)
        .add(bgfx::Attrib::Enum::TexCoord0, 2, bgfx::AttribType::Enum::Float)
        .add(bgfx::Attrib::Enum::TexCoord1, 2, bgfx::AttribType::Enum::Float)
        .add(bgfx::Attrib::Enum::Color0, 4, bgfx::AttribType::Enum::Float)
        .end();

    this->reset();
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

void
Renderer::destroy_texture(const bgfx::TextureHandle& handle) const
{
    KAACORE_ASSERT_TERMINATE(
        bgfx::isValid(handle), "Invalid handle - texture can't be destroyed.");
    bgfx::destroy(handle);
}

void
Renderer::begin_frame()
{
    if (this->_needs_reset) {
        this->reset();
        this->_needs_reset = false;
    }
    for (int i = 0; i <= KAACORE_MAX_VIEWS; ++i) {
        bgfx::setViewMode(i, bgfx::ViewMode::DepthAscending);
    }
}

void
Renderer::end_frame()
{
    // TODO: optimize!
    for (int i = 0; i <= KAACORE_MAX_VIEWS; ++i) {
        bgfx::touch(i);
    }
    bgfx::frame();
}

void
Renderer::final_frame()
{
    this->reset(true);
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
Renderer::reset(bool no_flags)
{
    KAACORE_LOG_DEBUG("Calling Renderer::reset()");
    auto window_size = get_engine()->window->_peek_size();
    bgfx::reset(
        window_size.x, window_size.y,
        no_flags ? 0 : this->_calculate_reset_flags());

    glm::uvec2 view_size, border_size;
    auto virtual_resolution = get_engine()->virtual_resolution();
    auto virtual_resolution_mode = get_engine()->_virtual_resolution_mode;

    if (virtual_resolution_mode == VirtualResolutionMode::adaptive_stretch) {
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
    } else if (
        virtual_resolution_mode == VirtualResolutionMode::aggresive_stretch) {
        view_size = window_size;
        border_size = {0, 0};
    } else if (virtual_resolution_mode == VirtualResolutionMode::no_stretch) {
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

    bgfx::setViewRect(_internal_view_index, 0, 0, window_size.x, window_size.y);
    bgfx::setViewClear(
        _internal_view_index, BGFX_CLEAR_COLOR, this->border_color);
}

void
Renderer::process_view(View& view) const
{
    if (view._requires_clean) {
        uint32_t r, g, b, a;
        a = static_cast<uint32_t>(view._clear_color.a * 255.0 + 0.5);
        b = static_cast<uint32_t>(view._clear_color.b * 255.0 + 0.5) << 8;
        g = static_cast<uint32_t>(view._clear_color.g * 255.0 + 0.5) << 16;
        r = static_cast<uint32_t>(view._clear_color.r * 255.0 + 0.5) << 24;
        auto clear_color_hex = a + b + g + r;
        bgfx::setViewClear(view._index, view._clear_flags, clear_color_hex);
        view._requires_clean = false;
    }

    if (view.is_dirty()) {
        view._refresh();

        bgfx::setViewRect(
            view._index, static_cast<uint16_t>(view._view_rect.x),
            static_cast<uint16_t>(view._view_rect.y),
            static_cast<uint16_t>(view._view_rect.z),
            static_cast<uint16_t>(view._view_rect.w));

        bgfx::setViewTransform(
            view._index, glm::value_ptr(view.camera._calculated_view),
            glm::value_ptr(view._projection_matrix));
    }
}

void
Renderer::render_draw_unit(const DrawBucketKey& key, const DrawUnit& draw_unit)
{
    bgfx::TransientVertexBuffer vertices_buffer;
    bgfx::TransientIndexBuffer indices_buffer;

    bgfx::allocTransientVertexBuffer(
        &vertices_buffer, draw_unit.details.vertices.size(),
        this->vertex_layout);
    bgfx::allocTransientIndexBuffer(
        &indices_buffer, draw_unit.details.indices.size());

    std::memcpy(
        vertices_buffer.data, draw_unit.details.vertices.data(),
        sizeof(StandardVertexData) * draw_unit.details.vertices.size());
    std::memcpy(
        indices_buffer.data, draw_unit.details.indices.data(),
        sizeof(VertexIndex) * draw_unit.details.indices.size());

    bgfx::setVertexBuffer(0, &vertices_buffer);
    bgfx::setIndexBuffer(&indices_buffer);
    this->_submit_draw_bucket_state(key);
}

void
Renderer::render_draw_bucket(
    const DrawBucketKey& key, const DrawBucket& draw_bucket)
{
    DrawBucket::Range range = draw_bucket.find_range();
    while (not range.empty()) {
        bgfx::TransientVertexBuffer vertices_buffer;
        bgfx::TransientIndexBuffer indices_buffer;
        KAACORE_LOG_TRACE(
            "Available transient vertex/index buffer size: {}/{}",
            bgfx::getAvailTransientVertexBuffer(
                0xFFFFFFFF, this->vertex_layout),
            bgfx::getAvailTransientIndexBuffer(0xFFFFFFFF));
        bgfx::allocTransientVertexBuffer(
            &vertices_buffer, range.vertices_count, this->vertex_layout);
        bgfx::allocTransientIndexBuffer(&indices_buffer, range.indices_count);

        draw_bucket.copy_range_details_to_transient_buffers(
            range, vertices_buffer, indices_buffer);

        bgfx::setVertexBuffer(0, &vertices_buffer);
        bgfx::setIndexBuffer(&indices_buffer);
        this->_submit_draw_bucket_state(key);

        range = draw_bucket.find_range(range.end);
    }
}

void
Renderer::render_draw_queue(const DrawQueue& draw_queue)
{
    for (const auto& [key, draw_bucket] : draw_queue) {
        this->render_draw_bucket(key, draw_bucket);
    }
}

void
Renderer::set_global_uniforms(const float last_dt, const float scene_time)
{
    glm::vec4 u_vec4_slot1{last_dt, scene_time, 0, 0};
    this->shading_context.set_uniform_value<glm::vec4>(
        "u_vec4_slot1", u_vec4_slot1);
    this->shading_context.bind("u_vec4_slot1");
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

void
Renderer::_submit_draw_bucket_state(const DrawBucketKey& key)
{
    bgfx::ProgramHandle program_handle;
    bgfx::TextureHandle texture_handle;
    auto texture =
        key.texture_raw_ptr ? key.texture_raw_ptr : this->default_texture.get();
    this->shading_context.set_uniform_texture(
        "s_texture", texture, _internal_sampler_stage_index);
    this->shading_context.bind("s_texture");
    auto material = key.material_raw_ptr ? key.material_raw_ptr
                                         : this->default_material.get();
    material->bind();
    program_handle = material->program->_handle;

    bgfx::setState(
        BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
        BGFX_STATE_MSAA | BGFX_STATE_BLEND_ALPHA | key.state_flags);
    bgfx::setStencil(key.stencil_flags);

    uint32_t sorting_value =
        (std::abs(std::numeric_limits<int16_t>::min()) + key.z_index);
    sorting_value <<= 16;
    sorting_value |= key.root_distance;
    key.views.each_active_internal_index([&](uint16_t view_internal_index) {
        // TODO calculate bgfx-depth value based on key
        bgfx::submit(
            view_internal_index, program_handle, sorting_value,
            BGFX_DISCARD_NONE);
    });
    bgfx::discard(BGFX_DISCARD_ALL);
}

uint32_t
Renderer::_calculate_reset_flags() const
{
    return (this->_capture ? BGFX_RESET_CAPTURE : 0) |
           (this->_vertical_sync ? BGFX_RESET_VSYNC : 0);
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

void
Renderer::setup_capture(CapturingAdapterBase* capturing_adapter)
{
    KAACORE_ASSERT(
        this->_renderer_callbacks.capturing_adapter == nullptr,
        "capturing_adapter already set");
    this->_renderer_callbacks.capturing_adapter = capturing_adapter;
    this->_capture = true;
    this->_needs_reset = true;
}

void
Renderer::clear_capture()
{
    KAACORE_ASSERT(
        this->_renderer_callbacks.capturing_adapter != nullptr,
        "capturing_adapter is not set");
    this->_renderer_callbacks.capturing_adapter = nullptr;
    this->_capture = false;
    this->_needs_reset = true;
}

} // namespace kaacore

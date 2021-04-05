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
#include "kaacore/images.h"
#include "kaacore/log.h"
#include "kaacore/memory.h"
#include "kaacore/platform.h"
#include "kaacore/renderer.h"
#include "kaacore/statistics.h"

namespace kaacore {

constexpr uint16_t _internal_view_index = 0;
constexpr uint8_t _internal_sampler_stage_index = 0;

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
        std::shared_ptr<Image>(),
        static_cast<bimg::ImageContainer*>(image_container));
    _used_containers.erase(key);
}

std::string
get_shader_model_tag(ShaderModel model)
{
    switch (model) {
        case ShaderModel::glsl:
            return "glsl";
        case ShaderModel::spriv:
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
        case PlatforType::linux:
            models = {ShaderModel::glsl, ShaderModel::spriv};
            break;
        case PlatforType::osx:
            models = {ShaderModel::metal, ShaderModel::glsl,
                      ShaderModel::spriv};
            break;
        case PlatforType::windows:
            models = {ShaderModel::hlsl_dx9, ShaderModel::hlsl_dx11,
                      ShaderModel::glsl, ShaderModel::spriv};
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

std::unique_ptr<Image>
load_default_image()
{
    // 1x1 white texture
    static const std::vector<uint8_t> image_content{0xFF, 0xFF, 0xFF, 0xFF};
    auto image_container =
        load_raw_image(bimg::TextureFormat::Enum::RGBA8, 1, 1, image_content);
    auto image = std::unique_ptr<Image>(new Image(image_container));
    bgfx::setName(image->texture_handle, "DEFAULT TEXTURE");
    return image;
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

Renderer::Renderer(bgfx::Init bgfx_init_data, const glm::uvec2& window_size)
{
    KAACORE_LOG_INFO("Initializing video.");
    KAACORE_CHECK(
        SDL_InitSubSystem(SDL_INIT_VIDEO) == 0,
        "Failed to initialize video subsystem: {}.", SDL_GetError());
    KAACORE_LOG_INFO("Initializing bgfx.");
    bgfx_init_data.resolution.width = window_size.x;
    bgfx_init_data.resolution.height = window_size.y;

    if (auto renderer_name = SDL_getenv("KAACORE_RENDERER")) {
        bgfx_init_data.type = this->_choose_renderer(renderer_name);
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

    this->texture_uniform =
        bgfx::createUniform("s_texture", bgfx::UniformType::Enum::Sampler, 1);

    this->reset();

    this->default_image = load_default_image();
    this->default_texture = this->default_image->texture_handle;

    KAACORE_LOG_INFO("Loading embedded default shader.");
    auto default_program = load_embedded_program("vs_default", "fs_default");
    KAACORE_LOG_INFO("Loading embedded sdf_font shader.");
    auto sdf_font_program = load_embedded_program("vs_default", "fs_sdf_font");
    this->default_material = Material::create(default_program);
    this->sdf_font_material = Material::create(sdf_font_program);
}

Renderer::~Renderer()
{
    KAACORE_LOG_INFO("Destroying renderer");
    bgfx::destroy(this->texture_uniform);
    this->default_image.reset();

    // since default shaders are embeded and not present
    // in registry, free them manually
    if (this->default_material) {
        this->default_material.res_ptr.get()
            ->program.res_ptr.get()
            ->vertex_shader->_uninitialize();
        this->default_material.res_ptr.get()
            ->program.res_ptr.get()
            ->fragment_shader->_uninitialize();
        this->default_material.res_ptr.get()
            ->program.res_ptr.get()
            ->_uninitialize();
    }

    if (this->sdf_font_material) {
        this->sdf_font_material.res_ptr.get()
            ->program.res_ptr.get()
            ->vertex_shader->_uninitialize();
        this->sdf_font_material.res_ptr.get()
            ->program.res_ptr.get()
            ->fragment_shader->_uninitialize();
        this->sdf_font_material.res_ptr.get()
            ->program.res_ptr.get()
            ->_uninitialize();
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
            return ShaderModel::spriv;
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
{}

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
Renderer::reset()
{
    KAACORE_LOG_DEBUG("Calling Renderer::reset()");
    auto window_size = get_engine()->window->_peek_size();
    bgfx::reset(window_size.x, window_size.y, this->_calculate_reset_flags());

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
Renderer::render_vertices(
    const uint16_t view_index, const std::vector<StandardVertexData>& vertices,
    const std::vector<VertexIndex>& indices, const bgfx::TextureHandle texture,
    const ResourceReference<Material>& material) const
{
    bgfx::TransientVertexBuffer vertices_buffer;
    bgfx::TransientIndexBuffer indices_buffer;

    bgfx::setState(
        BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
        BGFX_STATE_MSAA | BGFX_STATE_BLEND_ALPHA);
    this->_bind_material(material, texture);

    bgfx::allocTransientVertexBuffer(
        &vertices_buffer, vertices.size(), this->vertex_layout);
    bgfx::allocTransientIndexBuffer(&indices_buffer, indices.size());

    std::memcpy(
        vertices_buffer.data, vertices.data(),
        sizeof(StandardVertexData) * vertices.size());
    std::memcpy(
        indices_buffer.data, indices.data(),
        sizeof(VertexIndex) * indices.size());

    bgfx::setVertexBuffer(0, &vertices_buffer);
    bgfx::setIndexBuffer(&indices_buffer);

    bgfx::submit(view_index, material->program->_handle, false);
}

uint32_t
Renderer::_calculate_reset_flags() const
{
    return this->_vertical_sync ? BGFX_RESET_VSYNC : 0;
}

bgfx::RendererType::Enum
Renderer::_choose_renderer(const std::string& renderer_name) const
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
Renderer::_bind_material(
    const ResourceReference<Material>& material,
    bgfx::TextureHandle texture) const
{
    bgfx::setTexture(
        _internal_sampler_stage_index, this->texture_uniform, texture);
    material->_bind();
}

} // namespace kaacore

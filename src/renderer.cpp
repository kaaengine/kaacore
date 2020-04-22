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

#include "kaacore/renderer.h"

namespace kaacore {

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

template<class T, size_t N>
constexpr size_t array_size(T (&)[N])
{
    return N;
}

std::tuple<bool, const bgfx::Memory*, const bgfx::Memory*>
load_default_shaders(bgfx::RendererType::Enum renderer_type)
{
    const uint8_t* vs_data;
    const uint8_t* fs_data;
    size_t vs_data_size;
    size_t fs_data_size;
    if (renderer_type == bgfx::RendererType::Enum::OpenGL) {
        log("Loading default OpenGL GLSL shaders.");
        std::tie(fs_data, fs_data_size) = get_embedded_file_content(
            "shaders/binary/default_glsl_fragment_shader.bin");
        std::tie(vs_data, vs_data_size) = get_embedded_file_content(
            "shaders/binary/default_glsl_vertex_shader.bin");
    } else if (renderer_type == bgfx::RendererType::Enum::Direct3D9) {
        std::tie(fs_data, fs_data_size) = get_embedded_file_content(
            "shaders/binary/default_hlsl_d3d9_fragment_shader.bin");
        std::tie(vs_data, vs_data_size) = get_embedded_file_content(
            "shaders/binary/default_hlsl_d3d9_vertex_shader.bin");
    } else if (renderer_type == bgfx::RendererType::Enum::Direct3D11) {
        log("Loading default Direct3D11 HLSL shaders.");
        std::tie(fs_data, fs_data_size) = get_embedded_file_content(
            "shaders/binary/default_hlsl_d3d11_fragment_shader.bin");
        std::tie(vs_data, vs_data_size) = get_embedded_file_content(
            "shaders/binary/default_hlsl_d3d11_vertex_shader.bin");
    } else {
        log<LogLevel::warn>("No default shaders loaded");
        return std::make_tuple(false, nullptr, nullptr);
    }
    const bgfx::Memory* vs_mem = bgfx::makeRef(vs_data, vs_data_size);
    const bgfx::Memory* fs_mem = bgfx::makeRef(fs_data, fs_data_size);
    return std::make_tuple(true, vs_mem, fs_mem);
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

Renderer::Renderer(const glm::uvec2& window_size)
{
    log("Initializing renderer.");
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

    auto renderer_type = bgfx::getRendererType();

    const bgfx::Memory* vs_mem;
    const bgfx::Memory* fs_mem;

    bool found_defaults;
    std::tie(found_defaults, vs_mem, fs_mem) =
        load_default_shaders(renderer_type);
    if (!found_defaults) {
        log<LogLevel::error>(
            "Can't find precompiled shaders for this platform!");
        this->default_program = BGFX_INVALID_HANDLE;
        return;
    }

    auto vs = bgfx::createShader(vs_mem);
    auto fs = bgfx::createShader(fs_mem);

    log("Created shaders, VS: %d, FS: %d.", vs, fs);

    this->default_program = bgfx::createProgram(vs, fs, true);

    log("Created program: %d.", this->default_program);
}

Renderer::~Renderer()
{
    log("Destroying renderer");
    bgfx::destroy(this->texture_uniform);
    bgfx::destroy(this->default_program);
}

bgfx::TextureHandle
Renderer::make_texture(
    std::shared_ptr<bimg::ImageContainer> image_container,
    const uint64_t flags) const
{
    KAACORE_ASSERT(bgfx::isTextureValid(
        0, false, image_container->m_numLayers,
        bgfx::TextureFormat::Enum(image_container->m_format), flags));

    const bgfx::Memory* memory = bgfx::makeRef(
        image_container->m_data, image_container->m_size,
        _release_used_container, image_container.get());

    auto handle = bgfx::createTexture2D(
        uint16_t(image_container->m_width), uint16_t(image_container->m_height),
        1 < image_container->m_numMips, image_container->m_numLayers,
        bgfx::TextureFormat::Enum(image_container->m_format), flags, memory);

    KAACORE_ASSERT(bgfx::isValid(handle));
    _used_containers.insert(std::move(image_container));
    return handle;
}

void
Renderer::destroy_texture(const bgfx::TextureHandle& handle) const
{
    KAACORE_ASSERT_TERMINATE(bgfx::isValid(handle));
    bgfx::destroy(handle);
}

void
Renderer::begin_frame()
{}

void
Renderer::end_frame()
{
    // TODO: optimize!
    for (int i = 0; i < KAACORE_MAX_VIEWS; ++i) {
        bgfx::touch(i);
    }
    bgfx::frame();
}

void
Renderer::reset()
{
    log<LogLevel::debug>("Calling Renderer::reset()");
    auto window_size = get_engine()->window->size();
    bgfx::reset(window_size.x, window_size.y, this->_reset_flags);

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
    const std::vector<VertexIndex>& indices,
    const bgfx::TextureHandle texture) const
{
    bgfx::TransientVertexBuffer vertices_buffer;
    bgfx::TransientIndexBuffer indices_buffer;

    bgfx::setState(
        BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
        BGFX_STATE_MSAA | BGFX_STATE_BLEND_ALPHA);

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
    bgfx::setTexture(0, this->texture_uniform, texture);

    bgfx::submit(view_index, this->default_program, false);
}

} // namespace kaacore

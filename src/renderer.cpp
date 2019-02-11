#include <iterator>
#include <cstring>
#include <tuple>

#include <bgfx/bgfx.h>

#include "kaacore/renderer.h"
#include "kaacore/log.h"
#include "kaacore/files.h"
#include "kaacore/texture_loader.h"
#include "kaacore/embedded_data.h"


template<class T, size_t N>
constexpr size_t array_size(T (&)[N]) { return N; }


std::tuple<bool, const bgfx::Memory*, const bgfx::Memory*>
load_default_shaders(bgfx::RendererType::Enum renderer_type)
{
    const uint8_t* vs_data;
    const uint8_t* fs_data;
    size_t vs_data_size;
    size_t fs_data_size;
    if (renderer_type == bgfx::RendererType::Enum::OpenGL) {
        log("Loading default OpenGL GLSL shaders");
        vs_data = default_glsl_vertex_shader;
        fs_data = default_glsl_fragment_shader;
        vs_data_size = array_size(default_glsl_vertex_shader);
        fs_data_size = array_size(default_glsl_fragment_shader);
    } else if (renderer_type == bgfx::RendererType::Enum::Direct3D9) {
        log("Loading default Direct3D9 HLSL shaders");
        vs_data = default_hlsl_d3d9_vertex_shader;
        fs_data = default_hlsl_d3d9_fragment_shader;
        vs_data_size = array_size(default_hlsl_d3d9_vertex_shader);
        fs_data_size = array_size(default_hlsl_d3d9_fragment_shader);
    } else {
        log<LogLevel::warn>("No default shaders loaded");
        return std::make_tuple(false, nullptr, nullptr);
    }
    const bgfx::Memory* vs_mem = bgfx::makeRef(vs_data, vs_data_size);
    const bgfx::Memory* fs_mem = bgfx::makeRef(fs_data, fs_data_size);
    return std::make_tuple(true, vs_mem, fs_mem);
}


bgfx::TextureHandle load_default_texture()
{
    auto texture = load_texture(default_texture, array_size(default_texture));
    bgfx::setName(texture, "DEFAULT TEXTURE");
    return texture;
}


Renderer::Renderer() 
{
    log("Initializing renderer");
    this->reset_flags = BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X2;

    this->vertex_decl.begin() \
        .add(bgfx::Attrib::Enum::Position, 3, bgfx::AttribType::Enum::Float) \
        .add(bgfx::Attrib::Enum::TexCoord0, 2, bgfx::AttribType::Enum::Float) \
        .add(bgfx::Attrib::Enum::TexCoord1, 2, bgfx::AttribType::Enum::Float) \
        .add(bgfx::Attrib::Enum::Color0, 4, bgfx::AttribType::Enum::Float) \
        .end();

    this->texture_uniform = bgfx::createUniform(
        "s_texture", bgfx::UniformType::Enum::Int1, 1
    );

    bgfx::setViewClear(0, this->reset_flags);

    this->default_texture = load_default_texture();

    auto renderer_type = bgfx::getRendererType();

    const bgfx::Memory* vs_mem;
    const bgfx::Memory* fs_mem;

    bool found_defaults;
    std::tie(found_defaults, vs_mem, fs_mem) = load_default_shaders(renderer_type);
    if (!found_defaults) {
        log<LogLevel::error>("Can't find precompiled shaders for this platform!");
        this->default_program = BGFX_INVALID_HANDLE;
        return;
    }

    auto vs = bgfx::createShader(vs_mem);
    auto fs = bgfx::createShader(fs_mem);

    log("Created shaders, VS: %d, FS: %d", vs, fs);

    this->default_program = bgfx::createProgram(vs, fs, true);

    log("Created program: %d", this->default_program);
}


Renderer::~Renderer() {
    log("Destroying renderer");
    bgfx::destroy(this->texture_uniform);
    bgfx::destroy(this->default_program);
}


void Renderer::begin_frame()
{
    // TODO prepare buffers, etc.
}


void Renderer::end_frame()
{
    bgfx::touch(0);
    bgfx::frame();
}


void Renderer::render_vertices(std::vector<StandardVertexData> vertices,
                               std::vector<uint16_t> indices,
                               bgfx::TextureHandle texture)
{
    bgfx::TransientVertexBuffer vertices_buffer;
    bgfx::TransientIndexBuffer indices_buffer;

    bgfx::setState(BGFX_STATE_WRITE_RGB \
                 | BGFX_STATE_WRITE_A \
                 | BGFX_STATE_WRITE_Z \
                 | BGFX_STATE_MSAA \
                 | BGFX_STATE_BLEND_ALPHA);

    bgfx::allocTransientVertexBuffer(&vertices_buffer, vertices.size(),
                                     this->vertex_decl);
    bgfx::allocTransientIndexBuffer(&indices_buffer, indices.size());

    std::memcpy(vertices_buffer.data, vertices.data(),
        sizeof(StandardVertexData) * vertices.size());
    std::memcpy(indices_buffer.data, indices.data(),
        sizeof(uint16_t) * indices.size());

    bgfx::setVertexBuffer(0, &vertices_buffer);
    bgfx::setIndexBuffer(&indices_buffer);
    bgfx::setTexture(0, this->texture_uniform, texture);

    bgfx::submit(0, this->default_program, false);
}

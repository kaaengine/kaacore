#include <algorithm>
#include <memory>
#include <sstream>
#include <utility>

#include "kaacore/embedded_data.h"
#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/files.h"
#include "kaacore/platform.h"
#include "kaacore/renderer.h"
#include "kaacore/shaders.h"

#include <cmrc/cmrc.hpp>

namespace kaacore {

ResourcesRegistry<ShaderKey, Shader> _shaders_registry;
ResourcesRegistry<ProgramKey, Program> _programs_registry;
std::unordered_set<Memory> _used_memory;

void
initialize_shaders()
{
    _shaders_registry.initialze();
    _programs_registry.initialze();
}

void
uninitialize_shaders()
{
    _programs_registry.uninitialze();
    _shaders_registry.uninitialze();
}

void
_release_used_memory(void* memory, void* _user_data)
{
    auto key = Memory::reference(static_cast<std::byte*>(memory), 0);
    _used_memory.erase(key);
}

Memory
_load_shader(const std::string& path)
{
    File file(path);
    return Memory::copy(
        reinterpret_cast<std::byte*>(file.content.data()), file.content.size()
    );
}

Memory
_load_embedded_shader_memory(const std::string& path)
{
    try {
        return get_embedded_file_content(embedded_shaders_filesystem, path);
    } catch (embedded_file_error& err) {
        KAACORE_LOG_ERROR(
            "Failed to load embedded binary shader: {} ({})", path, err.what()
        );
        throw;
    }
}

std::string
_get_shader_model_tag(ShaderModel model)
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

ShaderModelMemoryMap
_load_embedded_shader_memory_map(
    const std::string& shader_name, const PlatformType platform
)
{
    std::vector<ShaderModel> models;
    switch (platform) {
        case PlatformType::linux:
            models = {ShaderModel::glsl, ShaderModel::spirv};
            break;
        case PlatformType::osx:
            models = {
                ShaderModel::metal, ShaderModel::glsl, ShaderModel::spirv
            };
            break;
        case PlatformType::windows:
            models = {
                ShaderModel::hlsl_dx9, ShaderModel::hlsl_dx11,
                ShaderModel::glsl, ShaderModel::spirv
            };
            break;
        default:
            KAACORE_LOG_ERROR(
                "Unsupported platform! Can't load embedded shaders."
            );
    }

    std::string path;
    ShaderModelMemoryMap memory_map;
    for (auto model : models) {
        auto shader_model_tag = _get_shader_model_tag(model);
        path = fmt::format("{}/{}.bin", shader_model_tag, shader_name);
        memory_map[model] = _load_embedded_shader_memory(path);
    }
    return memory_map;
}

Shader::Shader(const ShaderType type, const ShaderModelMemoryMap& memory_map)
    : _type(type), _models(memory_map)
{
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

Shader::Shader(const ShaderType type, ShaderModelMemoryMap&& memory_map)
    : _models(std::move(memory_map)), _type(type)
{
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

Shader::~Shader()
{
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

ResourceReference<Shader>
Shader::load(const ShaderType type, const ShaderModelMap& model_map)
{
    ShaderKey key;
    key.reserve(model_map.size());
    for (auto& kv_pair : model_map) {
        key.push_back(kv_pair.second);
    }
    std::sort(key.begin(), key.end());
    std::shared_ptr<Shader> shader;
    if ((shader = _shaders_registry.get_resource(key))) {
        return shader;
    }

    ShaderModelMemoryMap memory_map;
    for (auto& kv_pair : model_map) {
        memory_map[kv_pair.first] = std::move(_load_shader(kv_pair.second));
    }

    shader = std::shared_ptr<Shader>(new Shader(type, std::move(memory_map)));
    _shaders_registry.register_resource(key, shader);
    return shader;
}

ResourceReference<Shader>
Shader::create(const ShaderType type, const ShaderModelMemoryMap& memory_map)
{
    return std::shared_ptr<Shader>(new Shader(type, memory_map));
}

const Memory
Shader::memory()
{
    return this->_models[this->_used_model];
}

ShaderType
Shader::type() const
{
    return this->_type;
}

void
Shader::_initialize()
{
    auto renderer = get_engine()->renderer.get();
    auto model = renderer->shader_model();
    if (renderer->type() == RendererType::noop) {
        // Grab the first model, it doesn't matter which one
        // since rendering is disabled.
        model = this->_models.begin()->first;
    }
    if (this->_models.find(model) == this->_models.end()) {
        auto msg = fmt::format(
            "No suitable shader provided for renderer type: {}.",
            renderer->type()
        );
        throw kaacore::exception(msg);
    }

    auto& memory = this->_models[model];
    _used_memory.insert(memory);
    auto bgfx_memory =
        bgfx::makeRef(memory.get(), memory.size(), _release_used_memory);
    this->_handle = bgfx::createShader(bgfx_memory);
    if (not bgfx::isValid(this->_handle)) {
        throw kaacore::exception("Can't create shader.");
    }
    this->_used_model = model;
    this->is_initialized = true;
}

void
Shader::_uninitialize()
{
    bgfx::destroy(this->_handle);
    this->is_initialized = false;
}

EmbeddedShader::EmbeddedShader(const ShaderType type, const std::string& name)
    : _name(name)
{
    this->_type = type;
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

ResourceReference<EmbeddedShader>
EmbeddedShader::load(const ShaderType type, const std::string& shader_name)
{
    ShaderKey key{fmt::format("EMBEDDED::{}", shader_name), "str"};
    auto shader = std::static_pointer_cast<EmbeddedShader>(
        _shaders_registry.get_resource(key)
    );
    if (shader) {
        return shader;
    }

    shader =
        std::shared_ptr<EmbeddedShader>(new EmbeddedShader(type, shader_name));
    _shaders_registry.register_resource(key, shader);
    return shader;
}

void
EmbeddedShader::_initialize()
{
    this->_models =
        _load_embedded_shader_memory_map(this->_name, get_platform());
    Shader::_initialize();
}

Program::Program() : vertex_shader(nullptr), fragment_shader(nullptr) {}

Program::Program(
    const ResourceReference<Shader>& vertex,
    const ResourceReference<Shader>& fragment
)
    : vertex_shader(vertex), fragment_shader(fragment)
{
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

Program::~Program()
{
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

ResourceReference<Program>
Program::create(
    const ResourceReference<Shader>& vertex,
    const ResourceReference<Shader>& fragment
)
{
    std::shared_ptr<Program> program;
    ProgramKey key =
        std::make_pair(vertex.res_ptr.get(), fragment.res_ptr.get());

    if ((program = _programs_registry.get_resource(key))) {
        return program;
    }
    program = std::shared_ptr<Program>(new Program(vertex, fragment));
    _programs_registry.register_resource(key, program);
    return program;
}

void
Program::_initialize()
{
    this->_validate_shaders();
    this->_handle = bgfx::createProgram(
        this->vertex_shader->_handle, this->fragment_shader->_handle
    );
    if (not bgfx::isValid(this->_handle)) {
        std::ostringstream stream;
        stream << "Can't create program (vs handle: "
               << this->vertex_shader->_handle.idx
               << ", fs handle: " << this->fragment_shader->_handle.idx << ")";
        throw kaacore::exception(stream.str());
    }
    this->is_initialized = true;
}

void
Program::_uninitialize()
{
    bgfx::destroy(this->_handle);
    this->is_initialized = false;
}

void
Program::_validate_shaders()
{
    auto vertex_memory = this->vertex_shader->memory();
    auto fragment_memory = this->fragment_shader->memory();
    auto hash_size = sizeof(uint32_t);
    std::size_t input_offset = hash_size, output_offset = hash_size * 2;
    KAACORE_CHECK(
        (vertex_memory.size() >= output_offset + hash_size),
        "Invalid vertex shader format."
    );
    KAACORE_CHECK(
        (fragment_memory.size() >= input_offset + hash_size),
        "Invalid fragment shader format."
    );
    auto match = std::memcmp(
                     vertex_memory.get() + output_offset,
                     fragment_memory.get() + input_offset, hash_size
                 ) == 0;
    KAACORE_CHECK(
        match, "Vertex shader output doesn't match fragment shader input."
    );
}

} // namespace kaacore

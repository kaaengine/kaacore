#include <memory>
#include <sstream>
#include <utility>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/files.h"
#include "kaacore/shaders.h"

namespace kaacore {

ResourcesRegistry<ShaderKey, Shader> _shaders_registry;
ResourcesRegistry<ProgramKey, Program> _programs_registry;

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

Memory
_load_shader(const std::string& path)
{
    File file(path);
    return Memory::copy(
        reinterpret_cast<std::byte*>(file.content.data()), file.content.size());
}

Shader::Shader(const ShaderModelMemoryMap& memory_map, const ShaderType type)
    : _models(memory_map), _type(type)
{
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

Shader::Shader(ShaderModelMemoryMap&& memory_map, const ShaderType type)
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
    // TODO: releaseFn
}

ResourceReference<Shader>
Shader::load(const ShaderType type, const ShaderModelMap& model_map)
{
    ShaderKey key;
    key.reserve(model_map.size());
    for (auto& kv_pair : model_map) {
        key.push_back(kv_pair.second);
    }
    std::shared_ptr<Shader> shader;
    if ((shader = _shaders_registry.get_resource(key))) {
        return shader;
    }

    ShaderModelMemoryMap memory_map;
    for (auto& kv_pair : model_map) {
        memory_map[kv_pair.first] = std::move(_load_shader(kv_pair.second));
    }

    shader = std::shared_ptr<Shader>(new Shader(std::move(memory_map), type));
    _shaders_registry.register_resource(key, shader);
    return shader;
}

ResourceReference<Shader>
Shader::create(const ShaderType type, const ShaderModelMemoryMap memory_map)
{
    return std::shared_ptr<Shader>(new Shader(memory_map, type));
}

ShaderType
Shader::type() const
{
    return this->_type;
}

void
Shader::_initialize()
{
    auto model = get_engine()->renderer->shader_model();
    if (this->_models.find(model) == this->_models.end()) {
        auto msg = fmt::format(
            "No suitable shader provided for renderer type: {}.",
            get_engine()->renderer->type());
        throw kaacore::exception(msg);
    }

    auto& memory = this->_models[model];
    // TODO: releaseFn
    auto bgfx_memory = bgfx::makeRef(memory.get(), memory.size());
    this->_handle = bgfx::createShader(bgfx_memory);
    if (not bgfx::isValid(this->_handle)) {
        throw kaacore::exception("Can't create shader TODO.");
    }
    this->is_initialized = true;
}

void
Shader::_uninitialize()
{
    bgfx::destroy(this->_handle);
    this->is_initialized = false;
}

Program::Program() : vertex_shader(nullptr), fragment_shader(nullptr) {}

Program::Program(
    const ResourceReference<Shader>& vertex,
    const ResourceReference<Shader>& fragment)
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
    const ResourceReference<Shader>& fragment)
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
    this->_handle = bgfx::createProgram(
        this->vertex_shader->_handle, this->fragment_shader->_handle);
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

} // namespace kaacore

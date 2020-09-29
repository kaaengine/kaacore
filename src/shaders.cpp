#include <memory>
#include <sstream>
#include <utility>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/files.h"
#include "kaacore/shaders.h"

namespace kaacore {

ResourcesRegistry<std::string, Shader> _shaders_registry;
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

const bgfx::Memory*
_load_shader(const std::string& path)
{
    RawFile file(path);
    return bgfx::copy(file.content.data(), file.content.size());
}

Shader::Shader(const std::string& path)
    : path(path), _memory(_load_shader(path))
{
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

Shader::Shader(const bgfx::Memory* memory) : _memory(memory)
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

bgfx::ShaderHandle
Shader::handle()
{
    return this->_handle;
}

ResourceReference<Shader>
Shader::load(const std::string& path)
{
    std::shared_ptr<Shader> shader;
    if ((shader = _shaders_registry.get_resource(path))) {
        return shader;
    }
    shader = std::shared_ptr<Shader>(new Shader(path));
    _shaders_registry.register_resource(path, shader);
    return shader;
}

ResourceReference<Shader>
Shader::load(const bgfx::Memory* memory)
{
    return std::shared_ptr<Shader>(new Shader(memory));
}

void
Shader::_initialize()
{
    this->_handle = bgfx::createShader(this->_memory);
    if (not bgfx::isValid(this->_handle)) {
        throw kaacore::exception(
            "Can't create shader (path:" + this->path + ").");
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

bgfx::ProgramHandle
Program::handle()
{
    return this->_handle;
}

ResourceReference<Program>
Program::load(
    const ResourceReference<Shader>& vertex,
    const ResourceReference<Shader>& fragment)
{
    std::shared_ptr<Program> program;
    ProgramKey key = std::make_pair(vertex->_handle.idx, fragment->_handle.idx);

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

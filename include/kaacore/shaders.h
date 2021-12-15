#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <bgfx/bgfx.h>

#include "kaacore/memory.h"
#include "kaacore/resources.h"
#include "kaacore/utils.h"

namespace kaacore {

class Shader;

using ShaderKey = std::vector<std::string>;
using ProgramKey = std::pair<Shader*, Shader*>;

void
initialize_shaders();
void
uninitialize_shaders();

enum class ShaderType { vertex, fragment };

enum class ShaderModel { hlsl_dx9, hlsl_dx11, glsl, spirv, metal, unknown };

using ShaderModelMap = std::unordered_map<ShaderModel, std::string>;
using ShaderModelMemoryMap = std::unordered_map<ShaderModel, Memory>;

class Effect;
class Program;
class Renderer;

class Shader : public Resource {
  public:
    Shader() = default;
    ~Shader();
    const Memory memory();
    ShaderType type() const;
    static ResourceReference<Shader> load(
        const ShaderType type, const ShaderModelMap& model_map);
    static ResourceReference<Shader> create(
        const ShaderType type, const ShaderModelMemoryMap& memory_map);

  protected:
    ShaderType _type;
    ShaderModelMemoryMap _models;
    ShaderModel _used_model = ShaderModel::unknown;
    bgfx::ShaderHandle _handle = BGFX_INVALID_HANDLE;

    Shader(const ShaderType type, const ShaderModelMemoryMap& model_map);
    Shader(const ShaderType type, ShaderModelMemoryMap&& model_map);
    virtual void _initialize() override;
    virtual void _uninitialize() override;

    friend class Effect;
    friend class Program;
    friend class Renderer;
    friend class ResourcesRegistry<ShaderKey, Shader>;
};

class EmbeddedShader : public Shader {
  public:
    static ResourceReference<EmbeddedShader> load(
        const ShaderType type, const std::string& shader_name);

  private:
    const std::string _name;

    EmbeddedShader(const ShaderType type, const std::string& _name);
    virtual void _initialize() override;

    friend class ResourcesRegistry<ShaderKey, Shader>;
};

class Program : public Resource {
  public:
    const ResourceReference<Shader> vertex_shader;
    const ResourceReference<Shader> fragment_shader;

    Program();
    ~Program();
    static ResourceReference<Program> create(
        const ResourceReference<Shader>& vertex,
        const ResourceReference<Shader>& fragment);

  private:
    bgfx::ProgramHandle _handle = BGFX_INVALID_HANDLE;

    Program(
        const ResourceReference<Shader>& vertex,
        const ResourceReference<Shader>& fragment);
    virtual void _initialize() override;
    virtual void _uninitialize() override;
    void _validate_shaders();

    friend class Renderer;
    friend class ResourcesRegistry<ProgramKey, Program>;
};
} // namespace kaacore

namespace std {
template<>
struct hash<kaacore::ProgramKey> {
    size_t operator()(const kaacore::ProgramKey& key) const
    {
        return kaacore::hash_combined(key.first, key.second);
    }
};

template<>
struct hash<kaacore::ShaderKey> {
    size_t operator()(const kaacore::ShaderKey& key) const
    {
        using shader_key_iterator = kaacore::ShaderKey::const_iterator;
        return kaacore::hash_iterable<std::string, shader_key_iterator>(
            key.begin(), key.end());
    }
};
}

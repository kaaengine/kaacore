#pragma once

#include <bgfx/bgfx.h>
#include <string>

#include "kaacore/resources.h"
#include "kaacore/utils.h"

namespace kaacore {

typedef std::pair<uint16_t, uint16_t> ProgramKey;

void
initialize_shaders();
void
uninitialize_shaders();

class Program;
class Renderer;

class Shader : public Resource {
  public:
    const std::string path;

    Shader() = default;
    ~Shader();
    bgfx::ShaderHandle handle();
    static ResourceReference<Shader> load(const std::string& path);
    static ResourceReference<Shader> load(const bgfx::Memory* memory);

  private:
    bgfx::ShaderHandle _handle = BGFX_INVALID_HANDLE;
    const bgfx::Memory* _memory;

    Shader(const std::string& path);
    Shader(const bgfx::Memory* memory);
    virtual void _initialize() override;
    virtual void _uninitialize() override;

    friend class Program;
    friend class Renderer;
    friend class ResourcesRegistry<std::string, Shader>;
};

class Program : public Resource {
  public:
    const ResourceReference<Shader> vertex_shader;
    const ResourceReference<Shader> fragment_shader;

    Program();
    ~Program();
    bgfx::ProgramHandle handle();
    static ResourceReference<Program> load(
        const ResourceReference<Shader>& vertex,
        const ResourceReference<Shader>& fragment);

  private:
    bgfx::ProgramHandle _handle = BGFX_INVALID_HANDLE;

    Program(
        const ResourceReference<Shader>& vertex,
        const ResourceReference<Shader>& fragment);
    virtual void _initialize() override;
    virtual void _uninitialize() override;

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
}

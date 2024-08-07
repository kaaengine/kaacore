#pragma once

#include <atomic>
#include <limits>
#include <string>
#include <unordered_map>

#include "kaacore/resources.h"
#include "kaacore/shaders.h"
#include "kaacore/textures.h"
#include "kaacore/uniforms.h"

namespace kaacore {

using MaterialId = uint32_t;

void
initialize_materials();
void
uninitialize_materials();

class ShadingContext : public Resource {
  public:
    ShadingContext() = default;
    ShadingContext(const UniformSpecificationMap& uniforms);
    ~ShadingContext();
    UniformSpecificationMap uniforms() const;
    void set_uniform_texture(
        const std::string& name, const ResourceReference<Texture>& texture,
        const uint8_t stage,
        const uint32_t flags = std::numeric_limits<uint32_t>::max()
    );
    void set_uniform_texture(
        const std::string& name, const SamplerValue& value
    );
    std::optional<SamplerValue> get_uniform_texture(const std::string& name
    ) const;

    template<typename T>
    std::vector<T> get_uniform_value(const std::string& name) const
    {
        KAACORE_CHECK(
            this->_name_in_registry(name), "Unknown uniform name: {}.", name
        );
        return std::get<FloatUniform<T>>(this->_uniforms.at(name)).get();
    }
    template<typename T>
    void set_uniform_value(const std::string& name, UniformValue<T>&& value)
    {
        KAACORE_CHECK(
            this->_name_in_registry(name), "Unknown uniform name: {}.", name
        );
        std::get<FloatUniform<T>>(this->_uniforms[name]).set(std::move(value));
    }
    void bind(const std::string& name);
    void bind();

  protected:
    std::unordered_map<std::string, UniformVariant> _uniforms;

    virtual void _initialize() override;
    virtual void _uninitialize() override;
    bool _name_in_registry(const std::string& name) const;
    void _set_uniform_texture(
        const std::string& name, const Texture* texture, const uint8_t stage,
        const uint32_t flags = std::numeric_limits<uint32_t>::max()
    );
};

class Material : public ShadingContext {
  public:
    const ResourceReference<Program> program;

    static ResourceReference<Material> create(
        const ResourceReference<Program>& program,
        const UniformSpecificationMap& uniforms = {}
    );
    ResourceReference<Material> clone() const;
    void set_uniform_texture(
        const std::string& name, const ResourceReference<Texture>& texture,
        const uint8_t stage,
        const uint32_t flags = std::numeric_limits<uint32_t>::max()
    );
    void set_uniform_texture(
        const std::string& name, const SamplerValue& value
    );

  private:
    MaterialId _id;
    static inline std::atomic<MaterialId> _last_id = 0u;

    Material(
        const MaterialId id, const ResourceReference<Program>& program,
        const UniformSpecificationMap& uniforms
    );

    friend class ResourcesRegistry<MaterialId, Material>;
};

} // namespace kaacore

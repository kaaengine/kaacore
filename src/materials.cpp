#include <memory>
#include <unordered_set>

#include "kaacore/engine.h"
#include "kaacore/materials.h"
#include "kaacore/renderer.h"

namespace kaacore {

ResourcesRegistry<MaterialId, Material> _materials_registry;

void
initialize_materials()
{
    _materials_registry.initialze();
}

void
uninitialize_materials()
{
    _materials_registry.uninitialze();
}

ShadingContext::ShadingContext(const UniformSpecificationMap& uniforms)
{
    for (auto& kv_pair : uniforms) {
        const auto& [name, uniform] = kv_pair;
        switch (uniform.type()) {
            case UniformType::sampler:
                this->_uniforms.emplace(name, Sampler(name));
                break;
            case UniformType::vec4:
                this->_uniforms.emplace(
                    name, Vec4Uniform(name, uniform.number_of_elements()));
                break;
            case UniformType::mat3:
                this->_uniforms.emplace(
                    name, Mat3Uniform(name, uniform.number_of_elements()));
                break;
            case UniformType::mat4:
                this->_uniforms.emplace(
                    name, Mat4Uniform(name, uniform.number_of_elements()));
                break;
            default:
                throw kaacore::exception("Unknown uniform type.");
        }
    }

    if (is_engine_initialized()) {
        this->_initialize();
    }
}

ShadingContext::~ShadingContext()
{
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

UniformSpecificationMap
ShadingContext::uniforms() const
{
    UniformSpecificationMap result;
    result.reserve(this->_uniforms.size());
    for (auto& kv_pair : this->_uniforms) {
        std::visit(
            [&result](auto&& variant) {
                result[variant.name()] = variant.specification();
            },
            kv_pair.second);
    }
    return result;
}

std::optional<SamplerValue>
ShadingContext::get_uniform_texture(const std::string& name) const
{
    KAACORE_CHECK(
        this->_name_in_registry(name), "Unknown uniform name: {}.", name);
    return std::get<Sampler>(this->_uniforms.at(name)).get();
}

void
ShadingContext::set_uniform_texture(
    const std::string& name, const ResourceReference<Texture>& texture,
    const uint8_t stage, const uint32_t flags)
{
    KAACORE_CHECK(
        this->_name_in_registry(name), "Unknown uniform name: {}.", name);
    std::get<Sampler>(this->_uniforms[name]).set(texture.res_ptr, stage, flags);
}

void
ShadingContext::bind(const std::string& name)
{
    std::visit(
        [](auto&& variant) { variant._bind(); }, this->_uniforms.at(name));
}

void
ShadingContext::bind()
{
    for (auto& kv_pair : this->_uniforms) {
        std::visit([](auto&& variant) { variant._bind(); }, kv_pair.second);
    }
}

void
ShadingContext::_initialize()
{
    for (auto& kv_pair : this->_uniforms) {
        std::visit(
            [](auto&& variant) { variant._initialize(); }, kv_pair.second);
    }
    this->is_initialized = true;
}

void
ShadingContext::_uninitialize()
{
    for (auto& kv_pair : this->_uniforms) {
        std::visit(
            [](auto&& variant) { variant._uninitialize(); }, kv_pair.second);
    }
    this->is_initialized = false;
}

bool
ShadingContext::_name_in_registry(const std::string& name) const
{
    return this->_uniforms.find(name) != this->_uniforms.end();
}

Material::Material(
    const MaterialId id, const ResourceReference<Program>& program,
    const UniformSpecificationMap& uniforms)
    : ShadingContext(uniforms), program(program), _id(id)
{}

ResourceReference<Material>
Material::create(
    const ResourceReference<Program>& program,
    const UniformSpecificationMap& uniforms)
{
    auto reserved_names = Renderer::reserved_uniform_names();
    std::unordered_set<std::string> errors;
    for (const auto& kv_pair : uniforms) {
        const auto& name = kv_pair.first;
        if (not name.size()) {
            errors.insert("Blank uniform names are not allowed.");
        }
        if (reserved_names.find(name) != reserved_names.end()) {
            errors.insert(fmt::format(
                "{} is reserved for internal use, use other uniform name.",
                name));
        }
    }
    KAACORE_CHECK(
        not errors.size(), "Can't create material:\n{}",
        fmt::join(errors.begin(), errors.end(), "\n"));

    auto id = Material::_last_id.fetch_add(1, std::memory_order_relaxed);
    auto material =
        std::shared_ptr<Material>(new Material(id, program, uniforms));
    _materials_registry.register_resource(id, material);
    return material;
}

ResourceReference<Material>
Material::clone() const
{
    auto uniforms = this->uniforms();
    auto material = Material::create(this->program, uniforms);
    for (auto& kv_pair : uniforms) {
        const auto& [name, uniform] = kv_pair;
        switch (uniform.type()) {
            case UniformType::sampler:
                if (auto sampler_value_opt = this->get_uniform_texture(name)) {
                    auto sampler_value = sampler_value_opt.value();
                    material->set_uniform_texture(
                        name,
                        sampler_value.texture,
                        sampler_value.stage,
                        sampler_value.flags
                    );
                }
                break;
            case UniformType::vec4:
                material->set_uniform_value<glm::fvec4>(
                    name, this->get_uniform_value<glm::fvec4>(name));
                break;
            case UniformType::mat3:
                material->set_uniform_value<glm::fmat3>(
                    name, this->get_uniform_value<glm::fmat3>(name));
                break;
            case UniformType::mat4:
                material->set_uniform_value<glm::fmat4>(
                    name, this->get_uniform_value<glm::fmat4>(name));
                break;
            default:
                throw kaacore::exception("Unknown uniform type.");
        }
    }
    return material;
}

void
Material::set_uniform_texture(
    const std::string& name, const ResourceReference<Texture>& texture,
    const uint8_t stage, const uint32_t flags)
{
    KAACORE_CHECK(stage > 0, "Stage index must be greater than zero.");
    ShadingContext::set_uniform_texture(name, texture, stage, flags);
}

}

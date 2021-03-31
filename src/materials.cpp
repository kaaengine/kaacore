#include <memory>
#include <unordered_set>

#include "kaacore/engine.h"
#include "kaacore/materials.h"

namespace kaacore {

ResourcesRegistry<MaterialID, Material> _materials_registry;
std::unordered_set<std::string> _reserved_uniform_names({"s_texture"});

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

UniformSpecification::UniformSpecification(
    const UniformType type, const uint16_t number_of_elements)
    : _type(type), _number_of_elements(number_of_elements)
{
    if (type == UniformType::sampler) {
        KAACORE_CHECK(
            number_of_elements == 1, "",
            "Sampler uniform must not have multiple elements.");
    }
}

UniformType
UniformSpecification::type() const
{
    return this->_type;
}

uint16_t
UniformSpecification::number_of_elements() const
{
    return this->_number_of_elements;
}

UniformBase::UniformBase(
    const std::string& name, const UniformType type,
    const uint16_t number_of_elements)
    : _name(name), _type(type), _number_of_elements(number_of_elements)
{
    KAACORE_CHECK(
        _reserved_uniform_names.find(name) == _reserved_uniform_names.end(),
        "{} is reserved for internal use, use other uniform name.", name);
};

UniformBase::UniformBase(const UniformBase& other)
    : _name(other._name), _type(other._type),
      _number_of_elements(other._number_of_elements){};

UniformBase::UniformBase(UniformBase&& other)
    : _name(std::move(other._name)), _type(other._type),
      _number_of_elements(other._number_of_elements)
{
    this->_handle = other._handle;
    other._handle = BGFX_INVALID_HANDLE;
}
UniformBase&
UniformBase::operator=(UniformBase&& other)
{
    if (this == &other) {
        return *this;
    }

    this->_name = std::move(other._name);
    this->_type = other._type;
    this->_number_of_elements = other._number_of_elements;
    other._number_of_elements = 0;
    return *this;
}

UniformType
UniformBase::type() const
{
    return this->_type;
}

std::string
UniformBase::name() const
{
    return this->_name;
}

uint16_t
UniformBase::number_of_elements() const
{
    return this->_number_of_elements;
}

UniformSpecification
UniformBase::specification() const
{
    return {this->_type, this->_number_of_elements};
}

void
UniformBase::_initialize()
{
    this->_handle = bgfx::createUniform(
        this->_name.c_str(), this->_internal_type(), this->_number_of_elements);
}

void
UniformBase::_uninitialize()
{
    if (bgfx::isValid(this->_handle)) {
        bgfx::destroy(this->_handle);
    }
}

bgfx::UniformType::Enum
UniformBase::_internal_type()
{
    return static_cast<bgfx::UniformType::Enum>(this->_type);
}

Sampler::Sampler() : UniformBase("", UniformType::sampler) {}

Sampler::Sampler(const std::string& name)
    : UniformBase(name, UniformType::sampler){};

Sampler::Sampler(const Sampler& other)
    : UniformBase(other), _stage(other._stage), _flags(other._flags),
      _value(other._value), _texture(other._texture)
{}

Sampler::Sampler(Sampler&& other)
    : UniformBase(std::move(other)), _stage(other._stage), _flags(other._flags),
      _value(other._value), _texture(std::move(other._texture))
{
    other._stage = 0u;
    other._flags = 0u;
    other._value = BGFX_INVALID_HANDLE;
}

Sampler&
Sampler::operator=(Sampler&& other)
{
    if (this == &other) {
        return *this;
    }

    UniformBase::operator=(std::move(other));

    this->_stage = other._stage;
    this->_flags = other._flags;
    this->_value = other._value;
    this->_texture = std::move(other._texture);

    other._stage = 0u;
    other._flags = 0u;
    other._value = BGFX_INVALID_HANDLE;

    return *this;
}

SamplerValue
Sampler::get() const
{
    return {this->_stage, this->_flags, this->_texture.lock()};
}

void
Sampler::set(
    const ResourceReference<Image>& texture, const uint8_t stage,
    const uint32_t flags)
{
    KAACORE_CHECK(stage > 0, "Stage index must be greater than zero.");
    this->_value = texture->texture_handle;
    this->_texture = texture.res_ptr;
    this->_stage = stage;
    this->_flags = flags;
}

void
Sampler::set(const SamplerValue& value)
{
    KAACORE_CHECK(value.stage > 0, "Stage index must be greater than zero.");
    this->_value = value.texture->texture_handle;
    this->_texture = value.texture.res_ptr;
    this->_stage = value.stage;
    this->_flags = value.flags;
}

Material::Material(
    const MaterialID id, const ResourceReference<Program>& program,
    const UniformSpecificationMap& uniforms)
    : program(program), _id(id)
{
    for (auto& kv_pair : uniforms) {
        const auto& [name, uniform] = kv_pair;
        KAACORE_CHECK(name.size(), "Invalid uniform name: %s", name);
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

Material::~Material()
{
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

ResourceReference<Material>
Material::create(
    const ResourceReference<Program>& program,
    const UniformSpecificationMap& uniforms)
{
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
                material->set_sampler_value(
                    name, this->get_sampler_value(name));
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

UniformSpecificationMap
Material::uniforms() const
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

void
Material::set_sampler_value(
    const std::string& name, const ResourceReference<Image>& texture,
    const uint8_t stage, const uint32_t flags)
{
    KAACORE_CHECK(
        this->_name_in_registry(name), "Unknown uniform name: {}.", name);
    std::get<Sampler>(this->_uniforms[name]).set(texture, stage, flags);
}

void
Material::set_sampler_value(const std::string& name, const SamplerValue& value)
{
    KAACORE_CHECK(
        this->_name_in_registry(name), "Unknown uniform name: {}.", name);
    std::get<Sampler>(this->_uniforms[name]).set(value);
}

SamplerValue
Material::get_sampler_value(const std::string& name) const
{
    KAACORE_CHECK(
        this->_name_in_registry(name), "Unknown uniform name: {}.", name);
    return std::get<Sampler>(this->_uniforms.at(name)).get();
}

void
Sampler::_bind()
{
    bgfx::setTexture(this->_stage, this->_handle, this->_value, this->_flags);
}

void
Material::_bind()
{
    for (auto& kv_pair : this->_uniforms) {
        std::visit([](auto&& variant) { variant._bind(); }, kv_pair.second);
    }
}

void
Material::_initialize()
{
    for (auto& kv_pair : this->_uniforms) {
        std::visit(
            [](auto&& variant) { variant._initialize(); }, kv_pair.second);
    }
    this->is_initialized = true;
}

void
Material::_uninitialize()
{
    for (auto& kv_pair : this->_uniforms) {
        std::visit(
            [](auto&& variant) { variant._uninitialize(); }, kv_pair.second);
    }
    this->is_initialized = false;
}

bool
Material::_name_in_registry(const std::string& name) const
{
    return this->_uniforms.find(name) != this->_uniforms.end();
}

}

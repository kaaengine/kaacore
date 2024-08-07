#include "kaacore/uniforms.h"

namespace kaacore {

UniformSpecification::UniformSpecification(
    const UniformType type, const uint16_t number_of_elements
)
    : _type(type), _number_of_elements(number_of_elements)
{
    if (type == UniformType::sampler) {
        KAACORE_CHECK(
            number_of_elements == 1,
            "Sampler uniform must not have multiple elements."
        );
    }
}

bool
UniformSpecification::operator==(const UniformSpecification& other)
{
    return this->_type == other._type and
           this->_number_of_elements == other._number_of_elements;
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
    const uint16_t number_of_elements
)
    : _name(name), _type(type), _number_of_elements(number_of_elements){};

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
        this->_name.c_str(), this->_internal_type(), this->_number_of_elements
    );
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
      _value(other._value)
{}

Sampler::Sampler(Sampler&& other)
    : UniformBase(std::move(other)), _stage(other._stage), _flags(other._flags),
      _value(std::move(other._value))
{
    other._stage = 0u;
    other._flags = 0u;
    if (auto ptr = std::get_if<bgfx::TextureHandle>(&other._value)) {
        *ptr = BGFX_INVALID_HANDLE;
    }
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
    this->_value = std::move(other._value);

    other._stage = 0u;
    other._flags = 0u;
    if (auto ptr = std::get_if<bgfx::TextureHandle>(&other._value)) {
        *ptr = BGFX_INVALID_HANDLE;
    }

    return *this;
}

std::optional<SamplerValue>
Sampler::get() const
{
    if (auto ptr = std::get_if<std::shared_ptr<Texture>>(&this->_value)) {
        return SamplerValue{this->_stage, this->_flags, *ptr};
    }

    return std::nullopt;
}

void
Sampler::set(
    const ResourceReference<Texture>& texture, const uint8_t stage,
    const uint32_t flags
)
{
    this->_value = texture.res_ptr;
    this->_stage = stage;
    this->_flags = flags;
}

void
Sampler::set(const SamplerValue& value)
{
    this->_value = value.texture.res_ptr;
    this->_stage = value.stage;
    this->_flags = value.flags;
}

bgfx::TextureHandle
Sampler::_texture_handle()
{
    return std::visit(
        [](auto&& variant) -> bgfx::TextureHandle {
            using T = std::decay_t<decltype(variant)>;
            if constexpr (std::is_same_v<T, bgfx::TextureHandle>) {
                return variant;
            } else if constexpr (std::is_same_v<T, std::shared_ptr<Texture>>) {
                return variant->handle();
            }
        },
        this->_value
    );
}

void
Sampler::_set(const Texture* texture, const uint8_t stage, const uint32_t flags)
{
    this->_value = texture->handle();
    this->_stage = stage;
    this->_flags = flags;
}

void
Sampler::_bind()
{
    bgfx::setTexture(
        this->_stage, this->_handle, this->_texture_handle(), this->_flags
    );
}

} // namespace kaacore

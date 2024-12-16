#pragma once

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "kaacore/render_targets.h"
#include "kaacore/textures.h"
#include "kaacore/utils.h"

namespace kaacore {

enum class UniformType {
    sampler = bgfx::UniformType::Sampler,
    vec4 = bgfx::UniformType::Vec4,
    mat3 = bgfx::UniformType::Mat3,
    mat4 = bgfx::UniformType::Mat4
};

class UniformSpecification {
  public:
    UniformSpecification() = default;
    UniformSpecification(
        const UniformType type, const uint16_t number_of_elements = 1
    );
    bool operator==(const UniformSpecification& other);
    UniformType type() const;
    uint16_t number_of_elements() const;

  private:
    UniformType _type;
    uint16_t _number_of_elements;
};

class UniformBase {
  public:
    UniformBase(
        const std::string& name, const UniformType type,
        const uint16_t number_of_elements = 1
    );
    UniformBase(const UniformBase& other);
    UniformBase(UniformBase&& other);
    UniformBase& operator=(UniformBase&& other);
    UniformBase& operator=(const UniformBase& other) = delete;

    UniformType type() const;
    std::string name() const;
    uint16_t number_of_elements() const;
    UniformSpecification specification() const;

  protected:
    UniformType _type;
    std::string _name;
    uint16_t _number_of_elements;
    bgfx::UniformHandle _handle = BGFX_INVALID_HANDLE;

    void _initialize();
    void _uninitialize();
    bgfx::UniformType::Enum _internal_type();
};

class ShadingContext;

struct SamplerValue {
    uint8_t stage;
    uint32_t flags;
    ResourceReference<Texture> texture;
};

class Sampler : public UniformBase {
    using _Value = std::variant<bgfx::TextureHandle, std::shared_ptr<Texture>>;

  public:
    Sampler();
    Sampler(const std::string& name);
    Sampler(const Sampler& other);
    Sampler(Sampler&& other);
    Sampler& operator=(Sampler&& other);
    Sampler& operator=(const Sampler& other) = delete;

    std::optional<SamplerValue> get() const;
    void set(
        const ResourceReference<Texture>& texture, const uint8_t stage,
        const uint32_t flags
    );
    void set(const SamplerValue& value);

  private:
    _Value _value;
    uint8_t _stage;
    uint32_t _flags;

    bgfx::TextureHandle _texture_handle();
    void _set(
        const Texture* texture, const uint8_t stage, const uint32_t flags
    );
    void _bind();

    friend class ShadingContext;
};

template<typename T>
class UniformValue {
  public:
    UniformValue(){};
    UniformValue(const T value) { this->_value.push_back(value); };
    UniformValue(const std::vector<T>& values) { this->_value = values; };
    UniformValue(const UniformValue& other) { this->_value = other._value; }
    UniformValue& operator=(const UniformValue& other)
    {
        if (this == &other) {
            return *this;
        }
        this->_value = other._value;
        return *this;
    }
    UniformValue(UniformValue&& other)
    {
        this->_value = std::move(other._value);
    }
    UniformValue& operator=(UniformValue&& other)
    {
        if (this == &other) {
            return *this;
        }
        this->_value = std::move(other._value);
        return *this;
    }
    std::size_t number_of_elements() { return this->_value.size(); }
    float* raw_data()
    {
        return static_cast<float*>(glm::value_ptr(this->_value.front()));
    };
    std::vector<T> data() const { return this->_value; }

  private:
    std::vector<T> _value;
};

template<typename T>
class FloatUniform : public UniformBase {
  public:
    FloatUniform() : UniformBase("", this->_get_type()){};
    FloatUniform(const std::string& name, const uint16_t number_of_elements = 1)
        : UniformBase(name, this->_get_type(), number_of_elements)
    {}
    FloatUniform(const FloatUniform& other)
        : UniformBase(other), _value(other._value)
    {}
    FloatUniform(FloatUniform&& other)
        : UniformBase(other), _value(std::move(other._value))
    {}
    FloatUniform& operator=(const FloatUniform& other) = delete;
    FloatUniform& operator=(FloatUniform&& other)
    {
        if (this == &other) {
            return *this;
        }

        UniformBase::operator=(std::move(other));
        this->_value = std::move(other._value);
        return *this;
    }
    std::vector<T> get() const { return this->_value.data(); }
    void set(UniformValue<T>&& value)
    {
        KAACORE_CHECK(
            value.number_of_elements() == this->_number_of_elements,
            "Invald number of elements for uniform value, expected {}, got {}.",
            this->_number_of_elements, value.number_of_elements()
        );
        this->_value = std::move(value);
    }

  private:
    UniformValue<T> _value;

    constexpr UniformType _get_type()
    {
        if constexpr (std::is_same_v<T, glm::fvec4>) {
            return UniformType::vec4;
        } else if constexpr (std::is_same_v<T, glm::fmat3>) {
            return UniformType::mat3;
        } else if constexpr (std::is_same_v<T, glm::fmat4>) {
            return UniformType::mat4;
        } else {
            static_assert(always_false_v<T>, "Unsupported type for uniform.");
        }
    }
    void _bind()
    {
        bgfx::setUniform(
            this->_handle, this->_value.raw_data(),
            this->_value.number_of_elements()
        );
    }

    friend class ShadingContext;
};

using Vec4Uniform = FloatUniform<glm::fvec4>;
using Mat3Uniform = FloatUniform<glm::fmat3>;
using Mat4Uniform = FloatUniform<glm::fmat4>;
using UniformSpecificationMap =
    std::unordered_map<std::string, UniformSpecification>;
using UniformVariant =
    std::variant<Sampler, Vec4Uniform, Mat3Uniform, Mat4Uniform>;

} // namespace kaacore

#pragma once

#include <cstdint>

#include <bgfx/defines.h>

#include "kaacore/utils.h"


namespace kaacore {

enum class StencilTest : uint8_t {
    disabled = 0u,
    less = BGFX_STENCIL_TEST_LESS >> BGFX_STENCIL_TEST_SHIFT,
    less_equal = BGFX_STENCIL_TEST_LEQUAL >> BGFX_STENCIL_TEST_SHIFT,
    equal = BGFX_STENCIL_TEST_EQUAL >> BGFX_STENCIL_TEST_SHIFT,
    greater_equal = BGFX_STENCIL_TEST_GEQUAL >> BGFX_STENCIL_TEST_SHIFT,
    greater = BGFX_STENCIL_TEST_GREATER >> BGFX_STENCIL_TEST_SHIFT,
    not_equal = BGFX_STENCIL_TEST_NOTEQUAL >> BGFX_STENCIL_TEST_SHIFT,
    never = BGFX_STENCIL_TEST_NEVER >> BGFX_STENCIL_TEST_SHIFT,
    always = BGFX_STENCIL_TEST_ALWAYS >> BGFX_STENCIL_TEST_SHIFT,
};

enum class StencilOp : uint8_t {
    zero = BGFX_STENCIL_OP_FAIL_S_ZERO >> BGFX_STENCIL_OP_FAIL_S_SHIFT,
    keep = BGFX_STENCIL_OP_FAIL_S_KEEP >> BGFX_STENCIL_OP_FAIL_S_SHIFT,
    replace = BGFX_STENCIL_OP_FAIL_S_REPLACE >> BGFX_STENCIL_OP_FAIL_S_SHIFT,
    increase_wrap = BGFX_STENCIL_OP_FAIL_S_INCR >> BGFX_STENCIL_OP_FAIL_S_SHIFT,
    increase_clamp = BGFX_STENCIL_OP_FAIL_S_INCRSAT >> BGFX_STENCIL_OP_FAIL_S_SHIFT,
    decrease_wrap = BGFX_STENCIL_OP_FAIL_S_DECR >> BGFX_STENCIL_OP_FAIL_S_SHIFT,
    decrease_clamp = BGFX_STENCIL_OP_FAIL_S_DECRSAT >> BGFX_STENCIL_OP_FAIL_S_SHIFT,
    invert = BGFX_STENCIL_OP_FAIL_S_INVERT >> BGFX_STENCIL_OP_FAIL_S_SHIFT,
};


class StencilMode {
  public:
    typedef uint32_t Flags;

    StencilMode();
    StencilMode(const uint8_t value, const uint8_t mask, const StencilTest test,
        const StencilOp stencil_fail_op = StencilOp::keep,
        const StencilOp depth_fail_op = StencilOp::keep,
        const StencilOp pass_op = StencilOp::keep);
    static StencilMode make_disabled();

    inline bool is_disabled() const { return this->test() == StencilTest::disabled; }

    Flags stencil_flags() const;
    bool operator==(const StencilMode& other) const;

    void value(const uint8_t new_value);
    uint8_t value() const;

    void mask(const uint8_t new_value);
    uint8_t mask() const;

    void test(const StencilTest new_value);
    StencilTest test() const;

    void stencil_fail_op(const StencilOp new_value);
    StencilOp stencil_fail_op() const;

    void depth_fail_op(const StencilOp new_value);
    StencilOp depth_fail_op() const;

    void pass_op(const StencilOp new_value);
    StencilOp pass_op() const;

  private:
    Flags _stencil_flags;
    // uint32_t _children_modifiers;

  friend class std::hash<StencilMode>;
};

} // namespace kaacore

namespace std {
template<>
struct hash<kaacore::StencilMode> {
    size_t operator()(const kaacore::StencilMode& stencil_mode) const
    {
        return std::hash<uint32_t>{}(stencil_mode._stencil_flags);
    }
};
}

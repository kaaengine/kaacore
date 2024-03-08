#include "kaacore/stencil.h"

namespace kaacore {

inline uint8_t bitwise_retrieve(const uint32_t bitmask, const uint8_t shift, const uint32_t container)
{
    return (container & bitmask) >> shift;
}

inline uint32_t bitwise_store(const uint32_t bitmask, const uint8_t shift, const uint32_t container, uint8_t value)
{
    uint32_t new_container = container & ~bitmask;
    new_container |= (static_cast<uint32_t>(value) << shift) & bitmask;
    return new_container;
}

StencilMode::StencilMode()
    : StencilMode(0, 0xFF, StencilTest::always)
{
}

StencilMode
StencilMode::make_disabled()
{
    return StencilMode{0, 0, StencilTest::disabled};
}

StencilMode::StencilMode(const uint8_t value, const uint8_t mask, const StencilTest test,
        const StencilOp stencil_fail_op, const StencilOp depth_fail_op,
        const StencilOp pass_op)
    : _stencil_flags(0u)
{
    this->value(value);
    this->mask(mask);
    this->test(test);
    this->stencil_fail_op(stencil_fail_op);
    this->depth_fail_op(depth_fail_op);
    this->pass_op(pass_op);
}

StencilMode::Flags
StencilMode::stencil_flags() const
{
    if (this->test() == StencilTest::disabled) {
        return BGFX_STENCIL_DEFAULT;
    }

    return this->_stencil_flags;
}

bool
StencilMode::operator==(const StencilMode& other) const
{
    return this->_stencil_flags == other._stencil_flags;
}

void
StencilMode::value(const uint8_t new_value)
{
    this->_stencil_flags = bitwise_store(
        BGFX_STENCIL_FUNC_REF_MASK, BGFX_STENCIL_FUNC_REF_SHIFT,
        this->_stencil_flags, new_value
    );
}

uint8_t
StencilMode::value() const
{
    return bitwise_retrieve(
        BGFX_STENCIL_FUNC_REF_MASK, BGFX_STENCIL_FUNC_REF_SHIFT,
        this->_stencil_flags
    );
}

void
StencilMode::mask(const uint8_t new_value)
{
    this->_stencil_flags = bitwise_store(
        BGFX_STENCIL_FUNC_RMASK_MASK, BGFX_STENCIL_FUNC_RMASK_SHIFT,
        this->_stencil_flags, new_value
    );
}

uint8_t
StencilMode::mask() const
{
    return bitwise_retrieve(
        BGFX_STENCIL_FUNC_RMASK_MASK, BGFX_STENCIL_FUNC_RMASK_SHIFT,
        this->_stencil_flags
    );
}

void
StencilMode::test(const StencilTest new_value)
{
    this->_stencil_flags = bitwise_store(
        BGFX_STENCIL_TEST_MASK, BGFX_STENCIL_TEST_SHIFT,
        this->_stencil_flags, static_cast<uint8_t>(new_value)
    );
}

StencilTest
StencilMode::test() const
{
    return static_cast<StencilTest>(bitwise_retrieve(
        BGFX_STENCIL_TEST_MASK, BGFX_STENCIL_TEST_SHIFT,
        this->_stencil_flags
    ));
}

void
StencilMode::stencil_fail_op(const StencilOp new_value)
{
    this->_stencil_flags = bitwise_store(
        BGFX_STENCIL_OP_FAIL_S_MASK, BGFX_STENCIL_OP_FAIL_S_SHIFT,
        this->_stencil_flags, static_cast<uint8_t>(new_value)
    );
}

StencilOp
StencilMode::stencil_fail_op() const
{
    return static_cast<StencilOp>(bitwise_retrieve(
        BGFX_STENCIL_OP_FAIL_S_MASK, BGFX_STENCIL_OP_FAIL_S_SHIFT,
        this->_stencil_flags
    ));
}

void
StencilMode::depth_fail_op(const StencilOp new_value)
{
    this->_stencil_flags = bitwise_store(
        BGFX_STENCIL_OP_FAIL_Z_MASK, BGFX_STENCIL_OP_FAIL_Z_SHIFT,
        this->_stencil_flags, static_cast<uint8_t>(new_value)
    );
}

StencilOp
StencilMode::depth_fail_op() const
{
    return static_cast<StencilOp>(bitwise_retrieve(
        BGFX_STENCIL_OP_FAIL_Z_MASK, BGFX_STENCIL_OP_FAIL_Z_SHIFT,
        this->_stencil_flags
    ));
}

void
StencilMode::pass_op(const StencilOp new_value)
{
    this->_stencil_flags = bitwise_store(
        BGFX_STENCIL_OP_PASS_Z_MASK, BGFX_STENCIL_OP_PASS_Z_SHIFT,
        this->_stencil_flags, static_cast<uint8_t>(new_value)
    );
}

StencilOp
StencilMode::pass_op() const
{
    return static_cast<StencilOp>(bitwise_retrieve(
        BGFX_STENCIL_OP_PASS_Z_MASK, BGFX_STENCIL_OP_PASS_Z_SHIFT,
        this->_stencil_flags
    ));
}

} // namespace kaacore

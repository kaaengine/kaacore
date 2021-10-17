#include "kaacore/capture.h"

#pragma once

namespace kaacore {

struct RendererCallbacks : bgfx::CallbackI {
    void captureBegin(
        uint32_t width, uint32_t height, uint32_t pitch,
        bgfx::TextureFormat::Enum format, bool yflip) override;
    void captureEnd() override;
    void captureFrame(const void* data, uint32_t size) override;

    // Unused callbacks
    inline void fatal(
        const char*, uint16_t, bgfx::Fatal::Enum, const char*) override
    {}

    inline void traceVargs(
        const char*, uint16_t, const char*, va_list _argList) override
    {}

    inline void profilerBegin(
        const char*, uint32_t, const char*, uint16_t) override
    {}

    inline void profilerBeginLiteral(
        const char*, uint32_t, const char*, uint16_t) override
    {}

    inline void profilerEnd() override {}

    inline uint32_t cacheReadSize(uint64_t) override { return 0u; }

    inline bool cacheRead(uint64_t, void*, uint32_t) override { return false; }

    inline void cacheWrite(uint64_t, const void*, uint32_t) override {}

    inline void screenShot(
        const char*, uint32_t, uint32_t, uint32_t, const void*, uint32_t,
        bool) override
    {}

    CapturingAdapter* capturing_adapter = nullptr;
};

} // namespace kaacore

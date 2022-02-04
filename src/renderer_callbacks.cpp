#include <cstdint>

#include "kaacore/exceptions.h"
#include "kaacore/log.h"

#include "kaacore/renderer_callbacks.h"

namespace kaacore {

void
RendererCallbacks::captureBegin(
    uint32_t width, uint32_t height, uint32_t pitch,
    bgfx::TextureFormat::Enum format, bool yflip)
{
    KAACORE_LOG_TRACE("RendererCallbacks::captureBegin called");
    if (this->capturing_adapter) {
        this->capturing_adapter->initialize_capture_parameters(
            width, height, pitch, format, yflip);
    }
}

void
RendererCallbacks::captureEnd()
{
    KAACORE_LOG_TRACE("RendererCallbacks::captureEnd called");
}

void
RendererCallbacks::captureFrame(const void* data, uint32_t size)
{
    KAACORE_LOG_TRACE(
        "RendererCallbacks::captureFrame called, data size: {}", size);
    if (this->capturing_adapter) {
        this->capturing_adapter->process_raw_frame(data, size);
    }
}

} // namespace kaacore

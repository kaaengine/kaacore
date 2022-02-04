#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <bgfx/bgfx.h>
#include <bimg/bimg.h>

namespace kaacore {

struct CapturedFrameData {
    CapturedFrameData(std::byte* raw_ptr, const size_t size);
    inline std::byte* get() const { return this->ptr.get(); }

    std::shared_ptr<std::byte> ptr;
    size_t size;
};

struct CapturedFrames {
    CapturedFrames() {}
    CapturedFrames(
        const uint32_t width, const uint32_t height,
        const std::vector<CapturedFrameData>& frames)
        : width(width), height(height), frames(frames)
    {}

    std::vector<uint8_t*> raw_ptr_frames_uint8();

    uint32_t width;
    uint32_t height;
    std::vector<CapturedFrameData> frames;
};

class CapturingAdapter {
    friend class CaptureCallback;

  public:
    void on_frame(const std::byte* frame_data, uint32_t size);

    uint32_t width() const;
    uint32_t height() const;
    bimg::TextureFormat::Enum texture_format() const;
    bool y_flip() const;

    void process_raw_frame(const void* data, uint32_t size);
    void initialize_capture_parameters(
        uint32_t width, uint32_t height, uint32_t pitch,
        bgfx::TextureFormat::Enum format, bool y_flip);

    size_t frames_count() const;
    CapturedFrames get_captured_frames() const;

  private:
    size_t frame_line_bytes_count() const;
    void flip_aware_frame_copy(
        std::byte* dst, const std::byte* src, const uint32_t size) const;

    bool _is_initialized = false;
    std::unique_ptr<std::byte[]> _frame_data_buffer;
    size_t _frame_data_size;
    uint32_t _width;
    uint32_t _height;
    uint32_t _source_pitch;
    bimg::TextureFormat::Enum _source_format;
    bimg::TextureFormat::Enum _target_format = bimg::TextureFormat::Enum::RGBA8;
    bool _y_flip;
    std::vector<CapturedFrameData> _frames;
};

} // namespace kaacore

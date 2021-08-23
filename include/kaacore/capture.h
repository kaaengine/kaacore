#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <bgfx/bgfx.h>
#include <bimg/bimg.h>

namespace kaacore {

class CapturingAdapterBase {
    friend class CaptureCallback;

  public:
    CapturingAdapterBase() = default;
    virtual ~CapturingAdapterBase() {}

    virtual void on_frame(const std::byte* frame_data, uint32_t size) = 0;

    uint32_t width() const;
    uint32_t height() const;
    bimg::TextureFormat::Enum texture_format() const;
    bool y_flip() const;

    void process_raw_frame(const void* data, uint32_t size);
    void initialize_capture_parameters(
        uint32_t width, uint32_t height, uint32_t pitch,
        bgfx::TextureFormat::Enum format, bool y_flip);

  protected:
    size_t frame_line_bytes_count() const;
    void flip_aware_frame_copy(
        std::byte* dst, const std::byte* src, const uint32_t size) const;

  private:
    bool _is_initialized = false;
    std::unique_ptr<std::byte[]> _frame_data_buffer;
    size_t _frame_data_size;
    uint32_t _width;
    uint32_t _height;
    uint32_t _source_pitch;
    bimg::TextureFormat::Enum _source_format;
    bimg::TextureFormat::Enum _target_format = bimg::TextureFormat::Enum::RGBA8;
    bool _y_flip;
};

class ImagesDirectoryCapturingAdapter : public CapturingAdapterBase {
  public:
    ImagesDirectoryCapturingAdapter(const std::string name_prefix);

    void on_frame(const std::byte* frame_data, uint32_t size) override;

  private:
    std::string _name_format;
    size_t _frames_count;
};

class MemoryVectorCapturingAdapter : public CapturingAdapterBase {
  public:
    MemoryVectorCapturingAdapter();

    void on_frame(const std::byte* frame_data, uint32_t size) override;

    size_t frames_count() const;
    const std::vector<uint8_t*> frames_uint8() const;

  private:
    std::vector<std::unique_ptr<std::byte[]>> _frames;
};

} // namespace kaacore

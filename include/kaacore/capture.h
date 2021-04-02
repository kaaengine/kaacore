#pragma once

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

  private:
    void process_raw_frame(const void* data, uint32_t size);
    void initialize_capture_parameters(
        uint32_t width, uint32_t height, uint32_t pitch,
        bgfx::TextureFormat::Enum format, bool y_flip);

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

class CaptureCallback : public bgfx::CallbackI {
  public:
    void captureBegin(
        uint32_t _width, uint32_t _height, uint32_t _pitch,
        bgfx::TextureFormat::Enum _format, bool _yflip) override;
    void captureEnd() override;
    void captureFrame(const void* _data, uint32_t _size) override;

    void setup_capturing_adapter(CapturingAdapterBase* capturing_adapter);
    void clear_capturing_adapter();

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

  private:
    CapturingAdapterBase* _capturing_adapter = nullptr;
};

} // namespace kaacore

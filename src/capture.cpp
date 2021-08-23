#include "kaacore/exceptions.h"
#include "kaacore/log.h"

#include <bx/bx.h>
#include <bx/file.h>
#include <spdlog/fmt/fmt.h>

#include "kaacore/capture.h"

namespace kaacore {

uint32_t
CapturingAdapterBase::width() const
{
    KAACORE_ASSERT(this->_is_initialized, "Adapter was not initialized");
    return this->_width;
}

uint32_t
CapturingAdapterBase::height() const
{
    KAACORE_ASSERT(this->_is_initialized, "Adapter was not initialized");
    return this->_height;
}

bimg::TextureFormat::Enum
CapturingAdapterBase::texture_format() const
{
    KAACORE_ASSERT(this->_is_initialized, "Adapter was not initialized");
    return this->_target_format;
}

bool
CapturingAdapterBase::y_flip() const
{
    KAACORE_ASSERT(this->_is_initialized, "Adapter was not initialized");
    return this->_y_flip;
}

void
CapturingAdapterBase::process_raw_frame(const void* data, uint32_t size)
{
    thread_local bx::DefaultAllocator allocator;
    KAACORE_ASSERT(this->_is_initialized, "Adapter was not initialized");
    KAACORE_ASSERT(
        bimg::imageConvert(this->_source_format, this->_target_format),
        "Conversion between provided formats is not supported");

    bimg::imageConvert(
        &allocator, this->_frame_data_buffer.get(), this->_target_format,
        data, // source
        this->_source_format, this->_width, this->_height,
        1u // depth
    );

    this->on_frame(this->_frame_data_buffer.get(), this->_frame_data_size);
}

void
CapturingAdapterBase::initialize_capture_parameters(
    uint32_t width, uint32_t height, uint32_t pitch,
    bgfx::TextureFormat::Enum source_format, bool y_flip)
{
    KAACORE_ASSERT(
        not this->_is_initialized, "Adapter was already initialized");
    this->_is_initialized = true;
    this->_width = width;
    this->_height = height;
    this->_source_pitch = pitch;
    this->_source_format = bimg::TextureFormat::Enum(source_format);
    this->_y_flip = y_flip;
    this->_frame_data_size = height * this->frame_line_bytes_count();
    this->_frame_data_buffer.reset(new std::byte[this->_frame_data_size]);

    KAACORE_LOG_DEBUG(
        "Frame image parameters - width: {}, height: {}, pitch: {}, y_flip: {}",
        width, height, pitch, y_flip);
    KAACORE_LOG_DEBUG(
        "Frame image format: {} -> {}", bimg::getName(this->_source_format),
        bimg::getName(this->_target_format));
}

size_t
CapturingAdapterBase::frame_line_bytes_count() const
{
    KAACORE_CHECK(this->_is_initialized, "Adapter was not initialized yet.");
    return this->_width * (bimg::getBitsPerPixel(this->_target_format) / 8);
}

void
CapturingAdapterBase::flip_aware_frame_copy(
    std::byte* dst, const std::byte* src, const uint32_t size) const
{
    if (this->y_flip()) {
        uint32_t height = this->height();
        uint32_t line_bytes = this->frame_line_bytes_count();
        for (uint32_t i = 0; i < height; i++) {
            KAACORE_ASSERT(
                src + line_bytes * i + line_bytes <= src + size,
                "Out of bounds read for i = {}", i);
            KAACORE_ASSERT(
                dst + line_bytes * (height - i - 1) + line_bytes <= dst + size,
                "Out of bounds write for i = {}", i);
            std::memcpy(
                dst + line_bytes * (height - i - 1), src + line_bytes * i,
                line_bytes);
        }
    } else {
        std::memcpy(dst, src, size);
    }
}

ImagesDirectoryCapturingAdapter::ImagesDirectoryCapturingAdapter(
    const std::string name_prefix)
    : _name_format(name_prefix + "_{:06d}.png"), _frames_count(0),
      CapturingAdapterBase()
{}

void
ImagesDirectoryCapturingAdapter::on_frame(
    const std::byte* frame_data, uint32_t size)
{
    std::string file_path =
        fmt::format(this->_name_format, this->_frames_count);
    KAACORE_LOG_DEBUG(
        "Writing captured frame (size: {}) to file: {}", size, file_path);

    bx::FileWriter writer;
    bx::Error err;
    if (bx::open(&writer, file_path.c_str(), false, &err)) {
        uint32_t pitch = this->width() * 4;
        bimg::imageWritePng(
            &writer, this->width(), this->height(), pitch, frame_data,
            this->texture_format(), false, &err);
        bx::close(&writer);
    }
    this->_frames_count++;

    KAACORE_CHECK(
        err.isOk(), "BX error occured: {}", err.getMessage().getPtr());
}

MemoryVectorCapturingAdapter::MemoryVectorCapturingAdapter()
    : CapturingAdapterBase()
{}

void
MemoryVectorCapturingAdapter::on_frame(
    const std::byte* frame_data, uint32_t size)
{
    KAACORE_LOG_DEBUG("Storing captured frame #{}", this->frames_count() + 1);
    std::unique_ptr<std::byte[]> frame_stored{new std::byte[size]};
    this->flip_aware_frame_copy(frame_stored.get(), frame_data, size);
    this->_frames.push_back(std::move(frame_stored));
}

size_t
MemoryVectorCapturingAdapter::frames_count() const
{
    return this->_frames.size();
}

const std::vector<uint8_t*>
MemoryVectorCapturingAdapter::frames_uint8() const
{
    std::vector<uint8_t*> frames_vector;
    for (const auto& uptr : this->_frames) {
        frames_vector.push_back(reinterpret_cast<uint8_t*>(uptr.get()));
    }
    return frames_vector;
}

} // namespace kaacore

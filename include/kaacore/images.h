#pragma once

#include <memory>
#include <string>
#include <vector>

#include <bgfx/bgfx.h>
#include <bimg/decode.h>
#include <bx/bx.h>
#include <bx/file.h>
#include <glm/glm.hpp>

#include "kaacore/resources.h"

namespace kaacore {

void
initialize_images();
void
uninitialize_images();

bimg::ImageContainer*
load_image(const uint8_t* data, size_t size);
bimg::ImageContainer*
load_image(const char* path);
bimg::ImageContainer*
load_raw_image(
    bimg::TextureFormat::Enum format, uint16_t width, uint16_t height,
    const std::vector<uint8_t>& data);

class FontData;

class Image : public Resource {
  public:
    const std::string path;
    const uint64_t flags = BGFX_SAMPLER_NONE;
    bgfx::TextureHandle texture_handle;
    std::shared_ptr<bimg::ImageContainer> image_container;

    Image();
    ~Image();
    glm::uvec2 get_dimensions();

    static ResourceReference<Image> load(
        const std::string& path, uint64_t flags = BGFX_SAMPLER_NONE);
    static ResourceReference<Image> load(bimg::ImageContainer* image_container);

  private:
    Image(bimg::ImageContainer* image_container);
    Image(const std::string& path, uint64_t flags = BGFX_SAMPLER_NONE);
    virtual void _initialize() override;
    virtual void _uninitialize() override;

    friend class ResourcesRegistry<std::string, Image>;
    friend class FontData;
    friend std::unique_ptr<Image> load_default_image();
    friend class FontData;
};

template<typename T = uint8_t>
struct BitmapView {
    BitmapView() : content(nullptr), dimensions({0, 0}) {}

    BitmapView(T* content, const glm::uvec2 dimensions)
        : content(content), dimensions(dimensions)
    {
        KAACORE_ASSERT(
            content != nullptr,
            "Can't create BitmapView with NULL content pointer");
    }

    T& at(const size_t x, const size_t y)
    {
        KAACORE_ASSERT(
            x < this->dimensions.x,
            "Requested x={} exceeds X dimensions size: {}", x,
            this->dimensions.x);
        KAACORE_ASSERT(
            y < this->dimensions.y,
            "Requested y={} exceeds Y dimensions size: {}", y,
            this->dimensions.y);
        return *(this->content + (y * this->dimensions.x) + x);
    }

    void blit(BitmapView<T> source, const glm::uvec2 target_coords)
    {
        KAACORE_ASSERT(
            source.dimensions.x + target_coords.x <= this->dimensions.x,
            "Blitting size ({}) would overflow X dimension ({})",
            source.dimensions.x + target_coords.x, this->dimensions.x);
        KAACORE_ASSERT(
            source.dimensions.y + target_coords.y <= this->dimensions.y,
            "Blitting size ({}) would overflow Y dimension ({})",
            source.dimensions.y + target_coords.y, this->dimensions.y);

        for (size_t x = 0; x < source.dimensions.x; x++) {
            for (size_t y = 0; y < source.dimensions.y; y++) {
                this->at(target_coords.x + x, target_coords.y + y) =
                    source.at(x, y);
            }
        }
    }

    T* content;
    glm::uvec2 dimensions;
};

template<typename T = uint8_t>
struct Bitmap {
    Bitmap(const glm::uvec2 dimensions) : dimensions(dimensions)
    {
        this->container.resize(dimensions.x * dimensions.y);
    }

    BitmapView<T> view() { return {this->container.data(), this->dimensions}; }

    operator BitmapView<T>() { return this->view(); }

    T& at(const size_t x, const size_t y) { return this->view().at(x, y); }

    void blit(BitmapView<T> source, const glm::uvec2 target_coords)
    {
        this->view().blit(source, target_coords);
    }

    std::vector<T> container;
    glm::uvec2 dimensions;
};

} // namespace kaacore

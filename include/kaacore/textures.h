#pragma once

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <bgfx/bgfx.h>
#include <bimg/bimg.h>
#include <glm/glm.hpp>

#include "kaacore/log.h"
#include "kaacore/resources.h"

namespace kaacore {

void
initialize_textures();
void
uninitialize_textures();

bimg::ImageContainer*
load_image(const uint8_t* data, size_t size);
bimg::ImageContainer*
load_image(const std::string& path);
bimg::ImageContainer*
load_raw_image(
    bimg::TextureFormat::Enum format, uint16_t width, uint16_t height,
    const std::vector<uint8_t>& data
);

glm::dvec4
query_image_pixel(const bimg::ImageContainer* image, const glm::uvec2 position);

class FontData;

class Texture : public Resource {
  public:
    bgfx::TextureHandle handle() const;
    virtual glm::uvec2 get_dimensions() const = 0;
    virtual bool can_query() const;
    virtual glm::dvec4 query_pixel(const glm::uvec2 position) const;

  protected:
    bgfx::TextureHandle _handle;

    friend class FontData;
};

class MemoryTexture : public Texture {
  public:
    MemoryTexture() = default;
    ~MemoryTexture();
    std::shared_ptr<bimg::ImageContainer> image_container;

    bool can_query() const override;
    glm::dvec4 query_pixel(const glm::uvec2 position) const override;
    glm::uvec2 get_dimensions() const override;
    static ResourceReference<MemoryTexture> create(
        bimg::ImageContainer* image_container
    );

  protected:
    MemoryTexture(bimg::ImageContainer* image_container);
    virtual void _initialize() override;
    virtual void _uninitialize() override;

    friend std::unique_ptr<MemoryTexture> load_default_texture();
};

class ImageTexture : public MemoryTexture {
  public:
    const std::string path;

    static ResourceReference<ImageTexture> load(const std::string& path);

  private:
    ImageTexture(const std::string& path);
    virtual void _initialize() override;

    friend class ResourcesRegistry<std::string, ImageTexture>;
};

template<typename T = uint8_t>
struct BitmapView {
    BitmapView() : content(nullptr), dimensions({0, 0}) {}

    BitmapView(T* content, const glm::uvec2 dimensions)
        : content(content), dimensions(dimensions)
    {
        KAACORE_ASSERT(
            content != nullptr,
            "Can't create BitmapView with NULL content pointer"
        );
    }

    T& at(const size_t x, const size_t y)
    {
        KAACORE_ASSERT(
            x < this->dimensions.x,
            "Requested x={} exceeds X dimensions size: {}", x,
            this->dimensions.x
        );
        KAACORE_ASSERT(
            y < this->dimensions.y,
            "Requested y={} exceeds Y dimensions size: {}", y,
            this->dimensions.y
        );
        return *(this->content + (y * this->dimensions.x) + x);
    }

    void blit(BitmapView<T> source, const glm::uvec2 target_coords)
    {
        KAACORE_ASSERT(
            source.dimensions.x + target_coords.x <= this->dimensions.x,
            "Blitting size ({}) would overflow X dimension ({})",
            source.dimensions.x + target_coords.x, this->dimensions.x
        );
        KAACORE_ASSERT(
            source.dimensions.y + target_coords.y <= this->dimensions.y,
            "Blitting size ({}) would overflow Y dimension ({})",
            source.dimensions.y + target_coords.y, this->dimensions.y
        );

        for (size_t row = 0; row < source.dimensions.y; row++) {
            std::memcpy(
                this->content + (this->dimensions.x * (row + target_coords.y)) +
                    target_coords.x,
                source.content + (source.dimensions.x * row),
                sizeof(T) * source.dimensions.x
            );
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

#pragma once

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
initialize_image_resources();
void
uninitialize_image_resources();

bimg::ImageContainer*
load_image(const uint8_t* data, size_t size);
bimg::ImageContainer*
load_image(const char* path);
bimg::ImageContainer*
load_raw_image(
    bimg::TextureFormat::Enum format, uint16_t width, uint16_t height,
    const std::vector<uint8_t>& data);

class Image : public Resource {
  public:
    const std::string path;
    const uint64_t flags = BGFX_SAMPLER_NONE;
    bgfx::TextureHandle texture_handle;
    bimg::ImageContainer* image_container;

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
    bgfx::TextureHandle _make_texture();

    friend class ResourcesRegistry<std::string, Image>;
    friend std::unique_ptr<Image> load_default_image();
};

} // namespace kaacore

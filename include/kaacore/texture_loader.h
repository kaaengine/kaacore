#pragma once

#include <vector>

#include <bx/bx.h>
#include <bx/file.h>
#include <bimg/decode.h>
#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "kaacore/resources.h"


namespace kaacore {

bimg::ImageContainer* load_image(const uint8_t* data, size_t size);
bimg::ImageContainer* load_image(const char* path);
bimg::ImageContainer* load_raw_image(bimg::TextureFormat::Enum format,
                                     uint16_t width, uint16_t height,
                                     const std::vector<uint8_t>& data);

bgfx::TextureHandle make_texture(const bimg::ImageContainer* const image_container,
                                 const uint64_t flags=BGFX_SAMPLER_NONE);


struct Image {
    bgfx::TextureHandle texture_handle;
    bimg::ImageContainer* image_container;

    Image(const char* path, uint64_t flags=BGFX_SAMPLER_NONE);
    Image(bgfx::TextureHandle texture_handle,
          bimg::ImageContainer* image_container);
    ~Image();

    glm::uvec2 get_dimensions();

    // TODO hashmap with existing resources
    static Resource<Image> load(const char* path, uint64_t flags=BGFX_SAMPLER_NONE);
    static Resource<Image> load(bgfx::TextureHandle texture_handle, bimg::ImageContainer* image_container);
};

} //namespace kaacore

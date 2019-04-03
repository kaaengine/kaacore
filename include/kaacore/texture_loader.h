#pragma once

#include <utility>

#include <bx/bx.h>
#include <bx/file.h>
#include <bimg/decode.h>
#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "kaacore/resources.h"


namespace kaacore {

std::pair<bgfx::TextureHandle, bimg::ImageContainer*> load_texture(
    const uint8_t* data, size_t size, uint64_t flags=BGFX_SAMPLER_NONE
);
std::pair<bgfx::TextureHandle, bimg::ImageContainer*> load_texture_from_file(
    const char* path, uint64_t flags=BGFX_SAMPLER_NONE
);


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
};

} // namespace kaacore

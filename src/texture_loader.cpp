#include <memory>

#include <bx/bx.h>
#include <bx/file.h>
#include <bimg/decode.h>
#include <bgfx/bgfx.h>

#include "kaacore/log.h"
#include "kaacore/files.h"
#include "kaacore/exceptions.h"

#include "kaacore/texture_loader.h"


namespace kaacore {

static bx::DefaultAllocator texture_image_allocator;


bimg::ImageContainer* load_image(const uint8_t* data, size_t size)
{
    bimg::ImageContainer* image_container = \
        bimg::imageParse(&texture_image_allocator, data, size);
    assert(image_container != NULL);

    log("Image details - width: %u, height: %u, depth: %u, layers: %u, format: %u",
        image_container->m_width, image_container->m_height,
        image_container->m_depth, image_container->m_numLayers,
        image_container->m_format
    );

    return image_container;
}


bimg::ImageContainer* load_image(const char* path)
{
    log("Loading image from file: %s", path);
    RawFile file(path);
    log("Loaded file size: %d", file.content.size());
    return load_image(
        file.content.data(), file.content.size()
    );
}

bimg::ImageContainer* load_raw_image(bimg::TextureFormat::Enum format,
                                     uint16_t width, uint16_t height,
                                     const std::vector<uint8_t>& data)
{
    bimg::ImageContainer* image_container = \
        bimg::imageAlloc(&texture_image_allocator, format, width, height,
                         1, 1, false, false, data.data());

    assert(image_container != NULL);
    return image_container;
}


bgfx::TextureHandle make_texture(const bimg::ImageContainer* const image_container,
                                 const uint64_t flags)
{
    assert(bgfx::isTextureValid(0, false, image_container->m_numLayers,
           bgfx::TextureFormat::Enum(image_container->m_format), flags));

    const bgfx::Memory* mem = bgfx::makeRef(
        image_container->m_data, image_container->m_size
    );

    return bgfx::createTexture2D(
        uint16_t(image_container->m_width),
        uint16_t(image_container->m_height),
        1 < image_container->m_numMips,
        image_container->m_numLayers,
        bgfx::TextureFormat::Enum(image_container->m_format),
        flags,
        mem
    );
}


Image::Image(const char* path, uint64_t flags)
{
    this->image_container = load_image(path);
    this->texture_handle = make_texture(this->image_container, flags);
    bgfx::setName(this->texture_handle, path);
}

Image::Image(bgfx::TextureHandle texture_handle,
             bimg::ImageContainer* image_container)
{
    this->texture_handle = texture_handle;
    this->image_container = image_container;
}

Resource<Image> Image::load(const char* path, uint64_t flags)
{
    return std::make_shared<Image>(path, flags);
}

Image::~Image()
{
    if (bgfx::isValid(this->texture_handle)) {
        bgfx::destroy(this->texture_handle);
    }
    if (this->image_container != nullptr) {
        bimg::imageFree(this->image_container);
    }
}

glm::uvec2 Image::get_dimensions()
{
    KAACORE_CHECK(this->image_container != nullptr);
    return {this->image_container->m_width, this->image_container->m_height};
}

} // namespace kaacore

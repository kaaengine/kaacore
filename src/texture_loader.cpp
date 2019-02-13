#include <memory>

#include <bx/bx.h>
#include <bx/file.h>
#include <bimg/decode.h>
#include <bgfx/bgfx.h>

#include "kaacore/log.h"
#include "kaacore/files.h"

#include "kaacore/texture_loader.h"


static bx::DefaultAllocator texture_image_allocator;


std::pair<bgfx::TextureHandle, bimg::ImageContainer*> load_texture(
    const uint8_t* data, size_t size, uint64_t flags
)
{
    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

    bimg::ImageContainer* image_container = \
        bimg::imageParse(&texture_image_allocator, data, size);

    if (image_container == NULL) {
        log<LogLevel::error>("Failed to parse texture data");
        return std::make_pair(handle, image_container);
    }
    // if (NULL != _orientation)
    // {
    //     *_orientation = image_container->m_orientation;
    // }

    const bgfx::Memory* mem = bgfx::makeRef(
        image_container->m_data, image_container->m_size
    );

    if (!bgfx::isTextureValid(0, false, image_container->m_numLayers,
            bgfx::TextureFormat::Enum(image_container->m_format), flags)) {
        log<LogLevel::error>("Texture is not valid");
        return std::make_pair(handle, image_container);
    }
    handle = bgfx::createTexture2D(
        uint16_t(image_container->m_width),
        uint16_t(image_container->m_height),
        1 < image_container->m_numMips,
        image_container->m_numLayers,
        bgfx::TextureFormat::Enum(image_container->m_format),
        flags,
        mem
    );

    // if (NULL != _info)
    // {
    //     bgfx::calcTextureSize(
    //             *_info
    //             , uint16_t(image_container->m_width)
    //             , uint16_t(image_container->m_height)
    //             , uint16_t(image_container->m_depth)
    //             , image_container->m_cubeMap
    //             , 1 < image_container->m_numMips
    //             , image_container->m_numLayers
    //             , bgfx::TextureFormat::Enum(image_container->m_format)
    //             );
    // }

    return std::make_pair(handle, image_container);
}


std::pair<bgfx::TextureHandle, bimg::ImageContainer*> load_texture_from_file(
    const char* path, uint64_t flags
)
{
    log("Loading texture from file: %s", path);
    RawFile file(path);
    log("Loaded file size: %d", file.content.size());
    auto handle = load_texture(
        file.content.data(), file.content.size(), flags
    );
    auto texture = std::get<bgfx::TextureHandle>(handle);

    if (bgfx::isValid(texture))
    {
        bgfx::setName(texture, path);
    }

    return handle;
}


Image::Image(const char* path, uint64_t flags)
{
    auto p = load_texture_from_file(path, flags);
    this->texture_handle = std::get<bgfx::TextureHandle>(p);
    this->image_container = std::get<bimg::ImageContainer*>(p);
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
    assert(this->image_container != nullptr);
    return {this->image_container->m_width, this->image_container->m_height};
}

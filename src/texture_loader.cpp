#include <bx/bx.h>
#include <bx/file.h>
#include <bimg/decode.h>
#include <bgfx/bgfx.h>

#include "kaacore/log.h"
#include "kaacore/files.h"

#include "kaacore/texture_loader.h"


static bx::DefaultAllocator texture_image_allocator;


static void destroy_image_container(void* ptr, void* user_data)
{
    bimg::ImageContainer* image_container = (bimg::ImageContainer*)user_data;
    bimg::imageFree(image_container);
}


bgfx::TextureHandle load_texture(const uint8_t* data, size_t size, uint64_t flags)
{
    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

    bimg::ImageContainer* image_container = \
        bimg::imageParse(&texture_image_allocator, data, size);

    if (image_container == NULL) {
        log<LogLevel::error>("Failed to parse texture data");
        return handle;
    }
    // if (NULL != _orientation)
    // {
    //     *_orientation = image_container->m_orientation;
    // }

    const bgfx::Memory* mem = bgfx::makeRef(
        image_container->m_data,
        image_container->m_size,
        destroy_image_container,
        image_container
    );

    if (!bgfx::isTextureValid(0, false, image_container->m_numLayers,
            bgfx::TextureFormat::Enum(image_container->m_format), flags))
    {
        log<LogLevel::error>("Texture is not valid");
        return handle;
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

    return handle;
}


bgfx::TextureHandle load_texture_from_file(const char* path, uint64_t flags)
{
    log("Loading texture from file: %s", path);
    RawFile file(path);
    bgfx::TextureHandle handle = load_texture(
        file.content.data(), file.content.size(), flags
    );

    if (bgfx::isValid(handle))
    {
        bgfx::setName(handle, path);
    }

    return handle;
}

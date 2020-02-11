#include <memory>
#include <string>

#include <bgfx/bgfx.h>
#include <bimg/decode.h>
#include <bx/bx.h>
#include <bx/file.h>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/files.h"
#include "kaacore/images.h"
#include "kaacore/log.h"

namespace kaacore {

static bx::DefaultAllocator texture_image_allocator;
ResourcesRegistry<std::string, Image> _images_registry;

void
initialize_image_resources()
{
    _images_registry.initialze();
}

void
uninitialize_image_resources()
{
    _images_registry.uninitialze();
}

bimg::ImageContainer*
load_image(const uint8_t* data, size_t size)
{
    bimg::ImageContainer* image_container =
        bimg::imageParse(&texture_image_allocator, data, size);
    assert(image_container != NULL);

    log("Image details - width: %u, height: %u, depth: %u, layers: %u, format: "
        "%u",
        image_container->m_width, image_container->m_height,
        image_container->m_depth, image_container->m_numLayers,
        image_container->m_format);

    return image_container;
}

bimg::ImageContainer*
load_image(const char* path)
{
    log("Loading image from file: %s", path);
    RawFile file(path);
    log("Loaded file size: %d", file.content.size());
    return load_image(file.content.data(), file.content.size());
}

bimg::ImageContainer*
load_raw_image(
    bimg::TextureFormat::Enum format, uint16_t width, uint16_t height,
    const std::vector<uint8_t>& data)
{
    bimg::ImageContainer* image_container = bimg::imageAlloc(
        &texture_image_allocator, format, width, height, 1, 1, false, false,
        data.data());

    assert(image_container != NULL);
    return image_container;
}

void
_free_image_container(void* _data, void* image_container)
{
    bimg::imageFree(static_cast<bimg::ImageContainer*>(image_container));
}

bgfx::TextureHandle
make_texture(bimg::ImageContainer* const image_container, const uint64_t flags)
{
    assert(bgfx::isTextureValid(
        0, false, image_container->m_numLayers,
        bgfx::TextureFormat::Enum(image_container->m_format), flags));

    const bgfx::Memory* mem = bgfx::makeRef(
        image_container->m_data, image_container->m_size, _free_image_container,
        image_container);

    return bgfx::createTexture2D(
        uint16_t(image_container->m_width), uint16_t(image_container->m_height),
        1 < image_container->m_numMips, image_container->m_numLayers,
        bgfx::TextureFormat::Enum(image_container->m_format), flags, mem);
}

Image::Image(const std::string& path, uint64_t flags) : path(path), flags(flags)
{
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

Image::Image(bimg::ImageContainer* image_container)
{
    this->texture_handle = make_texture(image_container);
    this->image_container = image_container;
    this->is_initialized = true;
}

Image::~Image()
{
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

ResourceReference<Image>
Image::load(const std::string& path, uint64_t flags)
{
    std::shared_ptr<Image> image;
    if (image = _images_registry.get_resource(path)) {
        return image;
    }
    image = std::shared_ptr<Image>(new Image(path, flags));
    _images_registry.register_resource(path, image);
    return image;
}

ResourceReference<Image>
Image::load(bimg::ImageContainer* image_container)
{
    return std::shared_ptr<Image>(new Image(image_container));
}

glm::uvec2
Image::get_dimensions()
{
    KAACORE_CHECK(this->image_container != nullptr);
    return {this->image_container->m_width, this->image_container->m_height};
}

void
Image::_initialize()
{
    printf("@@@@@@@@@@@@@ INIT @@@@@@@\n");
    auto raw_path = this->path.c_str();
    this->image_container = load_image(raw_path);
    this->texture_handle = make_texture(this->image_container, flags);
    bgfx::setName(this->texture_handle, raw_path);
    this->is_initialized = true;
}

void
Image::_uninitialize()
{
    if (bgfx::isValid(this->texture_handle)) {
        bgfx::destroy(this->texture_handle);
    }
    _images_registry.unregister_resource(this->path);
    this->is_initialized = false;
}

} // namespace kaacore

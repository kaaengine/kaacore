#include <memory>
#include <string>
#include <unordered_set>

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
// bump up Image's ref count while it's being used by bgfx,
// this may take up to 2 frames
std::unordered_set<std::shared_ptr<Image>> _used_images;

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

Image::Image(const std::string& path, uint64_t flags) : path(path), flags(flags)
{
    this->image_container = load_image(path.c_str());
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

Image::Image(bimg::ImageContainer* image_container)
    : image_container(image_container)
{
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

Image::~Image()
{
    _images_registry.unregister_resource(this->path);
    bimg::imageFree(this->image_container);
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

ResourceReference<Image>
Image::load(const std::string& path, uint64_t flags)
{
    std::shared_ptr<Image> image;
    if ((image = _images_registry.get_resource(path))) {
        return image;
    }
    image = std::shared_ptr<Image>(new Image(path, flags));
    _images_registry.register_resource(path, image);
    _used_images.insert(image);
    return image;
}

ResourceReference<Image>
Image::load(bimg::ImageContainer* image_container)
{
    auto image = std::shared_ptr<Image>(new Image(image_container));
    _used_images.insert(image);
    return image;
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
    this->texture_handle = this->_make_texture();
    bgfx::setName(this->texture_handle, this->path.c_str());
    this->is_initialized = true;
}

void
Image::_uninitialize()
{
    if (bgfx::isValid(this->texture_handle)) {
        bgfx::destroy(this->texture_handle);
    }
    this->is_initialized = false;
}

void
_cleanup_used_image(void* _data, void* image)
{
    // use aliasing constructor in order to create non-owning ptr
    // that we will use as a key
    std::shared_ptr<Image> key(
        std::shared_ptr<Image>(), static_cast<Image*>(image));
    _used_images.erase(key);
}

bgfx::TextureHandle
Image::_make_texture()
{
    assert(bgfx::isTextureValid(
        0, false, this->image_container->m_numLayers,
        bgfx::TextureFormat::Enum(this->image_container->m_format),
        this->flags));

    const bgfx::Memory* mem = bgfx::makeRef(
        this->image_container->m_data, this->image_container->m_size,
        _cleanup_used_image, this);

    return bgfx::createTexture2D(
        uint16_t(this->image_container->m_width),
        uint16_t(this->image_container->m_height),
        1 < this->image_container->m_numMips,
        this->image_container->m_numLayers,
        bgfx::TextureFormat::Enum(this->image_container->m_format), this->flags,
        mem);
}

} // namespace kaacore

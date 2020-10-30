#include <memory>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/files.h"
#include "kaacore/images.h"
#include "kaacore/log.h"

namespace kaacore {

static bx::DefaultAllocator texture_image_allocator;
ResourcesRegistry<std::string, Image> _images_registry;

void
initialize_images()
{
    _images_registry.initialze();
}

void
uninitialize_images()
{
    _images_registry.uninitialze();
}

void
_destroy_image_container(bimg::ImageContainer* image_container)
{
    bimg::imageFree(image_container);
}

bimg::ImageContainer*
load_image(const uint8_t* data, size_t size)
{
    bimg::ImageContainer* image_container =
        bimg::imageParse(&texture_image_allocator, data, size);
    assert(image_container != NULL);

    KAACORE_LOG_INFO(
        "Image details - width: {}, height: {}, depth: {}, layers: {}, alpha: "
        "{}, format: {}",
        image_container->m_width, image_container->m_height,
        image_container->m_depth, image_container->m_numLayers,
        image_container->m_hasAlpha, image_container->m_format);

    return image_container;
}

bimg::ImageContainer*
load_image(const char* path)
{
    KAACORE_LOG_INFO("Loading image from file: {}", path);
    RawFile file(path);
    KAACORE_LOG_INFO("Loaded file size: {}", file.content.size());
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
    this->image_container = std::shared_ptr<bimg::ImageContainer>(
        load_image(path.c_str()), _destroy_image_container);
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

Image::Image(bimg::ImageContainer* image_container)
{
    this->image_container = std::shared_ptr<bimg::ImageContainer>(
        image_container, _destroy_image_container);
    if (is_engine_initialized()) {
        this->_initialize();
    }
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
    if ((image = _images_registry.get_resource(path))) {
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
    KAACORE_CHECK(this->image_container != nullptr, "Invalid image container.");
    return {this->image_container->m_width, this->image_container->m_height};
}

void
Image::_initialize()
{
    this->texture_handle = get_engine()->renderer->make_texture(
        this->image_container, this->flags);
    bgfx::setName(this->texture_handle, this->path.c_str());
    this->is_initialized = true;
}

void
Image::_uninitialize()
{
    get_engine()->renderer->destroy_texture(this->texture_handle);
    this->is_initialized = false;
}

} // namespace kaacore

#include <memory>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/files.h"
#include "kaacore/log.h"
#include "kaacore/textures.h"

namespace kaacore {

static bx::DefaultAllocator texture_image_allocator;
ResourcesRegistry<std::string, Texture> _textures_registry;

void
initialize_textures()
{
    _textures_registry.initialze();
}

void
uninitialize_textures()
{
    _textures_registry.uninitialze();
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
load_image(const std::string& path)
{
    KAACORE_LOG_INFO("Loading image from file: {}", path);
    File file(path);
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

Texture::Texture(const std::string& path, uint64_t flags)
    : path(path), flags(flags)
{
    this->image_container = std::shared_ptr<bimg::ImageContainer>(
        load_image(path), _destroy_image_container);
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

Texture::Texture(bimg::ImageContainer* image_container)
{
    this->image_container = std::shared_ptr<bimg::ImageContainer>(
        image_container, _destroy_image_container);
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

Texture::~Texture()
{
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

ResourceReference<Texture>
Texture::load(const std::string& path, uint64_t flags)
{
    std::shared_ptr<Texture> texture;
    if ((texture = _textures_registry.get_resource(path))) {
        return texture;
    }
    texture = std::shared_ptr<Texture>(new Texture(path, flags));
    _textures_registry.register_resource(path, texture);
    return texture;
}

ResourceReference<Texture>
Texture::load(bimg::ImageContainer* image_container)
{
    return std::shared_ptr<Texture>(new Texture(image_container));
}

glm::uvec2
Texture::get_dimensions()
{
    KAACORE_CHECK(this->image_container != nullptr, "Invalid image container.");
    return {this->image_container->m_width, this->image_container->m_height};
}

void
Texture::_initialize()
{
    this->handle = get_engine()->renderer->make_texture(
        this->image_container, this->flags);
    bgfx::setName(this->handle, this->path.c_str());
    this->is_initialized = true;
}

void
Texture::_uninitialize()
{
    get_engine()->renderer->destroy_texture(this->handle);
    this->is_initialized = false;
}

} // namespace kaacore

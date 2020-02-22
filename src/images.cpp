#include <deque>
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

/*
 * Helper class that takes care of images being loaded to bgfx.
 *
 * Since the memory that is used to load texture to bgfx should be available
 * at least for two frames, we bump up its ref count by storing it in a set.
 */
struct _Limbo {
    void add(std::shared_ptr<Image> ref) { this->_used_images.insert(ref); }

    void enqueue_to_remove(Image* ptr) { _enqueued_to_remove.push_front(ptr); }

    void process()
    {
        while (this->_enqueued_to_remove.size()) {
            auto image_ptr = this->_enqueued_to_remove.back();
            // Images are stored as raw pointers to save time for
            // incrementing/decrementing ref count while are passed around,
            // but at this point we have to construct shared_ptr.
            // Use aliasing constructor to create non-owning ptr that we will
            // use as a key
            std::shared_ptr<Image> key(
                std::shared_ptr<Image>(), static_cast<Image*>(image_ptr));
            this->_used_images.erase(key);
            this->_enqueued_to_remove.pop_back();
        }
    }

    void clear()
    {
        this->process();
        this->_used_images.clear();
    }

  private:
    std::deque<Image*> _enqueued_to_remove;
    std::unordered_set<std::shared_ptr<Image>> _used_images;
} _limbo;

void
initialize_images()
{
    _images_registry.initialze();
}

void
images_on_frame()
{
    _limbo.process();
}

void
uninitialize_images()
{
    _limbo.clear();
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
    _limbo.add(image);
    return image;
}

ResourceReference<Image>
Image::load(bimg::ImageContainer* image_container)
{
    auto image = std::shared_ptr<Image>(new Image(image_container));
    _limbo.add(image);
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
    // due to internal bgfx restrictions we are not allowed to destroy
    // texture from this callback, bypass this by queuing pointer
    // for later deletion
    _limbo.enqueue_to_remove(static_cast<Image*>(image));
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

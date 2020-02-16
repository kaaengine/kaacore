#include "kaacore/resources_manager.h"
#include "kaacore/audio.h"
#include "kaacore/fonts.h"
#include "kaacore/images.h"

namespace kaacore {

ResourcesManager::ResourcesManager()
{
    this->_initialize_resources();
}

ResourcesManager::~ResourcesManager()
{
    this->_uninitialize_resources();
}

void
ResourcesManager::_initialize_resources()
{
    initialize_image_resources();
    initialize_audio_resources();
    initialize_font_resources();
}

void
ResourcesManager::_uninitialize_resources()
{
    uninitialize_image_resources();
    uninitialize_audio_resources();
    uninitialize_font_resources();
}

} // namespace kaacore

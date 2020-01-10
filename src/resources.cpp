#include "kaacore/resources.h"

namespace kaacore {

std::unordered_map<std::string, std::weak_ptr<Resource>> _registered_resources;

void
register_resource(
    const std::string& key, const std::shared_ptr<Resource>& resource)
{
    if (_registered_resources.find(key) != _registered_resources.end()) {
        throw exception(
            std::string("Detected attempt to override already registered "
                        "resource. (key=") +
            key + ")");
    }
    _registered_resources[key] = resource;
}

std::shared_ptr<Resource>
get_registered_resource(const std::string& key)
{
    auto it = _registered_resources.find(key);
    if (it == _registered_resources.end()) {
        return nullptr;
    }
    return it->second.lock();
}

void
deregiser_resource(const std::string& key)
{
    _registered_resources.erase(key);
}

Resource::Resource(const std::string& key) : key(key) {}

Resource::Resource() : key("") {}

Resource::~Resource()
{
    if (not this->key.empty()) {
        deregiser_resource(this->key);
    }
}

ResourceManager::ResourceManager()
{
    this->_initialize_resources();
}

ResourceManager::~ResourceManager()
{
    this->_uninitialize_resources();
}

void
ResourceManager::_initialize_resources()
{
    for (auto it = _registered_resources.begin();
         it != _registered_resources.end(); it++) {
        it->second.lock()->_initialize();
    }
}

void
ResourceManager::_uninitialize_resources()
{
    for (auto it = _registered_resources.begin();
         it != _registered_resources.end(); it++) {
        it->second.lock()->_uninitialize();
    }
}

} // namespace kaacore

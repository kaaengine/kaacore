#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "kaacore/exceptions.h"

namespace kaacore {

class Resource;

void
register_resource(
    const std::string& key, const std::shared_ptr<Resource>& resource);

std::shared_ptr<Resource>
get_registered_resource(const std::string& key);

void
deregiser_resource(const std::string& key);

class ResourceManager;
class Resource {
  public:
    const std::string key;
    bool is_initialized = false;

    virtual ~Resource();

  protected:
    Resource();
    Resource(const std::string& key);
    virtual void _initialize() = 0;
    virtual void _uninitialize() = 0;

    friend class ResourceManager;
};

template<typename T>
struct ResourceReference {
    std::shared_ptr<T> res_ptr;

    ResourceReference() : res_ptr(nullptr) {}
    ResourceReference(std::shared_ptr<T> ptr) : res_ptr(ptr) {}
    inline operator bool() const { return bool(this->res_ptr); }
    T* operator->() const
    {
        auto ptr = this->res_ptr;
        if (ptr and not ptr->is_initialized) {
            throw exception(
                std::string(
                    "Detected access to uninitialized resource. (key=") +
                ptr->key + ")");
        }
        return ptr.get();
    }
};

class ResourceManager {
  public:
    ResourceManager();
    ~ResourceManager();

  private:
    void _initialize_resources();
    void _uninitialize_resources();
};

} // namespace kaacore

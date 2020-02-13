#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "kaacore/exceptions.h"

namespace kaacore {

class Resource {
  public:
    bool is_initialized = false;

    virtual ~Resource() = 0;

  protected:
    virtual void _initialize() = 0;
    virtual void _uninitialize() = 0;
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
            throw exception("Detected access to uninitialized resource.");
        }
        return ptr.get();
    }
};

template<typename Key_T, typename Resource_T>
class ResourcesRegistry {
  public:
    void initialze()
    {
        for (auto& it : this->_registry) {
            it.second.lock()->_initialize();
        }
    }
    void uninitialze()
    {
        for (auto& it : this->_registry) {
            it.second.lock()->_uninitialize();
        }
    }
    void register_resource(
        const Key_T& key, const std::weak_ptr<Resource_T> resource)
    {
        KAACORE_ASSERT(this->_registry.find(key) == _registry.end());
        this->_registry[key] = resource;
    }
    std::shared_ptr<Resource_T> get_resource(const Key_T& key)
    {
        auto it = this->_registry.find(key);
        if (it == this->_registry.end()) {
            return nullptr;
        }
        return it->second.lock();
    }
    void unregister_resource(const Key_T& key) { this->_registry.erase(key); }

  private:
    static inline std::unordered_map<Key_T, std::weak_ptr<Resource_T>>
        _registry;
};

} // namespace kaacore

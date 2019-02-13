#pragma once

#include <memory>


template <typename T>
struct Resource {
    std::shared_ptr<T> res_ptr;

    Resource() : res_ptr(nullptr) {}
    Resource(std::shared_ptr<T> ptr) : res_ptr(ptr) {}

    inline operator bool() const
    {
        return bool(this->res_ptr);
    }

    T* operator->() const
    {
        return this->res_ptr.get();
    }
};

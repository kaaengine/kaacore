#pragma once

#include <functional>

namespace kaacore {

using ResourceSystemCallback = std::function<void()>;

enum class ResourceSystemCallbackType {
    pre_init,
    post_init,
    pre_uninit,
    post_uninit
};

class ResourcesManager {
  public:
    ResourcesManager();
    ~ResourcesManager();

  private:
    void _initialize_resources();
    void _uninitialize_resources();
};

} // namespace kaacore

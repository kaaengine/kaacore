#pragma once

namespace kaacore {

class ResourcesManager {
  public:
    ResourcesManager();
    ~ResourcesManager();

    void on_frame();

  private:
    void _initialize_resources();
    void _uninitialize_resources();
};

} // namespace kaacore

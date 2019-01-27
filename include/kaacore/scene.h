#pragma once

#include <vector>

#include "kaacore/nodes.h"
#include "kaacore/input.h"


struct Scene {
    Node root_node;

    uint64_t time = 0;

    Scene() = default;
    virtual ~Scene() = default;

    virtual void process_frame(uint32_t dt);
    virtual void update(uint32_t dt);
    // TODO on_enter, on_exit?

    const std::vector<Event>& get_events() const;
};

#pragma once

#include <functional>
#include <memory>

#include "kaacore/engine.h"
#include "kaacore/scenes.h"

typedef std::function<void(uint32_t)> TestingSceneUpdateFunction;

struct TestingScene : kaacore::Scene {
    void update(const kaacore::Duration dt) override;
    void run_on_engine(uint32_t frames);

    std::function<void(const kaacore::Duration dt)> update_function = nullptr;
    uint32_t frames_left = 0;
};

std::unique_ptr<kaacore::Engine>
initialize_testing_engine(bool window_visible = false);

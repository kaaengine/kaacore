#pragma once

#include <functional>
#include <memory>

#include "kaacore/engine.h"
#include "kaacore/scenes.h"

using namespace kaacore;

typedef std::function<void(uint32_t)> TestingSceneUpdateFunction;

struct TestingScene : Scene {
    void update(const Duration dt) override;
    void run_on_engine(uint32_t frames);

    std::function<void(const Duration dt)> update_function = nullptr;
    uint32_t frames_left = 0;
};

std::unique_ptr<Engine>
initialize_testing_engine();

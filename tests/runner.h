#pragma once

#include <functional>
#include <memory>

#include "kaacore/engine.h"
#include "kaacore/scenes.h"

using namespace kaacore;

typedef std::function<void(uint32_t)> TestingSceneUpdateFunction;

struct TestingScene : Scene {
    void update(uint32_t dt);
    void run_on_engine(uint32_t frames);

    std::function<void(uint32_t)> update_function = nullptr;
    uint32_t frames_left = 0;
};

std::unique_ptr<Engine>
initialize_testing_engine();

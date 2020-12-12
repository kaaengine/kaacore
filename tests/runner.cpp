#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <glm/glm.hpp>

#include "kaacore/engine.h"

#include "runner.h"

using namespace kaacore;

extern "C" int
main(int argc, char* argv[])
{
    int result = Catch::Session().run(argc, argv);

    return result;
}

void
TestingScene::update(uint32_t dt)
{
    if (this->frames_left == 0) {
        get_engine()->quit();
        return;
    }
    this->update_function(dt);
    this->frames_left--;
}

void
TestingScene::run_on_engine(uint32_t frames)
{
    this->frames_left = frames;
    get_engine()->run(this);
}

std::unique_ptr<Engine>
initialize_testing_engine()
{
    auto engine = std::make_unique<Engine>(glm::dvec2{100, 100});
    engine->window->hide();
    return engine;
}

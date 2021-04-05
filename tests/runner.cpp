#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <glm/glm.hpp>

#include "kaacore/engine.h"

#include "runner.h"

extern "C" int
main(int argc, char* argv[])
{
    int result = Catch::Session().run(argc, argv);

    return result;
}

void
TestingScene::update(const kaacore::Duration dt)
{
    if (this->frames_left == 0) {
        kaacore::get_engine()->quit();
        return;
    }
    this->update_function(dt);
    this->frames_left--;
}

void
TestingScene::run_on_engine(uint32_t frames)
{
    this->frames_left = frames;
    kaacore::get_engine()->run(this);
}

std::unique_ptr<kaacore::Engine>
initialize_testing_engine()
{
    return std::make_unique<kaacore::Engine>(glm::dvec2{1, 1});
}

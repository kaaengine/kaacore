#include "kaacore/engine.h"
#include "kaacore/input.h"
#include "kaacore/log.h"
#include "kaacore/scenes.h"

using namespace kaacore;

struct DemoScene : Scene {

    void update(uint32_t dt) override
    {
        for (auto const& event : this->get_events()) {
            auto system = event.system();
            auto keyboard = event.keyboard();
            if ((keyboard and keyboard->is_pressing(Keycode::q)) or
                (system and system->quit())) {
                get_engine()->quit();
                break;
            }

            if (auto controller = event.controller()) {
                if (controller->button()) {
                    if (controller->is_pressing(ControllerButton::a)) {
                        log<LogLevel::info>("A button pressed.");
                    } else if (controller->is_pressing(ControllerButton::b)) {
                        log<LogLevel::info>("B button pressed.");
                    } else if (controller->is_pressing(ControllerButton::x)) {
                        log<LogLevel::info>("X button pressed.");
                    } else if (controller->is_pressing(ControllerButton::y)) {
                        log<LogLevel::info>("Y button pressed.");
                    } else if (controller->is_pressing(
                                   ControllerButton::dpad_up)) {
                        log<LogLevel::info>("Up button pressed.");
                    } else if (controller->is_pressing(
                                   ControllerButton::dpad_down)) {
                        log<LogLevel::info>("Down button pressed.");
                    } else if (controller->is_pressing(
                                   ControllerButton::dpad_left)) {
                        log<LogLevel::info>("Left button pressed.");
                    } else if (controller->is_pressing(
                                   ControllerButton::dpad_right)) {
                        log<LogLevel::info>("Right button pressed.");
                    } else if (controller->is_pressing(
                                   ControllerButton::left_shoulder)) {
                        log<LogLevel::info>("Left shoulder button pressed.");
                    } else if (controller->is_pressing(
                                   ControllerButton::right_shoulder)) {
                        log<LogLevel::info>("Right shoulder button pressed.");
                    } else if (controller->is_pressing(
                                   ControllerButton::left_stick)) {
                        log<LogLevel::info>("Left stick button pressed.");
                    } else if (controller->is_pressing(
                                   ControllerButton::right_stick)) {
                        log<LogLevel::info>("Right stick button pressed.");
                    } else if (controller->is_pressing(
                                   ControllerButton::back)) {
                        log<LogLevel::info>("Back button pressed.");
                    } else if (controller->is_pressing(
                                   ControllerButton::start)) {
                        log<LogLevel::info>("Start button pressed.");
                    }
                } else if (controller->axis()) {
                    auto left_x =
                        controller->axis_motion(ControllerAxis::left_x);
                    auto left_y =
                        controller->axis_motion(ControllerAxis::left_y);
                    auto right_x =
                        controller->axis_motion(ControllerAxis::right_x);
                    auto right_y =
                        controller->axis_motion(ControllerAxis::right_y);
                    auto trigger_left =
                        controller->axis_motion(ControllerAxis::trigger_left);
                    auto trigger_right =
                        controller->axis_motion(ControllerAxis::trigger_right);

                    if (left_x or left_y) {
                        log<LogLevel::info>(
                            "Left stick motion: %f, %f", left_x, left_y);
                    } else if (right_x or right_y) {
                        log<LogLevel::info>(
                            "Right stick motion: %f, %f", right_x, right_y);
                    } else if (trigger_left) {
                        log<LogLevel::info>(
                            "Left trigger motion: %f", trigger_left);
                    } else if (trigger_right) {
                        log<LogLevel::info>(
                            "Right trigger motion: %f", trigger_right);
                    }
                }
            }
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    Engine eng({800, 600});
    eng.window->show();
    DemoScene scene;
    scene.camera.position = {0., 0.};
    eng.run(&scene);

    return 0;
}

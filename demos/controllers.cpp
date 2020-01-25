#include "kaacore/engine.h"
#include "kaacore/input.h"
#include "kaacore/log.h"
#include "kaacore/scenes.h"

using namespace kaacore;

struct DemoScene : Scene {

    void update(uint32_t dt) override
    {
        for (auto const& event : this->get_events()) {
            auto keyboard_key = event.keyboard_key();
            if (keyboard_key and keyboard_key->key() == Keycode::q) {
                get_engine()->quit();
                break;
            }

            if (auto controller_button = event.controller_button()) {
                switch (controller_button->button()) {
                    case ControllerButton::a:
                        log<LogLevel::info>("A button pressed.");
                        break;
                    case ControllerButton::b:
                        log<LogLevel::info>("B button pressed.");
                        break;
                    case ControllerButton::x:
                        log<LogLevel::info>("X button pressed.");
                        break;
                    case ControllerButton::y:
                        log<LogLevel::info>("Y button pressed.");
                        break;
                    case ControllerButton::dpad_up:
                        log<LogLevel::info>("Up button pressed.");
                        break;
                    case ControllerButton::dpad_down:
                        log<LogLevel::info>("Down button pressed.");
                        break;
                    case ControllerButton::dpad_left:
                        log<LogLevel::info>("Left button pressed.");
                        break;
                    case ControllerButton::dpad_right:
                        log<LogLevel::info>("Right button pressed.");
                        break;
                    case ControllerButton::left_shoulder:
                        log<LogLevel::info>("Left shoulder button pressed.");
                        break;
                    case ControllerButton::right_shoulder:
                        log<LogLevel::info>("Right shoulder button pressed.");
                        break;
                    case ControllerButton::left_stick:
                        log<LogLevel::info>("Left stick button pressed.");
                        break;
                    case ControllerButton::right_stick:
                        log<LogLevel::info>("Right stick button pressed.");
                        break;
                    case ControllerButton::back:
                        log<LogLevel::info>("Back button pressed.");
                        break;
                    case ControllerButton::start:
                        log<LogLevel::info>("Start button pressed.");
                        break;
                    case ControllerButton::guide:
                        log<LogLevel::info>("Guide button pressed.");
                        break;
                }
            }

            if (auto controller_motion = event.controller_axis()) {
                auto left_x =
                    controller_motion->axis_motion(ControllerAxis::left_x);
                auto left_y =
                    controller_motion->axis_motion(ControllerAxis::left_y);
                auto right_x =
                    controller_motion->axis_motion(ControllerAxis::right_x);
                auto right_y =
                    controller_motion->axis_motion(ControllerAxis::right_y);
                auto trigger_left = controller_motion->axis_motion(
                    ControllerAxis::trigger_left);
                auto trigger_right = controller_motion->axis_motion(
                    ControllerAxis::trigger_right);

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

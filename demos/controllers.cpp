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

            auto controller_button = event.controller_button();
            if (controller_button && controller_button->is_button_up()) {
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
            } else if (auto controller_motion = event.controller_axis()) {
                switch (controller_motion->axis()) {
                    case ControllerAxis::left_x:
                        log<LogLevel::info>(
                            "Left stick motion: %f, 0.0",
                            controller_motion->motion());
                        break;
                    case ControllerAxis::left_y:
                        log<LogLevel::info>(
                            "Left stick motion: 0.0, %f",
                            controller_motion->motion());
                        break;
                    case ControllerAxis::right_x:
                        log<LogLevel::info>(
                            "Right stick motion: %f, 0.0",
                            controller_motion->motion());
                        break;
                    case ControllerAxis::right_y:
                        log<LogLevel::info>(
                            "Right stick motion: 0.0, %f",
                            controller_motion->motion());
                        break;
                    case ControllerAxis::trigger_left:
                        log<LogLevel::info>(
                            "Right trigger motion: %f, 0.0",
                            controller_motion->motion());
                        break;
                    case ControllerAxis::trigger_right:
                        log<LogLevel::info>(
                            "Right trigger motion: 0.0, %f",
                            controller_motion->motion());
                        break;
                }
            } else if (auto controller_device = event.controller_device()) {
                if (controller_device->is_added()) {
                    log<LogLevel::info>(
                        "Controller added: %d", controller_device->id());

                } else if (controller_device->is_removed()) {
                    log<LogLevel::info>(
                        "Controller removed: %d", controller_device->id());
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

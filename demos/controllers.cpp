#include "kaacore/engine.h"
#include "kaacore/input.h"
#include "kaacore/log.h"
#include "kaacore/scenes.h"

struct DemoScene : kaacore::Scene {

    void update(const kaacore::Duration dt) override
    {
        for (auto const& event : this->get_events()) {
            auto keyboard_key = event.keyboard_key();
            if (keyboard_key and keyboard_key->key() == kaacore::Keycode::q) {
                kaacore::get_engine()->quit();
                break;
            }

            auto controller_button = event.controller_button();
            if (controller_button && controller_button->is_button_up()) {
                switch (controller_button->button()) {
                    case kaacore::ControllerButton::a:
                        KAACORE_APP_LOG_INFO("A button pressed.");
                        break;
                    case kaacore::ControllerButton::b:
                        KAACORE_APP_LOG_INFO("B button pressed.");
                        break;
                    case kaacore::ControllerButton::x:
                        KAACORE_APP_LOG_INFO("X button pressed.");
                        break;
                    case kaacore::ControllerButton::y:
                        KAACORE_APP_LOG_INFO("Y button pressed.");
                        break;
                    case kaacore::ControllerButton::dpad_up:
                        KAACORE_APP_LOG_INFO("Up button pressed.");
                        break;
                    case kaacore::ControllerButton::dpad_down:
                        KAACORE_APP_LOG_INFO("Down button pressed.");
                        break;
                    case kaacore::ControllerButton::dpad_left:
                        KAACORE_APP_LOG_INFO("Left button pressed.");
                        break;
                    case kaacore::ControllerButton::dpad_right:
                        KAACORE_APP_LOG_INFO("Right button pressed.");
                        break;
                    case kaacore::ControllerButton::left_shoulder:
                        KAACORE_APP_LOG_INFO("Left shoulder button pressed.");
                        break;
                    case kaacore::ControllerButton::right_shoulder:
                        KAACORE_APP_LOG_INFO("Right shoulder button pressed.");
                        break;
                    case kaacore::ControllerButton::left_stick:
                        KAACORE_APP_LOG_INFO("Left stick button pressed.");
                        break;
                    case kaacore::ControllerButton::right_stick:
                        KAACORE_APP_LOG_INFO("Right stick button pressed.");
                        break;
                    case kaacore::ControllerButton::back:
                        KAACORE_APP_LOG_INFO("Back button pressed.");
                        break;
                    case kaacore::ControllerButton::start:
                        KAACORE_APP_LOG_INFO("Start button pressed.");
                        break;
                    case kaacore::ControllerButton::guide:
                        KAACORE_APP_LOG_INFO("Guide button pressed.");
                        break;
                }
            } else if (auto controller_motion = event.controller_axis()) {
                switch (controller_motion->axis()) {
                    case kaacore::ControllerAxis::left_x:
                        KAACORE_APP_LOG_INFO(
                            "Left stick motion: {}, 0.0",
                            controller_motion->motion()
                        );
                        break;
                    case kaacore::ControllerAxis::left_y:
                        KAACORE_APP_LOG_INFO(
                            "Left stick motion: 0.0, {}",
                            controller_motion->motion()
                        );
                        break;
                    case kaacore::ControllerAxis::right_x:
                        KAACORE_APP_LOG_INFO(
                            "Right stick motion: {}, 0.0",
                            controller_motion->motion()
                        );
                        break;
                    case kaacore::ControllerAxis::right_y:
                        KAACORE_APP_LOG_INFO(
                            "Right stick motion: 0.0, {}",
                            controller_motion->motion()
                        );
                        break;
                    case kaacore::ControllerAxis::trigger_left:
                        KAACORE_APP_LOG_INFO(
                            "Right trigger motion: {}, 0.0",
                            controller_motion->motion()
                        );
                        break;
                    case kaacore::ControllerAxis::trigger_right:
                        KAACORE_APP_LOG_INFO(
                            "Right trigger motion: 0.0, {}",
                            controller_motion->motion()
                        );
                        break;
                }
            } else if (auto controller_device = event.controller_device()) {
                if (controller_device->is_added()) {
                    KAACORE_APP_LOG_INFO(
                        "Controller added: {}", controller_device->id()
                    );

                } else if (controller_device->is_removed()) {
                    KAACORE_APP_LOG_INFO(
                        "Controller removed: {}", controller_device->id()
                    );
                }
            }
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    kaacore::Engine eng({800, 600});
    DemoScene scene;
    scene.camera().position({0., 0.});
    eng.run(&scene);

    return 0;
}

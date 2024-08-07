#include <cmath>
#include <limits>

#include <SDL.h>

#include "kaacore/audio.h"
#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/log.h"

#include "kaacore/input.h"

namespace kaacore {

bool
_handle_quit(const Event& event)
{
    get_engine()->quit();
    return false;
}

glm::dvec2
_scale_vector_to_virtual_resolution(int32_t x, int32_t y)
{
    auto engine = get_engine();
    double final_x = x * static_cast<double>(engine->virtual_resolution().x) /
                     static_cast<double>(engine->renderer->view_size.x);
    double final_y = y * static_cast<double>(engine->virtual_resolution().y) /
                     static_cast<double>(engine->renderer->view_size.y);
    return {final_x, final_y};
}

glm::dvec2
_naive_screen_position_to_virtual_resolution(int32_t x, int32_t y)
{
    auto border_size = get_engine()->renderer->border_size;
    x -= border_size.x, y -= border_size.y;
    return _scale_vector_to_virtual_resolution(x, y);
}

double
_normalize_controller_axis(int16_t value)
{
    if (value >= 0) {
        return static_cast<double>(value) / SHRT_MAX;
    }
    return static_cast<double>(value) / std::abs(SHRT_MIN);
}

bool
operator==(const EventType& event_type, const uint32_t& event_num)
{
    return static_cast<uint32_t>(event_type) == event_num;
}

bool
operator==(const uint32_t& event_num, const EventType& event_type)
{
    return static_cast<uint32_t>(event_type) == event_num;
}

EventType
_sdl_to_kaacore_type(const SDL_Event& sdl_event)
{
    if (sdl_event.type == SDL_WINDOWEVENT) {
        return static_cast<EventType>(sdl_event.window.event);
    }
    return static_cast<EventType>(sdl_event.type);
}

EventType
BaseEvent::type() const
{
    return _sdl_to_kaacore_type(this->sdl_event);
}

uint32_t
BaseEvent::timestamp() const
{
    return this->sdl_event.common.timestamp;
}

bool
SystemEvent::is_quit() const
{
    return this->type() == EventType::quit;
}

bool
SystemEvent::is_clipboard_updated() const
{
    return this->type() == EventType::clipboard_updated;
}

bool
WindowEvent::is_shown() const
{
    return this->type() == EventType::window_shown;
}

bool
WindowEvent::is_exposed() const
{
    return this->type() == EventType::window_exposed;
}

bool
WindowEvent::is_moved() const
{
    return this->type() == EventType::window_moved;
}

bool
WindowEvent::is_resized() const
{
    return this->type() == EventType::window_resized;
}

bool
WindowEvent::is_minimized() const
{
    return this->type() == EventType::window_minimized;
}

bool
WindowEvent::is_maximized() const
{
    return this->type() == EventType::window_maximized;
}

bool
WindowEvent::is_restored() const
{
    return this->type() == EventType::window_restored;
}

bool
WindowEvent::is_enter() const
{
    return this->type() == EventType::window_enter;
}

bool
WindowEvent::is_leave() const
{
    return this->type() == EventType::window_leave;
}

bool
WindowEvent::is_focus_gained() const
{
    return this->type() == EventType::window_focus_gained;
}

bool
WindowEvent::is_focus_lost() const
{
    return this->type() == EventType::window_focus_lost;
}

bool
WindowEvent::is_close() const
{
    return this->type() == EventType::window_close;
}

Keycode
KeyboardKeyEvent::key() const
{
    return static_cast<Keycode>(this->sdl_event.key.keysym.sym);
}

bool
KeyboardKeyEvent::is_key_down() const
{
    return this->type() == EventType::key_down;
}

bool
KeyboardKeyEvent::is_key_up() const
{
    return this->type() == EventType::key_up;
}

bool
KeyboardKeyEvent::repeat() const
{
    return this->sdl_event.key.repeat;
}

std::string
KeyboardTextEvent::text() const
{
    return this->sdl_event.text.text;
}

MouseButton
MouseButtonEvent::button() const
{
    return static_cast<MouseButton>(this->sdl_event.button.button);
}

bool
MouseButtonEvent::is_button_down() const
{
    return this->type() == EventType::mouse_button_down;
}

bool
MouseButtonEvent::is_button_up() const
{
    return this->type() == EventType::mouse_button_up;
}

glm::dvec2
MouseButtonEvent::position() const
{
    return _naive_screen_position_to_virtual_resolution(
        this->sdl_event.button.x, this->sdl_event.button.y
    );
}

glm::dvec2
MouseMotionEvent::position() const
{
    return _naive_screen_position_to_virtual_resolution(
        this->sdl_event.motion.x, this->sdl_event.motion.y
    );
}

glm::dvec2
MouseMotionEvent::motion() const
{
    return _scale_vector_to_virtual_resolution(
        sdl_event.motion.xrel, this->sdl_event.motion.yrel
    );
}

glm::dvec2
MouseWheelEvent::scroll() const
{
    auto direction =
        glm::dvec2(this->sdl_event.wheel.x, this->sdl_event.wheel.y);
    // positive Y axis goes down
    direction *= -1;

    if (this->sdl_event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
        direction *= -1;
    }
    return direction;
}

ControllerId
BaseControllerEvent::id() const
{
    return this->sdl_event.cdevice.which;
}

ControllerButton
ControllerButtonEvent::button() const
{
    return static_cast<ControllerButton>(this->sdl_event.cbutton.button);
}

bool
ControllerButtonEvent::is_button_down() const
{
    return this->type() == EventType::controller_button_down;
}

bool
ControllerButtonEvent::is_button_up() const
{
    return this->type() == EventType::controller_button_up;
}

ControllerAxis
ControllerAxisEvent::axis() const
{
    return static_cast<ControllerAxis>(this->sdl_event.caxis.axis);
}

double
ControllerAxisEvent::motion() const
{
    return _normalize_controller_axis(this->sdl_event.caxis.value);
}

Event::Event() {}
Event::Event(SDL_Event sdl_event)
{
    BaseEvent event;
    event.sdl_event = sdl_event;
    this->common = event;
}

EventType
Event::type() const
{
    return this->common.type();
}

uint32_t
Event::timestamp() const
{
    return this->common.timestamp();
}

const SystemEvent* const
Event::system() const
{
    auto type = this->type();
    if (type == EventType::quit or type == EventType::clipboard_updated) {
        return &this->_system;
    }
    return nullptr;
}

const WindowEvent* const
Event::window() const
{
    if (this->common.sdl_event.type == SDL_WINDOWEVENT) {
        return &this->_window;
    }
    return nullptr;
}

const KeyboardKeyEvent* const
Event::keyboard_key() const
{
    auto type = this->type();
    if (type == EventType::key_up or type == EventType::key_down) {
        return &this->_keyboard_key;
    }
    return nullptr;
}

const KeyboardTextEvent* const
Event::keyboard_text() const
{
    if (this->type() == EventType::text_input) {
        return &this->_keyboard_text;
    }
    return nullptr;
}

const MouseButtonEvent* const
Event::mouse_button() const
{
    auto type = this->common.type();
    if (type == EventType::mouse_button_up or
        type == EventType::mouse_button_down) {
        return &this->_mouse_button;
    }
    return nullptr;
}

const MouseMotionEvent* const
Event::mouse_motion() const
{
    if (this->type() == EventType::mouse_motion) {
        return &this->_mouse_motion;
    }
    return nullptr;
}

const MouseWheelEvent* const
Event::mouse_wheel() const
{
    if (this->type() == EventType::mouse_wheel) {
        return &this->_mouse_wheel;
    }
    return nullptr;
}

const ControllerButtonEvent* const
Event::controller_button() const
{
    auto type = this->common.type();
    if (type == EventType::controller_button_up or
        type == EventType::controller_button_down) {
        return &this->_controller_button;
    }
    return nullptr;
}

const ControllerAxisEvent* const
Event::controller_axis() const
{
    if (this->type() == EventType::controller_axis_motion) {
        return &this->_controller_axis;
    }
    return nullptr;
}

bool
ControllerDeviceEvent::is_added() const
{
    return this->type() == EventType::controller_added;
}

bool
ControllerDeviceEvent::is_removed() const
{
    return this->type() == EventType::controller_removed;
}

const ControllerDeviceEvent* const
Event::controller_device() const
{
    auto type = this->type();
    if (type == EventType::controller_added or
        type == EventType::controller_removed) {
        return &this->_controller_device;
    }
    return nullptr;
}

const MusicFinishedEvent* const
Event::music_finished() const
{
    if (this->common.type() == EventType::music_finished) {
        return &this->_music_finished;
    }
    return nullptr;
}

bool InputManager::_custom_events_registered = false;

InputManager::InputManager()
{
    if (not this->_custom_events_registered) {
        auto num_events =
            static_cast<uint32_t>(EventType::_sentinel) - SDL_USEREVENT;
        auto first_event = SDL_RegisterEvents(num_events);
        KAACORE_CHECK_TERMINATE(
            first_event == SDL_USEREVENT, "Input system misconfigured."
        );
    }
    this->_custom_events_registered = true;

    this->register_callback(EventType::quit, _handle_quit);
}

std::string
InputManager::SystemManager::get_clipboard_text() const
{
    InputManager* input_manager = container_of(this, &InputManager::system);
    return get_engine()->make_call_from_main_thread<std::string>(
        [input_manager]() -> std::string {
            auto text = SDL_GetClipboardText();
            if (text == nullptr) {
                KAACORE_LOG_ERROR("Unable to read clipboard content.");
                return "";
            }
            return text;
        }
    );
}

void
InputManager::SystemManager::set_clipboard_text(const std::string& text) const
{
    InputManager* input_manager = container_of(this, &InputManager::system);
    return get_engine()->make_call_from_main_thread<void>([input_manager,
                                                           &text]() {
        if (SDL_SetClipboardText(text.c_str()) < 0) {
            KAACORE_LOG_ERROR("Unable to set clipboard content.");
        }
    });
}

bool
InputManager::KeyboardManager::is_pressed(const Keycode kc) const
{
    auto scancode = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(kc));
    return SDL_GetKeyboardState(NULL)[scancode] == 1;
}

bool
InputManager::KeyboardManager::is_released(const Keycode kc) const
{
    return not this->is_pressed(kc);
}

bool
InputManager::MouseManager::is_pressed(const MouseButton mb) const
{
    return SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(static_cast<uint8_t>(mb));
}

bool
InputManager::MouseManager::is_released(const MouseButton mb) const
{
    return not this->is_pressed(mb);
}

glm::dvec2
InputManager::MouseManager::get_position() const
{
    int pos_x, pos_y;
    SDL_GetMouseState(&pos_x, &pos_y);
    KAACORE_LOG_TRACE("Raw (SDL) mouse position: {}x{}", pos_x, pos_y);
    return _naive_screen_position_to_virtual_resolution(pos_x, pos_y);
}

bool
InputManager::MouseManager::cursor_visible() const
{
    InputManager* input_manager = container_of(this, &InputManager::mouse);
    return get_engine()->make_call_from_main_thread<bool>([input_manager]() {
        return SDL_ShowCursor(SDL_QUERY);
    });
}

void
InputManager::MouseManager::cursor_visible(const bool visible)
{
    InputManager* input_manager = container_of(this, &InputManager::mouse);
    return get_engine()->make_call_from_main_thread<void>(
        [input_manager, visible]() { return SDL_ShowCursor(visible); }
    );
}

bool
InputManager::MouseManager::relative_mode() const
{
    InputManager* input_manager = container_of(this, &InputManager::mouse);
    return get_engine()->make_call_from_main_thread<bool>([input_manager]() {
        return SDL_GetRelativeMouseMode();
    });
}

void
InputManager::MouseManager::relative_mode(const bool rel)
{
    InputManager* input_manager = container_of(this, &InputManager::mouse);
    get_engine()->make_call_from_main_thread<void>([input_manager, rel]() {
        if (SDL_SetRelativeMouseMode(static_cast<SDL_bool>(rel)) < 0) {
            KAACORE_LOG_ERROR(SDL_GetError());
        }
    });
}

InputManager::ControllerManager::~ControllerManager()
{
    for (auto& it : this->_connected_map) {
        SDL_GameControllerClose(it.second);
    }
}

bool
InputManager::ControllerManager::is_connected(const ControllerId id) const
{
    auto it = this->_connected_map.find(id);
    if (it == this->_connected_map.end()) {
        return false;
    }

    return SDL_GameControllerGetAttached(it->second);
}

bool
InputManager::ControllerManager::is_pressed(
    const ControllerButton cb, const ControllerId id
) const
{
    if (not this->is_connected(id)) {
        return false;
    }

    auto controller = this->_connected_map.at(id);
    return SDL_GameControllerGetButton(
        controller, static_cast<SDL_GameControllerButton>(cb)
    );
}

bool
InputManager::ControllerManager::is_released(
    const ControllerButton cb, const ControllerId id
) const
{
    return not this->is_pressed(cb, id);
}

bool
InputManager::ControllerManager::is_pressed(
    const ControllerAxis ca, const ControllerId id
) const
{
    return this->get_axis_motion(ca, id);
}

bool
InputManager::ControllerManager::is_released(
    const ControllerAxis ca, const ControllerId id
) const
{
    return not this->is_pressed(ca, id);
}

double
InputManager::ControllerManager::get_axis_motion(
    const ControllerAxis axis, const ControllerId id
) const
{
    if (not this->is_connected(id)) {
        return 0;
    }

    auto controller = this->_connected_map.at(id);
    return _normalize_controller_axis(SDL_GameControllerGetAxis(
        controller, static_cast<SDL_GameControllerAxis>(axis)
    ));
}

std::string
InputManager::ControllerManager::get_name(const ControllerId id) const
{
    if (not this->is_connected(id)) {
        return "";
    }
    auto name = SDL_GameControllerName(this->_connected_map.at(id));
    if (name == nullptr) {
        return "";
    }
    return name;
}

glm::dvec2
InputManager::ControllerManager::get_triggers(const ControllerId id) const
{
    return {
        this->get_axis_motion(ControllerAxis::trigger_left, id),
        this->get_axis_motion(ControllerAxis::trigger_right, id),
    };
}

glm::dvec2
InputManager::ControllerManager::get_sticks(
    const CompoundControllerAxis axis, const ControllerId id
) const
{
    if (axis == CompoundControllerAxis::left_stick) {
        return {
            this->get_axis_motion(ControllerAxis::left_x, id),
            this->get_axis_motion(ControllerAxis::left_y, id)
        };
    } else if (axis == CompoundControllerAxis::right_stick) {
        return {
            this->get_axis_motion(ControllerAxis::right_x, id),
            this->get_axis_motion(ControllerAxis::right_y, id)
        };
    }
    return {0, 0};
}

std::vector<ControllerId>
InputManager::ControllerManager::get_connected_controllers() const
{
    std::vector<ControllerId> result(this->_connected_map.size());
    for (auto& it : this->_connected_map) {
        result.push_back(it.first);
    }
    return result;
}

ControllerId
InputManager::ControllerManager::connect(int device_index)
{
    auto controller = SDL_GameControllerOpen(device_index);
    if (not controller) {
        KAACORE_LOG_ERROR(
            "Failed to connect game controller: {}", SDL_GetError()
        );
        return -1;
    }

    auto controller_id =
        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));

    this->_connected_map[controller_id] = controller;
    return controller_id;
}

void
InputManager::ControllerManager::disconnect(ControllerId id)
{
    SDL_GameControllerClose(this->_connected_map[id]);
    this->_connected_map.erase(id);
}

void
InputManager::register_callback(EventType event_type, EventCallback callback)
{
    if (callback != nullptr) {
        this->_registered_callbacks[event_type] = std::move(callback);
    } else {
        this->_registered_callbacks.erase(event_type);
    }
}

void
InputManager::push_event(SDL_Event sdl_event)
{
    auto type = _sdl_to_kaacore_type(sdl_event);
    if (not _is_event_supported(type)) {
        return;
    }

    switch (type) {
        case EventType::controller_added:
            sdl_event.cdevice.which =
                this->controller.connect(sdl_event.cdevice.which);
            KAACORE_LOG_DEBUG(
                "Controller connected ({}).", sdl_event.cdevice.which
            );
            break;

        case EventType::controller_removed:
            this->controller.disconnect(sdl_event.cdevice.which);
            KAACORE_LOG_DEBUG(
                "Controller disconnected ({}).", sdl_event.cdevice.which
            );
            break;
    }

    Event event(sdl_event);
    auto it = this->_registered_callbacks.find(type);
    if (it != this->_registered_callbacks.end()) {
        EventCallback& callback = it->second;
        if (not callback(event)) {
            return;
        }
    }

    this->events_queue.push_back(std::move(event));
}

void
InputManager::clear_events()
{
    this->events_queue.clear();
}

} // namespace kaacore

#include <cmath>
#include <limits>

#include <SDL.h>

#include "kaacore/audio.h"
#include "kaacore/engine.h"
#include "kaacore/log.h"

#include "kaacore/input.h"

namespace kaacore {

glm::dvec2
_naive_screen_position_to_virtual(int32_t x, int32_t y)
{
    auto engine = get_engine();
    glm::dvec2 pos = {x, y};
    pos -= engine->renderer->border_size;
    pos.x *= double(engine->virtual_resolution().x) /
             double(engine->renderer->view_size.x);
    pos.y *= double(engine->virtual_resolution().y) /
             double(engine->renderer->view_size.y);
    return pos;
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
SystemEvent::quit() const
{
    return this->type() == EventType::quit;
}

bool
SystemEvent::clipboard_updated() const
{
    return this->type() == EventType::clipboard_updated;
}

bool
WindowEvent::shown() const
{
    return this->type() == EventType::window_shown;
}

bool
WindowEvent::exposed() const
{
    return this->type() == EventType::window_exposed;
}

bool
WindowEvent::moved() const
{
    return this->type() == EventType::window_moved;
}

bool
WindowEvent::resized() const
{
    return this->type() == EventType::window_resized;
}

bool
WindowEvent::minimized() const
{
    return this->type() == EventType::window_minimized;
}

bool
WindowEvent::maximized() const
{
    return this->type() == EventType::window_maximized;
}

bool
WindowEvent::restored() const
{
    return this->type() == EventType::window_restored;
}

bool
WindowEvent::enter() const
{
    return this->type() == EventType::window_enter;
}

bool
WindowEvent::leave() const
{
    return this->type() == EventType::window_leave;
}

bool
WindowEvent::focus_gained() const
{
    return this->type() == EventType::window_focus_gained;
}

bool
WindowEvent::focus_lost() const
{
    return this->type() == EventType::window_focus_lost;
}

bool
WindowEvent::close() const
{
    return this->type() == EventType::window_close;
}

glm::dvec2
WindowEvent::size() const
{
    if (not this->resized()) {
        return {0, 0};
    }

    return {this->sdl_event.window.data1, this->sdl_event.window.data2};
}

glm::dvec2
WindowEvent::position() const
{
    if (not this->moved()) {
        return {0, 0};
    }

    return {this->sdl_event.window.data1, this->sdl_event.window.data2};
}

bool
KeyboardEvent::key() const
{
    auto type = this->type();
    return type == EventType::key_down or type == EventType::key_up;
}

bool
KeyboardEvent::text_input() const
{
    return this->type() == EventType::text_input;
}

bool
KeyboardEvent::is_pressing(Keycode kc) const
{
    return this->type() == EventType::key_down and
           this->sdl_event.key.keysym.sym == static_cast<SDL_Keycode>(kc);
}

bool
KeyboardEvent::is_releasing(Keycode kc) const
{
    return this->type() == EventType::key_up and
           this->sdl_event.key.keysym.sym == static_cast<SDL_Keycode>(kc);
}

std::string
KeyboardEvent::text() const
{
    if (not this->text_input()) {
        return "";
    }

    return this->sdl_event.text.text;
}

bool
MouseEvent::button() const
{
    auto type = this->type();
    return type == EventType::mouse_button_down or
           type == EventType::mouse_button_up;
}

bool
MouseEvent::motion() const
{
    return this->type() == EventType::mouse_motion;
}

bool
MouseEvent::wheel() const
{
    return this->type() == EventType::mouse_wheel;
}

bool
MouseEvent::is_pressing(const MouseButton mb) const
{
    return this->type() == EventType::mouse_button_down and
           this->sdl_event.button.button == static_cast<uint8_t>(mb);
}

bool
MouseEvent::is_releasing(const MouseButton mb) const
{
    return this->type() == EventType::mouse_button_up and
           this->sdl_event.button.button == static_cast<uint8_t>(mb);
}

glm::dvec2
MouseEvent::position() const
{
    if (this->button()) {
        return _naive_screen_position_to_virtual(
            this->sdl_event.button.x, this->sdl_event.button.y);
    } else if (this->motion()) {
        return _naive_screen_position_to_virtual(
            this->sdl_event.motion.x, this->sdl_event.motion.y);
    }

    return {0, 0};
}

glm::dvec2
MouseEvent::scroll() const
{
    if (not this->wheel()) {
        return {0, 0};
    }

    auto direction =
        glm::dvec2(this->sdl_event.wheel.x, this->sdl_event.wheel.y);
    // positive Y axis goes down
    direction *= -1;

    if (this->sdl_event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
        direction *= -1;
    }
    return direction;
}

bool
ControllerEvent::button() const
{
    auto type = this->type();
    return type == EventType::controller_button_down or
           type == EventType::controller_button_up;
}

bool
ControllerEvent::axis() const
{
    return this->type() == EventType::controller_axis_motion;
}

bool
ControllerEvent::added() const
{
    return this->type() == EventType::controller_added;
}

bool
ControllerEvent::removed() const
{
    return this->type() == EventType::controller_removed;
}

bool
ControllerEvent::remapped() const
{
    return this->type() == EventType::controller_removed;
}

bool
ControllerEvent::is_pressing(const ControllerButton cb) const
{
    if (not this->button()) {
        return false;
    }
    return this->sdl_event.cbutton.state == SDL_PRESSED and
           this->sdl_event.cbutton.button == static_cast<uint8_t>(cb);
}

bool
ControllerEvent::is_releasing(const ControllerButton cb) const
{
    if (not this->button()) {
        return false;
    }
    return this->sdl_event.cbutton.state == SDL_RELEASED and
           this->sdl_event.cbutton.button == static_cast<uint8_t>(cb);
}

double
ControllerEvent::axis_motion(const ControllerAxis ca) const
{
    if (not this->axis() or
        this->sdl_event.caxis.axis != static_cast<uint8_t>(ca)) {
        return 0;
    }
    return _normalize_controller_axis(this->sdl_event.caxis.value);
}

ControllerID
ControllerEvent::id() const
{
    return this->sdl_event.cdevice.which;
}

bool
AudioEvent::music_finished() const
{
    return this->type() == EventType::music_finished;
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

const KeyboardEvent* const
Event::keyboard() const
{
    auto type = this->type();
    if (type == EventType::key_up or type == EventType::key_down or
        type == EventType::text_input) {
        return &this->_keyboard;
    }
    return nullptr;
}

const MouseEvent* const
Event::mouse() const
{
    auto type = this->common.type();
    if (type == EventType::mouse_button_up or
        type == EventType::mouse_button_down or
        type == EventType::mouse_motion or type == EventType::mouse_wheel) {
        return &this->_mouse;
    }
    return nullptr;
}

const ControllerEvent* const
Event::controller() const
{
    auto type = this->common.type();
    if (type == EventType::controller_button_up or
        type == EventType::controller_button_down or
        type == EventType::controller_axis_motion or
        type == EventType::controller_added or
        type == EventType::controller_removed or
        type == EventType::controller_remapped) {
        return &this->_controller;
    }
    return nullptr;
}

const AudioEvent* const
Event::audio() const
{
    if (this->common.type() == EventType::music_finished) {
        return &this->_audio;
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
        KAACORE_CHECK_TERMINATE(first_event == SDL_USEREVENT);
    }
    this->_custom_events_registered = true;
}

std::string
InputManager::SystemManager::get_clipboard_text() const
{
    auto text = SDL_GetClipboardText();
    if (text == nullptr) {
        log<LogLevel::error>("Unable to read clipboard content.");
        return "";
    }
    return text;
}

void
InputManager::SystemManager::set_clipboard_text(const std::string& text) const
{
    if (SDL_SetClipboardText(text.c_str()) < 0) {
        log<LogLevel::error>("Unable to set clipboard content.");
    }
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
    return _naive_screen_position_to_virtual(pos_x, pos_y);
}

InputManager::ControllerManager::~ControllerManager()
{
    for (auto& it : this->_connected_map) {
        this->disconnect(it.first);
    }
}

bool
InputManager::ControllerManager::is_connected(const ControllerID id) const
{
    auto it = this->_connected_map.find(id);
    if (it == this->_connected_map.end()) {
        return false;
    }

    return SDL_GameControllerGetAttached(it->second);
}

bool
InputManager::ControllerManager::is_pressed(
    const ControllerButton cb, const ControllerID id) const
{
    if (not this->is_connected(id)) {
        return false;
    }

    auto controller = this->_connected_map.at(id);
    return SDL_GameControllerGetButton(
        controller, static_cast<SDL_GameControllerButton>(cb));
}

bool
InputManager::ControllerManager::is_released(
    const ControllerButton cb, const ControllerID id) const
{
    return not this->is_pressed(cb, id);
}

bool
InputManager::ControllerManager::is_pressed(
    const ControllerAxis ca, const ControllerID id) const
{
    return this->get_axis_motion(ca, id);
}

bool
InputManager::ControllerManager::is_released(
    const ControllerAxis ca, const ControllerID id) const
{
    return not this->is_pressed(ca, id);
}

double
InputManager::ControllerManager::get_axis_motion(
    const ControllerAxis axis, const ControllerID id) const
{
    if (not this->is_connected(id)) {
        return 0;
    }

    auto controller = this->_connected_map.at(id);
    return _normalize_controller_axis(SDL_GameControllerGetAxis(
        controller, static_cast<SDL_GameControllerAxis>(axis)));
}

std::string
InputManager::ControllerManager::get_name(const ControllerID id) const
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
InputManager::ControllerManager::get_triggers(const ControllerID id) const
{
    return {
        this->get_axis_motion(ControllerAxis::trigger_left, id),
        this->get_axis_motion(ControllerAxis::trigger_right, id),
    };
}

glm::dvec2
InputManager::ControllerManager::get_sticks(
    const CompoundControllerAxis axis, const ControllerID id) const
{
    if (axis == CompoundControllerAxis::left_stick) {
        return {this->get_axis_motion(ControllerAxis::left_x, id),
                this->get_axis_motion(ControllerAxis::left_y, id)};
    } else if (axis == CompoundControllerAxis::right_stick) {
        return {this->get_axis_motion(ControllerAxis::right_x, id),
                this->get_axis_motion(ControllerAxis::right_y, id)};
    }
    return {0, 0};
}

std::vector<ControllerID>
InputManager::ControllerManager::get_connected_controllers() const
{
    std::vector<ControllerID> result(this->_connected_map.size());
    for (auto& it : this->_connected_map) {
        result.push_back(it.first);
    }
    return result;
}

ControllerID
InputManager::ControllerManager::connect(int device_index)
{
    auto controller = SDL_GameControllerOpen(device_index);
    if (not controller) {
        log<LogLevel::error>(
            "Failed to connect game controller: %s\n", SDL_GetError());
        return -1;
    }

    auto controller_id =
        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));

    this->_connected_map[controller_id] = controller;
    return controller_id;
}

void
InputManager::ControllerManager::disconnect(ControllerID id)
{
    SDL_GameControllerClose(this->_connected_map[id]);
    this->_connected_map.erase(id);
}

void
InputManager::register_callback(EventType event_type, EventCallback callback)
{
    this->_registered_callbacks[event_type] = callback;
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
            log<LogLevel::debug>("Controller conneced.");
            break;

        case EventType::controller_removed:
            this->controller.disconnect(sdl_event.cdevice.which);
            log<LogLevel::debug>("Controller disconnected.");
            break;
    }

    Event event(sdl_event);
    auto it = this->_registered_callbacks.find(event.type());
    if (it != this->_registered_callbacks.end()) {
        auto callback = it->second;
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

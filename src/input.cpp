#include <SDL.h>

#include "kaacore/engine.h"

#include "kaacore/input.h"


namespace kaacore {

glm::dvec2 _naive_screen_position_to_virtual(int32_t x, int32_t y)
{
    auto engine = get_engine();
    glm::dvec2 pos = {x, y};
    pos -= engine->renderer->border_size;
    pos.x *= double(engine->virtual_resolution().x) / double(engine->renderer->view_size.x);
    pos.y *= double(engine->virtual_resolution().y) / double(engine->renderer->view_size.y);
    return pos;
}

Event::Event() : sdl_event() {}

Event::Event(SDL_Event sdl_event) : sdl_event(sdl_event) {}

bool Event::is_quit() const
{
    return this->sdl_event.type == SDL_QUIT;
}

bool Event::is_keyboard_event() const
{
    return (this->sdl_event.type == SDL_KEYUP or
            this->sdl_event.type == SDL_KEYDOWN);
}

bool Event::is_mouse_event() const
{
    return (this->sdl_event.type == SDL_MOUSEBUTTONUP or
            this->sdl_event.type == SDL_MOUSEBUTTONDOWN);
}

bool Event::is_pressing(Keycode kc) const
{
    return (this->sdl_event.type == SDL_KEYDOWN and
            this->sdl_event.key.keysym.sym == static_cast<SDL_Keycode>(kc));
}

bool Event::is_pressing(Mousecode mc) const
{
    return (this->sdl_event.type == SDL_MOUSEBUTTONDOWN and
            this->sdl_event.button.button == static_cast<uint8_t>(mc));
}

bool Event::is_releasing(Keycode kc) const
{
    return (this->sdl_event.type == SDL_KEYUP and
            this->sdl_event.key.keysym.sym == static_cast<SDL_Keycode>(kc));
}

bool Event::is_releasing(Mousecode mc) const
{
    return (this->sdl_event.type == SDL_MOUSEBUTTONUP and
            this->sdl_event.button.button == static_cast<uint8_t>(mc));
}

glm::dvec2 Event::get_mouse_position() const
{
    return _naive_screen_position_to_virtual(
        this->sdl_event.button.x, this->sdl_event.button.y
    );
}


void InputManager::push_event(SDL_Event sdl_event)
{
    this->events_queue.emplace_back(sdl_event);
}

void InputManager::clear_events()
{
    this->events_queue.clear();
}

bool InputManager::is_pressed(Keycode kc) const
{
    auto scancode = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(kc));
    return SDL_GetKeyboardState(NULL)[scancode] == 1;
}

bool InputManager::is_pressed(Mousecode mc) const
{
    return (SDL_GetMouseState(NULL, NULL) &
            SDL_BUTTON(static_cast<uint8_t>(mc)));
}

bool InputManager::is_released(Keycode kc) const
{
    auto scancode = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(kc));
    return SDL_GetKeyboardState(NULL)[scancode] == 0;
}

bool InputManager::is_released(Mousecode mc) const
{
    return not (SDL_GetMouseState(NULL, NULL) &
                SDL_BUTTON(static_cast<uint8_t>(mc)));
}

glm::dvec2 InputManager::get_mouse_position() const
{
    int pos_x, pos_y;
    SDL_GetMouseState(&pos_x, &pos_y);
    return _naive_screen_position_to_virtual(pos_x, pos_y);
}

} // namespace kaacore

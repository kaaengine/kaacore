#include <SDL.h>

#include "kaacore/input.h"



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
    return (this->sdl_event.type == SDL_MOUSEBUTTONDOWN or
            this->sdl_event.button.button == static_cast<uint8_t>(mc));
}

bool Event::is_releasing(Keycode kc) const
{
    return (this->sdl_event.type == SDL_KEYUP and
            this->sdl_event.key.keysym.sym == static_cast<SDL_Keycode>(kc));
}

bool Event::is_releasing(Mousecode mc) const
{
    return (this->sdl_event.type == SDL_MOUSEBUTTONUP or
            this->sdl_event.button.button == static_cast<uint8_t>(mc));
}


void InputManager::push_event(SDL_Event sdl_event)
{
    this->events_queue.emplace_back(sdl_event);
}

void InputManager::clear_events()
{
    this->events_queue.clear();
}

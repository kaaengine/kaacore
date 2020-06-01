#pragma once

#include <functional>
#include <initializer_list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL.h>
#include <glm/glm.hpp>

#include "kaacore/audio.h"
#include "kaacore/threading.h"

namespace kaacore {

typedef SDL_JoystickID ControllerID;
struct Event;
typedef std::function<bool(const Event&)> EventCallback;

enum class Keycode {
    unknown = SDLK_UNKNOWN,
    return_ = SDLK_RETURN,
    escape = SDLK_ESCAPE,
    backspace = SDLK_BACKSPACE,
    tab = SDLK_TAB,
    space = SDLK_SPACE,
    exclaim = SDLK_EXCLAIM,
    quotedbl = SDLK_QUOTEDBL,
    hash = SDLK_HASH,
    percent = SDLK_PERCENT,
    dollar = SDLK_DOLLAR,
    ampersand = SDLK_AMPERSAND,
    quote = SDLK_QUOTE,
    leftparen = SDLK_LEFTPAREN,
    rightparen = SDLK_RIGHTPAREN,
    asterisk = SDLK_ASTERISK,
    plus = SDLK_PLUS,
    comma = SDLK_COMMA,
    minus = SDLK_MINUS,
    period = SDLK_PERIOD,
    slash = SDLK_SLASH,
    num_0 = SDLK_0,
    num_1 = SDLK_1,
    num_2 = SDLK_2,
    num_3 = SDLK_3,
    num_4 = SDLK_4,
    num_5 = SDLK_5,
    num_6 = SDLK_6,
    num_7 = SDLK_7,
    num_8 = SDLK_8,
    num_9 = SDLK_9,
    colon = SDLK_COLON,
    semicolon = SDLK_SEMICOLON,
    less = SDLK_LESS,
    equals = SDLK_EQUALS,
    greater = SDLK_GREATER,
    question = SDLK_QUESTION,
    at = SDLK_AT,
    leftbracket = SDLK_LEFTBRACKET,
    backslash = SDLK_BACKSLASH,
    rightbracket = SDLK_RIGHTBRACKET,
    caret = SDLK_CARET,
    underscore = SDLK_UNDERSCORE,
    backquote = SDLK_BACKQUOTE,
    a = SDLK_a,
    b = SDLK_b,
    c = SDLK_c,
    d = SDLK_d,
    e = SDLK_e,
    f = SDLK_f,
    g = SDLK_g,
    h = SDLK_h,
    i = SDLK_i,
    j = SDLK_j,
    k = SDLK_k,
    l = SDLK_l,
    m = SDLK_m,
    n = SDLK_n,
    o = SDLK_o,
    p = SDLK_p,
    q = SDLK_q,
    r = SDLK_r,
    s = SDLK_s,
    t = SDLK_t,
    u = SDLK_u,
    v = SDLK_v,
    w = SDLK_w,
    x = SDLK_x,
    y = SDLK_y,
    z = SDLK_z,
    A = SDLK_a,
    B = SDLK_b,
    C = SDLK_c,
    D = SDLK_d,
    E = SDLK_e,
    F = SDLK_f,
    G = SDLK_g,
    H = SDLK_h,
    I = SDLK_i,
    J = SDLK_j,
    K = SDLK_k,
    L = SDLK_l,
    M = SDLK_m,
    N = SDLK_n,
    O = SDLK_o,
    P = SDLK_p,
    Q = SDLK_q,
    R = SDLK_r,
    S = SDLK_s,
    T = SDLK_t,
    U = SDLK_u,
    V = SDLK_v,
    W = SDLK_w,
    X = SDLK_x,
    Y = SDLK_y,
    Z = SDLK_z,
    capslock = SDLK_CAPSLOCK,
    F1 = SDLK_F1,
    F2 = SDLK_F2,
    F3 = SDLK_F3,
    F4 = SDLK_F4,
    F5 = SDLK_F5,
    F6 = SDLK_F6,
    F7 = SDLK_F7,
    F8 = SDLK_F8,
    F9 = SDLK_F9,
    F10 = SDLK_F10,
    F11 = SDLK_F11,
    F12 = SDLK_F12,
    printscreen = SDLK_PRINTSCREEN,
    scrolllock = SDLK_SCROLLLOCK,
    pause = SDLK_PAUSE,
    insert = SDLK_INSERT,
    home = SDLK_HOME,
    pageup = SDLK_PAGEUP,
    delete_ = SDLK_DELETE,
    end = SDLK_END,
    pagedown = SDLK_PAGEDOWN,
    right = SDLK_RIGHT,
    left = SDLK_LEFT,
    down = SDLK_DOWN,
    up = SDLK_UP,
    numlockclear = SDLK_NUMLOCKCLEAR,
    kp_divide = SDLK_KP_DIVIDE,
    kp_multiply = SDLK_KP_MULTIPLY,
    kp_minus = SDLK_KP_MINUS,
    kp_plus = SDLK_KP_PLUS,
    kp_enter = SDLK_KP_ENTER,
    kp_1 = SDLK_KP_1,
    kp_2 = SDLK_KP_2,
    kp_3 = SDLK_KP_3,
    kp_4 = SDLK_KP_4,
    kp_5 = SDLK_KP_5,
    kp_6 = SDLK_KP_6,
    kp_7 = SDLK_KP_7,
    kp_8 = SDLK_KP_8,
    kp_9 = SDLK_KP_9,
    kp_0 = SDLK_KP_0,
    kp_period = SDLK_KP_PERIOD,
    application = SDLK_APPLICATION,
    power = SDLK_POWER,
    kp_equals = SDLK_KP_EQUALS,
    F13 = SDLK_F13,
    F14 = SDLK_F14,
    F15 = SDLK_F15,
    F16 = SDLK_F16,
    F17 = SDLK_F17,
    F18 = SDLK_F18,
    F19 = SDLK_F19,
    F20 = SDLK_F20,
    F21 = SDLK_F21,
    F22 = SDLK_F22,
    F23 = SDLK_F23,
    F24 = SDLK_F24,
    execute = SDLK_EXECUTE,
    help = SDLK_HELP,
    menu = SDLK_MENU,
    select = SDLK_SELECT,
    stop = SDLK_STOP,
    again = SDLK_AGAIN,
    undo = SDLK_UNDO,
    cut = SDLK_CUT,
    copy = SDLK_COPY,
    paste = SDLK_PASTE,
    find = SDLK_FIND,
    mute = SDLK_MUTE,
    volumeup = SDLK_VOLUMEUP,
    volumedown = SDLK_VOLUMEDOWN,
    kp_comma = SDLK_KP_COMMA,
    kp_equalsas400 = SDLK_KP_EQUALSAS400,
    alterase = SDLK_ALTERASE,
    sysreq = SDLK_SYSREQ,
    cancel = SDLK_CANCEL,
    clear = SDLK_CLEAR,
    prior = SDLK_PRIOR,
    return2 = SDLK_RETURN2,
    separator = SDLK_SEPARATOR,
    out = SDLK_OUT,
    oper = SDLK_OPER,
    clearagain = SDLK_CLEARAGAIN,
    crsel = SDLK_CRSEL,
    exsel = SDLK_EXSEL,
    kp_00 = SDLK_KP_00,
    kp_000 = SDLK_KP_000,
    thousandsseparator = SDLK_THOUSANDSSEPARATOR,
    decimalseparator = SDLK_DECIMALSEPARATOR,
    currencyunit = SDLK_CURRENCYUNIT,
    currencysubunit = SDLK_CURRENCYSUBUNIT,
    kp_leftparen = SDLK_KP_LEFTPAREN,
    kp_rightparen = SDLK_KP_RIGHTPAREN,
    kp_leftbrace = SDLK_KP_LEFTBRACE,
    kp_rightbrace = SDLK_KP_RIGHTBRACE,
    kp_tab = SDLK_KP_TAB,
    kp_backspace = SDLK_KP_BACKSPACE,
    kp_a = SDLK_KP_A,
    kp_b = SDLK_KP_B,
    kp_c = SDLK_KP_C,
    kp_d = SDLK_KP_D,
    kp_e = SDLK_KP_E,
    kp_f = SDLK_KP_F,
    kp_xor = SDLK_KP_XOR,
    kp_power = SDLK_KP_POWER,
    kp_percent = SDLK_KP_PERCENT,
    kp_less = SDLK_KP_LESS,
    kp_greater = SDLK_KP_GREATER,
    kp_ampersand = SDLK_KP_AMPERSAND,
    kp_dblampersand = SDLK_KP_DBLAMPERSAND,
    kp_verticalbar = SDLK_KP_VERTICALBAR,
    kp_dblverticalbar = SDLK_KP_DBLVERTICALBAR,
    kp_colon = SDLK_KP_COLON,
    kp_hash = SDLK_KP_HASH,
    kp_space = SDLK_KP_SPACE,
    kp_at = SDLK_KP_AT,
    kp_exclam = SDLK_KP_EXCLAM,
    kp_memstore = SDLK_KP_MEMSTORE,
    kp_memrecall = SDLK_KP_MEMRECALL,
    kp_memclear = SDLK_KP_MEMCLEAR,
    kp_memadd = SDLK_KP_MEMADD,
    kp_memsubtract = SDLK_KP_MEMSUBTRACT,
    kp_memmultiply = SDLK_KP_MEMMULTIPLY,
    kp_memdivide = SDLK_KP_MEMDIVIDE,
    kp_plusminus = SDLK_KP_PLUSMINUS,
    kp_clear = SDLK_KP_CLEAR,
    kp_clearentry = SDLK_KP_CLEARENTRY,
    kp_binary = SDLK_KP_BINARY,
    kp_octal = SDLK_KP_OCTAL,
    kp_decimal = SDLK_KP_DECIMAL,
    kp_hexadecimal = SDLK_KP_HEXADECIMAL,
    lctrl = SDLK_LCTRL,
    lshift = SDLK_LSHIFT,
    lalt = SDLK_LALT,
    lgui = SDLK_LGUI,
    rctrl = SDLK_RCTRL,
    rshift = SDLK_RSHIFT,
    ralt = SDLK_RALT,
    rgui = SDLK_RGUI,
    mode = SDLK_MODE,
    audionext = SDLK_AUDIONEXT,
    audioprev = SDLK_AUDIOPREV,
    audiostop = SDLK_AUDIOSTOP,
    audioplay = SDLK_AUDIOPLAY,
    audiomute = SDLK_AUDIOMUTE,
    mediaselect = SDLK_MEDIASELECT,
    www = SDLK_WWW,
    mail = SDLK_MAIL,
    calculator = SDLK_CALCULATOR,
    computer = SDLK_COMPUTER,
    ac_search = SDLK_AC_SEARCH,
    ac_home = SDLK_AC_HOME,
    ac_back = SDLK_AC_BACK,
    ac_forward = SDLK_AC_FORWARD,
    ac_stop = SDLK_AC_STOP,
    ac_refresh = SDLK_AC_REFRESH,
    ac_bookmarks = SDLK_AC_BOOKMARKS,
    brightnessdown = SDLK_BRIGHTNESSDOWN,
    brightnessup = SDLK_BRIGHTNESSUP,
    displayswitch = SDLK_DISPLAYSWITCH,
    kbdillumtoggle = SDLK_KBDILLUMTOGGLE,
    kbdillumdown = SDLK_KBDILLUMDOWN,
    kbdillumup = SDLK_KBDILLUMUP,
    eject = SDLK_EJECT,
    sleep = SDLK_SLEEP,
};

enum class MouseButton {
    left = SDL_BUTTON_LEFT,
    middle = SDL_BUTTON_MIDDLE,
    right = SDL_BUTTON_RIGHT,
    x1 = SDL_BUTTON_X1,
    x2 = SDL_BUTTON_X2
};

enum class ControllerButton {
    a = SDL_CONTROLLER_BUTTON_A,
    b = SDL_CONTROLLER_BUTTON_B,
    x = SDL_CONTROLLER_BUTTON_X,
    y = SDL_CONTROLLER_BUTTON_Y,
    back = SDL_CONTROLLER_BUTTON_BACK,
    guide = SDL_CONTROLLER_BUTTON_GUIDE,
    start = SDL_CONTROLLER_BUTTON_START,
    left_stick = SDL_CONTROLLER_BUTTON_LEFTSTICK,
    right_stick = SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    left_shoulder = SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    right_shoulder = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    dpad_up = SDL_CONTROLLER_BUTTON_DPAD_UP,
    dpad_down = SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    dpad_left = SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    dpad_right = SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
};

enum class ControllerAxis {
    left_x = SDL_CONTROLLER_AXIS_LEFTX,
    left_y = SDL_CONTROLLER_AXIS_LEFTY,
    right_x = SDL_CONTROLLER_AXIS_RIGHTX,
    right_y = SDL_CONTROLLER_AXIS_RIGHTY,
    trigger_left = SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    trigger_right = SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
};

enum class EventType {
    // Public SDL events
    quit = SDL_QUIT,
    clipboard_updated = SDL_CLIPBOARDUPDATE,

    window_shown = SDL_WINDOWEVENT_SHOWN,
    window_hidden = SDL_WINDOWEVENT_HIDDEN,
    window_exposed = SDL_WINDOWEVENT_EXPOSED,
    window_moved = SDL_WINDOWEVENT_MOVED,
    window_resized = SDL_WINDOWEVENT_RESIZED,
    window_minimized = SDL_WINDOWEVENT_MINIMIZED,
    window_maximized = SDL_WINDOWEVENT_MAXIMIZED,
    window_restored = SDL_WINDOWEVENT_RESTORED,
    window_enter = SDL_WINDOWEVENT_ENTER,
    window_leave = SDL_WINDOWEVENT_LEAVE,
    window_focus_gained = SDL_WINDOWEVENT_FOCUS_GAINED,
    window_focus_lost = SDL_WINDOWEVENT_FOCUS_LOST,
    window_close = SDL_WINDOWEVENT_CLOSE,

    key_down = SDL_KEYDOWN,
    key_up = SDL_KEYUP,
    text_input = SDL_TEXTINPUT,

    mouse_motion = SDL_MOUSEMOTION,
    mouse_button_down = SDL_MOUSEBUTTONDOWN,
    mouse_button_up = SDL_MOUSEBUTTONUP,
    mouse_wheel = SDL_MOUSEWHEEL,

    controller_axis_motion = SDL_CONTROLLERAXISMOTION,
    controller_button_down = SDL_CONTROLLERBUTTONDOWN,
    controller_button_up = SDL_CONTROLLERBUTTONUP,
    controller_added = SDL_CONTROLLERDEVICEADDED,
    controller_removed = SDL_CONTROLLERDEVICEREMOVED,
    controller_remapped = SDL_CONTROLLERDEVICEREMAPPED,

    // Public custom events
    music_finished = SDL_USEREVENT,
    channel_finished,

    // Private custom events
    _timer_fired,
    _sentinel,
};

bool
operator==(const EventType& event_type, const uint32_t& event_num);
bool
operator==(const uint32_t& event_num, const EventType& event_type);

enum class CompoundEventType {
    window = SDL_WINDOWEVENT,
    system = 0,
    keyboard,
    mouse,
    controller
};

enum class CompoundControllerAxis { left_stick, right_stick };

static inline bool
_is_event_supported(EventType type)
{
    if (type == EventType::quit or type == EventType::clipboard_updated or
        type == EventType::window_shown or type == EventType::window_hidden or
        type == EventType::window_exposed or type == EventType::window_moved or
        type == EventType::window_resized or
        type == EventType::window_minimized or
        type == EventType::window_maximized or
        type == EventType::window_restored or type == EventType::window_enter or
        type == EventType::window_leave or
        type == EventType::window_focus_gained or
        type == EventType::window_focus_lost or
        type == EventType::window_close or type == EventType::key_down or
        type == EventType::key_up or type == EventType::text_input or
        type == EventType::mouse_motion or
        type == EventType::mouse_button_down or
        type == EventType::mouse_button_up or type == EventType::mouse_wheel or
        type == EventType::controller_axis_motion or
        type == EventType::controller_button_down or
        type == EventType::controller_button_up or
        type == EventType::controller_added or
        type == EventType::controller_removed or
        type == EventType::controller_remapped or
        type == EventType::music_finished) {
        return true;
    }
    return false;
}

struct BaseEvent {
    SDL_Event sdl_event;

    EventType type() const;
    uint32_t timestamp() const;
};

struct SystemEvent : public BaseEvent {
    bool is_quit() const;
    bool is_clipboard_updated() const;
};

struct WindowEvent : public BaseEvent {
    bool is_shown() const;
    bool is_exposed() const;
    bool is_moved() const;
    bool is_resized() const;
    bool is_minimized() const;
    bool is_maximized() const;
    bool is_restored() const;
    bool is_enter() const;
    bool is_leave() const;
    bool is_focus_gained() const;
    bool is_focus_lost() const;
    bool is_close() const;
};

struct KeyboardKeyEvent : public BaseEvent {
    Keycode key() const;
    bool is_key_down() const;
    bool is_key_up() const;
    bool repeat() const;
};

struct KeyboardTextEvent : public BaseEvent {
    std::string text() const;
};

struct MouseButtonEvent : public BaseEvent {
    MouseButton button() const;
    bool is_button_down() const;
    bool is_button_up() const;
    glm::dvec2 position() const;
};

struct MouseMotionEvent : public BaseEvent {
    glm::dvec2 position() const;
    glm::dvec2 motion() const;
};

struct MouseWheelEvent : public BaseEvent {
    glm::dvec2 scroll() const;
};

struct BaseControllerEvent : public BaseEvent {
    ControllerID id() const;
};

struct ControllerButtonEvent : public BaseControllerEvent {
    ControllerButton button() const;
    bool is_button_down() const;
    bool is_button_up() const;
};

struct ControllerAxisEvent : public BaseControllerEvent {
    ControllerAxis axis() const;
    double motion() const;
};

struct ControllerDeviceEvent : public BaseControllerEvent {
    bool is_added() const;
    bool is_removed() const;
};

struct MusicFinishedEvent : public BaseEvent {};

struct Event {
    union {
        BaseEvent common;

        SystemEvent _system;
        WindowEvent _window;
        KeyboardKeyEvent _keyboard_key;
        KeyboardTextEvent _keyboard_text;
        MouseButtonEvent _mouse_button;
        MouseMotionEvent _mouse_motion;
        MouseWheelEvent _mouse_wheel;
        ControllerButtonEvent _controller_button;
        ControllerAxisEvent _controller_axis;
        ControllerDeviceEvent _controller_device;
        MusicFinishedEvent _music_finished;
    };

    Event();
    Event(const SDL_Event sdl_event);

    EventType type() const;
    uint32_t timestamp() const;

    const SystemEvent* const system() const;
    const WindowEvent* const window() const;
    const KeyboardKeyEvent* const keyboard_key() const;
    const KeyboardTextEvent* const keyboard_text() const;
    const MouseButtonEvent* const mouse_button() const;
    const MouseMotionEvent* const mouse_motion() const;
    const MouseWheelEvent* const mouse_wheel() const;
    const ControllerButtonEvent* const controller_button() const;
    const ControllerAxisEvent* const controller_axis() const;
    const ControllerDeviceEvent* const controller_device() const;
    const MusicFinishedEvent* const music_finished() const;
};

struct InputManager {
    std::vector<Event> events_queue;
    static bool _custom_events_registered;

    InputManager(std::mutex& _sdl_windowing_call_mutex);

    struct SystemManager {
        std::string get_clipboard_text() const;
        void set_clipboard_text(const std::string& text) const;
    } system;

    struct KeyboardManager {
        bool is_pressed(const Keycode kc) const;
        bool is_released(const Keycode kc) const;
    } keyboard;

    struct MouseManager {
        bool is_pressed(const MouseButton mb) const;
        bool is_released(const MouseButton mb) const;
        glm::dvec2 get_position() const;

        bool relative_mode() const;
        void relative_mode(const bool rel);
    } mouse;

    struct ControllerManager {
        ~ControllerManager();
        bool is_connected(const ControllerID id) const;
        bool is_pressed(const ControllerButton cb, const ControllerID id) const;
        bool is_released(
            const ControllerButton cb, const ControllerID id) const;
        bool is_pressed(const ControllerAxis ca, const ControllerID id) const;
        bool is_released(const ControllerAxis ca, const ControllerID id) const;
        double get_axis_motion(
            const ControllerAxis axis, const ControllerID id) const;
        std::string get_name(const ControllerID id) const;
        glm::dvec2 get_triggers(const ControllerID id) const;
        glm::dvec2 get_sticks(
            const CompoundControllerAxis axis, const ControllerID id) const;
        std::vector<ControllerID> get_connected_controllers() const;

        ControllerID connect(int device_index);
        void disconnect(ControllerID id);

      private:
        std::unordered_map<ControllerID, SDL_GameController*> _connected_map;
    } controller;

    void register_callback(EventType event_type, EventCallback callback);
    void push_event(SDL_Event sdl_event);
    void clear_events();

  private:
    void _thread_safe_call(DelayedSyscallFunction&& func);

    std::unordered_map<EventType, EventCallback> _registered_callbacks;
    std::mutex& _sdl_windowing_call_mutex;

    friend struct MouseManager;
};

} // namespace kaacore

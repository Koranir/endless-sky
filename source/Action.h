#ifndef ES_ACTION_H_
#define ES_ACTION_H_

#include "DataNode.h"
#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <variant>

namespace ActionKind {
    template<class... Ts>
    struct Match : Ts... { using Ts::operator()...; };
    // explicit deduction guide (not needed as of C++20)
    template<class... Ts>
    Match(Ts...) -> Match<Ts...>;
    

    template<typename T>
    class _AutoDeriveComparators : public std::tuple<T> {
    public:
        const T &get() const {
            return std::get<0>(*this);
        }
    
    };

    // The keycode of a keyboard event
    class Keyboard : public _AutoDeriveComparators<SDL_Keycode> {};
    // The button number of a mouse button event, accessible by SDL_BUTTON(x)
    // and the SDL_BUTTON_LEFT and family of defines.
    class MouseButton : public _AutoDeriveComparators<int> {};
    // The amount scrolled in a mouse wheel event. First is x, second is y.
    class MouseScroll : public _AutoDeriveComparators<std::pair<int32_t, int32_t>> {};
    // The button that was pressed of the game controller
    class ControllerButton : public _AutoDeriveComparators<SDL_GameControllerButton> {};
    // The axis of the controller, as well as the value itself, normalized between -1 -> 1
    class ControllerAxis : public _AutoDeriveComparators<std::pair<SDL_GameControllerAxis, double>> {};

    class None : public std::tuple<> {};
}

class Action : public std::variant<
    ActionKind::MouseButton,
    ActionKind::MouseScroll,
    ActionKind::Keyboard,
    ActionKind::ControllerButton,
    ActionKind::ControllerAxis,
    ActionKind::None
> {
public:
    enum Direction {
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

public:
    static std::optional<Action> FromEvent(const SDL_Event &event);
    static Action FromString(const std::string &str);

    std::optional<Direction> AsDirectional() const;

    std::string DisplayName() const;

};

#endif

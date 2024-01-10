#include "Action.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Input.h"
#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>

using namespace std;

namespace Kind = ActionKind;

namespace {}

optional<Action> Action::FromEvent(const SDL_Event &event) {
    switch (event.type) {
    case SDL_KEYDOWN:
        return {{Kind::Keyboard{event.key.keysym.sym}}};
    case SDL_MOUSEBUTTONDOWN:
        return {{Kind::MouseButton{event.button.button}}};
    case SDL_CONTROLLERBUTTONDOWN:
        return {{Kind::MouseButton{event.cbutton.button}}};
    case SDL_CONTROLLERAXISMOTION:
    {
        auto val = static_cast<double>(event.caxis.value) / INT16_MAX;
        return (abs(val) > Input::Deadzone())
            ? optional<Action>({Kind::ControllerAxis{make_pair(static_cast<SDL_GameControllerAxis>(event.caxis.axis), val)}})
            : nullopt;
    }
    };

    return nullopt;
}

optional<Action::Direction> Action::AsDirectional() const {
    return visit(ActionKind::Match{
        [](Kind::Keyboard key){
            switch(key.get()) {
            case SDLK_DOWN:
                return optional(Direction::DOWN);
            case SDLK_UP:
                return optional(Direction::UP);
            case SDLK_LEFT:
                return optional(Direction::LEFT);
            case SDLK_RIGHT:
                return optional(Direction::RIGHT);
            }

            return optional<Direction>();
        },
        [](Kind::MouseButton button){
            return optional<Direction>();
        },
        [](Kind::MouseScroll scroll) {
            auto [x, y] = scroll.get();
            if(x > 0) {
                return optional(Direction::RIGHT);
            } else if(x < 0) {
                return optional(Direction::LEFT);
            } else if(y > 0) {
                return optional(Direction::UP);
            } else if(y < 0) {
                return optional(Direction::DOWN);
            }

            return optional<Direction>();
        },
        [](Kind::ControllerButton button){
            switch(button.get()) {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                return optional(Direction::UP);
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                return optional(Direction::DOWN);
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                return optional(Direction::LEFT);
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                return optional(Direction::RIGHT);
            default:
                break;
            }

            return optional<Direction>();
        },
        [](Kind::ControllerAxis){
            // Controller axis are a bit iffy, it's better to handle them on a per-use basis.
            return optional<Direction>();
        },
        [&](ActionKind::None){
            return optional<Direction>();
        }
    }, *this);
}

string Action::DisplayName() const {
    return visit(ActionKind::Match{
        [](Kind::Keyboard key){
            return string(SDL_GetKeyName(key.get()));
        },
        [](Kind::MouseButton button){
            return string("Mouse " + to_string(button.get()));
        },
        [this](Kind::MouseScroll scroll) {
            auto direction = AsDirectional();
            if(!direction) {
                return string("Scroll Null");
            }

            switch(direction.value()) {
            case LEFT:
                return string("Scroll -X");
            case RIGHT:
                return string("Scroll +X");
            case UP:
                return string("Scroll +Y");
            case DOWN:
                return string("Scroll -Y");
            }

            return string("?Scroll");
        },
        [](Kind::ControllerButton button){
            return string(SDL_GameControllerGetStringForButton(button.get()));
        },
        [](Kind::ControllerAxis axis_){
            auto &[axis, val] = axis_.get();

            if(val > 0.) {
                switch(axis) {
                case SDL_CONTROLLER_AXIS_INVALID:
                    return string("?Axis+");
                case SDL_CONTROLLER_AXIS_LEFTX:
                    return string("LStick X+");
                case SDL_CONTROLLER_AXIS_LEFTY:
                    return string("LStick Y+");
                case SDL_CONTROLLER_AXIS_RIGHTX:
                    return string("RStick X+");
                case SDL_CONTROLLER_AXIS_RIGHTY:
                    return string("RStick Y+");
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                    return string("Left Trigger");
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                    return string("Right Trigger");
                case SDL_CONTROLLER_AXIS_MAX:
                    return string("?Axis+");
                }
            } else {
                switch(axis) {
                case SDL_CONTROLLER_AXIS_INVALID:
                    return string("?Axis-");
                case SDL_CONTROLLER_AXIS_LEFTX:
                    return string("LStick X-");
                case SDL_CONTROLLER_AXIS_LEFTY:
                    return string("LStick Y-");
                case SDL_CONTROLLER_AXIS_RIGHTX:
                    return string("RStick X-");
                case SDL_CONTROLLER_AXIS_RIGHTY:
                    return string("RStick Y-");
                case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                    return string("?Left Trigger");
                case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                    return string("?Right Trigger");
                case SDL_CONTROLLER_AXIS_MAX:
                    return string("?Axis-");
                }
            }

            return string("?Axis!!");
        },
        [&](ActionKind::None){ return string("(none)"); }
    }, *this);
}

Action Action::FromString(const std::string &str) {
    auto val = str.substr(1);
    switch(str.front()) {
    case 'k':
        return {{Kind::Keyboard{DataNode::Value(val)}}};
    case 'm':
        return {{Kind::MouseButton{DataNode::Value(val)}}};
    case 's':
        if(val.at(0) == 'x')
            return {{Kind::MouseScroll{make_pair(
                (val.at(1) == '+') ? 1 : -1,
                0)}}};
        else
            return {{Kind::MouseScroll{make_pair(
                0,
                (val.at(1) == '+') ? 1 : -1)}}};
    case 'c':
        return {{Kind::ControllerButton{static_cast<SDL_GameControllerButton>(DataNode::Value(val))}}};
    case 'n':
        return {{Kind::None{}}};
    case 'a':
        auto val2 = val.substr(1);
        return {{Kind::ControllerAxis{make_pair(
            static_cast<SDL_GameControllerAxis>(DataNode::Value(val2)),
            (val.front() == '-') ? -1.0 : 1.0
        )}}};
    }

    return {{Kind::Keyboard{DataNode::Value(val)}}};
}

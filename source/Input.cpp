#include "Input.h"
#include "Command.h"
#include <SDL_gamecontroller.h>
#include <SDL_joystick.h>

using namespace std;

namespace {
    SDL_GameController *controller;
    // TODO: Separate stick deadzones and triggers
    double deadzone = 0.5;

    int retryDelay = 0;
    // Retry to connect to a controller every 5 seconds
    constexpr static int RETRY_DELAY = 60 * 5;

    ActionKind::MouseScroll scrollState;

    Command::KeyMapping method;
}

void Input::SetInputMethod(Command::KeyMapping method_) {
    method = method_;
}
Command::KeyMapping Input::InputMethod() {
    return method;
}

void Input::Update() {
    if(!controller) {
        retryDelay ? retryDelay-- : 0;

        if(!retryDelay) {
            retryDelay = RETRY_DELAY;
        } else ;

        if(SDL_NumJoysticks()) {
            for(int i = 0; i < SDL_NumJoysticks(); i++) {
                if(SDL_IsGameController(i))
                {
                    controller = SDL_GameControllerOpen(i);
                }
            }
        } else retryDelay = RETRY_DELAY * 5;
    }
}

void Input::SetScrollState(const ActionKind::MouseScroll &state) {
    scrollState = state;
}

SDL_GameController *Input::GetController() {
    return controller;
}

ActionKind::MouseScroll &Input::MouseScrollState() {
    return scrollState;
}

double Input::Deadzone() {
    return deadzone;
}

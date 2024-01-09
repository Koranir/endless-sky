#include "Controller.h"
#include <SDL_error.h>
#include <SDL_gamecontroller.h>
#include <SDL_joystick.h>
#include <cmath>
#include <iostream>
#include <optional>
#include <ostream>

using namespace std;

namespace {
    SDL_GameController *controller = nullptr;
    double deadzone = 0.25;
}

optional<SDL_GameController *> Controller::Get() {
    if(controller)
        return make_optional(controller);
    else
        return optional<SDL_GameController *>();
}

bool Controller::TryConnect() {
    if(controller) return true;

    cerr << "Number of joysticks: " << SDL_NumJoysticks() << ".\n";

    for(int i = 0; i < SDL_NumJoysticks(); i++)
    {
        cerr << "Testing " << i << " for val\n";
        if(SDL_IsGameController(i))
        {
            cerr << i << " is a controller\n";
            controller = SDL_GameControllerOpen(i);

            if(!controller) {
                cerr << SDL_GetError() << endl;
            } else {
                cerr << "Ok!\n";
            }
        }
    }

    return controller;
}

double Controller::Deadzone() {
    return deadzone;
}

void Controller::SetDeadzone(double deadzone_) {
    deadzone = deadzone_;
}

bool Controller::IsValidVal(double percentage) {
    return abs(percentage) > deadzone;
}

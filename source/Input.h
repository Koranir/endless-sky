#ifndef ES_INPUT_H_
#define ES_INPUT_H_

#include "Action.h"
#include "Command.h"
#include <SDL_gamecontroller.h>

class Input {
public:
    static void Update();

    static void SetInputMethod(Command::KeyMapping method);
    static Command::KeyMapping InputMethod();
    static Command::KeyMapping MappingForAction(const Action &action) {
		return std::visit(ActionKind::Match{
			[&](ActionKind::MouseButton){ return Command::KeyMapping::MOUSE; },
			[&](ActionKind::MouseScroll){ return Command::KeyMapping::MOUSE; },
			[&](ActionKind::Keyboard){ return Command::KeyMapping::KEYBOARD; },
			[&](ActionKind::ControllerButton){ return Command::KeyMapping::CONTROLLER; },
			[&](ActionKind::ControllerAxis){ return Command::KeyMapping::CONTROLLER; },
            [&](ActionKind::None){ return Command::KeyMapping::KEYBOARD; }
		}, action);
	}

    static SDL_GameController *GetController();
    static double Deadzone();

    static ActionKind::MouseScroll &MouseScrollState();
    static void SetScrollState(const ActionKind::MouseScroll &state);

};

#endif

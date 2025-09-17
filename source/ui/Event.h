#pragma once

#include "Core.h"
#include "../MouseButton.h"

#include <variant>

namespace ui {

namespace event {
struct MouseClick {
	MouseButton button;
};

struct MouseOver {
	Vector<float> position;
};
}

using Event = std::variant<event::MouseClick, event::MouseOver>;

}

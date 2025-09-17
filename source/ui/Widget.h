/* ui/Widget.h
Copyright (c) 2025 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Core.h"

#include "Event.h"

#include <memory>
#include <type_traits>
#include <vector>

class Clipboard;

namespace ui {

template<typename Message>
class UiEngine;

template<typename Message>
class Widget {
public:
	struct Response {
		bool eventCaptured;
	};

	enum class MouseInteraction {
		NONE,
		IDLE,
		POINTER,
		GRAB,
		TEXT,
		CROSSHAIR,
		WORKING,
		GRABBING,
		RESIZING_HORIZONTALLY,
		RESIZING_VERTICALLY,
		RESIZING_DIAGONALLYUP,
		RESIZING_DIAGONALLYDOWN,
		NOT_ALLOWED,
		ZOOM_IN,
		ZOOM_OUT,
		CELL,
		MOVE,
		COPY,
		HELP,
	};

public:
	virtual ~Widget() = default;

public:
	virtual Size<Length> BoundSize() = 0;
	virtual void Layout(
		UiEngine<Message> &engine,
		Size<float> limits
	);
	virtual Response Update(
		UiEngine<Message> &engine,
		const Event &event,
		Rect<float> bounds,
		const Cursor &cursor,
		Clipboard &clipboard,
		std::vector<Message> &messages
	);
	virtual void Draw(
		UiEngine<Message> &engine,
		const DrawContext &drawContext,
		Rect<float> bounds,
		Rect<float> viewport,
		const Cursor &cursor
	) = 0;
	virtual MouseInteraction Interaction(
		UiEngine<Message> &engine,
		Rect<float> bounds,
		const Cursor &cursor
	);

};

template<typename Message>
using Element = std::unique_ptr<Widget<Message>>;
template<typename Message>
Element<Message> make_element(auto &&widget) {
	return std::make_unique<std::remove_reference_t<decltype(widget)>>(std::move(widget));
}


}

#include "Widget.cpp"

/* ui/Engine.h
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
#include "Widget.h"

#include "../text/Clipboard.h"

#include "../MouseButton.h"
#include <vector>

namespace ui {

template<typename Message>
class Widget;

template<typename Message>
class UiEngine {
public:
	UiEngine();
	UiEngine(Element<Message> &&element) : widget(std::move(element)) {}

	Rect<float> Bounds() const;
	void SetBounds(Rect<float> newBounds);
	void SetWidget(Element<Message> &&element);
	void SetCursor(Cursor newCursor);
	void Draw(Rect<float> viewport, const ui::DrawContext &context = ui::DrawContext {
		.textColor = Color(1.0),
	});

	void MouseClick(MouseButton button);
	void MouseOver(Vector<float> position);
	Widget<Message>::MouseInteraction MouseInteraction() const;

	std::vector<Message> DrainMessages();

private:
	void RelayoutIfDirty();

private:
	Element<Message> widget;
	Clipboard clipboard;

	Rect<float> bounds;
	Cursor cursor;

	bool dirty = true;
	std::vector<Message> messages;

};

}

#include "Engine.cpp"

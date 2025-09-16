/* ui/Widget.cpp
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

#include "Widget.h"
#include "Core.h"

namespace ui {

void Widget::Layout(
	Renderer &renderer,
	Size<float> bounds
) {}

Widget::Response Widget::Update(
	Renderer &renderer,
	const Event &event,
	Rect<float> bounds,
	const Cursor &cursor,
	Clipboard &clipboard,
	std::vector<Message> &messages
) {
	return Response {
		.eventCaptured = false,
	};
}

Widget::MouseInteraction Widget::Interaction(
	Renderer &renderer,
	Rect<float> bounds,
	const Cursor &cursor
) {
	return MouseInteraction::IDLE;
}



}

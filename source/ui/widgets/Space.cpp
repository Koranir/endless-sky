/* ui/widgets/Space.cpp
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

#include "Space.h"

namespace ui {
namespace widget {

template<typename Message>
Space<Message>::Space(Size<Length> size) : size(size) {}

template<typename Message>
Space<Message> Space<Message>::Horizontal(Length horizontal, Length vertical) {
	return Space(Size{
		horizontal,
		vertical
	});
}
template<typename Message>
Space<Message> Space<Message>::Vertical(Length vertical, Length horizontal) {
	return Space(Size{
		horizontal,
		vertical
	});
}
template<typename Message>
Space<Message> Space<Message>::Fill(int fill) {
	return Space(Size<Length>{
		{.fill = fill},
		{.fill = fill}
	});
}

template<typename Message>
Size<Length> Space<Message>::BoundSize() {
	return size;
}

template<typename Message>
void Space<Message>::Draw(
	UiEngine<Message> &engine,
	const DrawContext &drawContext,
	Rect<float> bounds,
	Rect<float> viewport,
	const Cursor &cursor
) {}

}
}

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

#include "Space.h"

namespace ui {
namespace widget {

Space::Space(Size<Length> size) : size(size) {}

Space Space::Horizontal(Length horizontal, Length vertical) {
	return Space(Size{
		horizontal,
		vertical
	});
}
Space Space::Vertical(Length vertical, Length horizontal) {
	return Space(Size{
		horizontal,
		vertical
	});
}
Space Space::Fill(int fill) {
	return Space(Size<Length>{
		{.fill = fill},
		{.fill = fill}
	});
}

Size<Length> Space::BoundSize() {
	return size;
}

void Space::Draw(
	Renderer &renderer,
	const DrawContext &drawContext,
	Rect<float> bounds,
	const Cursor &cursor
) {}

}
}

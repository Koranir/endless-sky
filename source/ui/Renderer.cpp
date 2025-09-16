/* ui/Renderer.cpp
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

#include "Renderer.h"

#include "Core.h"
#include "Widget.h"

namespace ui {

Renderer::Renderer() {}

Rect<float> Renderer::Viewport() const
{
	return viewport;
}

void Renderer::Draw(Widget &widget, Rect<float> bounds, const DrawContext &context)
{
	// virtual void Draw(
	// 	Renderer &renderer,
	// 	const DrawContext &drawContext,
	// 	Rect<float> bounds,
	// 	const Cursor &cursor
	// ) = 0;
	widget.Layout(*this, bounds.Dimensions());
	widget.Draw(
		*this,
		context,
		bounds,
		cursor
	);
}

}

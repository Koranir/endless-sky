/* ui/widgets/Column.cpp
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

#include "Column.h"

#include <algorithm>

using namespace ui;
using namespace ui::widget;



ColumnWidget::ColumnWidget() noexcept : size(Size<Length>::Splat({})) {}

Size<Length> ColumnWidget::BoundSize()
{
	Size<Length> nSize = size;
	for (const auto &elem : children) {
		Size elemSz = elem.first->BoundSize();
		nSize.width.MergeLimits(elemSz.width);
		nSize.height.MergeLimits(elemSz.height);
	}
	return nSize;
}

void ColumnWidget::Layout(
	Renderer &renderer,
	Size<float> limits
)
{
	float freeHeight = limits.height;
	float totalFactor = 0;
	std::vector<std::pair<Length, float *>> pass;
	for (auto &[elem, allocated] : children) {
		auto sz = elem->BoundSize();
		// freeHeight -= sz.height.min;
		totalFactor += sz.height.fill;
		pass.emplace_back(sz.height, &allocated);
	}
	// TODO: This might not be needed
	std::sort(pass.begin(), pass.end(), [](const auto &a, const auto &b) {
		return a.first.max < b.first.max;
	});
	for (auto &[elem, allocated] : pass) {
		*allocated = std::min(elem.max, std::max(elem.min, freeHeight * (elem.fill / totalFactor)));
		freeHeight -= *allocated;
		totalFactor -= elem.fill;
	}

	for (auto &[elem, allocated] : children) {
		elem->Layout(renderer, {
			.width = limits.width,
			.height = allocated,
		});
	}
}

Widget::Response ColumnWidget::Update(
 Renderer &renderer,
 const Event &event,
 Rect<float> bounds,
 const Cursor &cursor,
 Clipboard &clipboard,
 std::vector<Message> &messages
)
{
	// TODO: Some sort of keyboard focus system?
	// Tab-through while the column has focus, bubble up focus events, skip elements that don't want focus...

	for (auto &[elem, allocated] : children) {
		Rect<float> elemBounds = {
			.left = bounds.left,
			.right = bounds.right,
			.top = bounds.top,
			.bottom = bounds.top - allocated,
		};
		bounds.top -= allocated;

		auto response = elem->Update(renderer, event, elemBounds, cursor, clipboard, messages);
		if (response.eventCaptured)
			return response;
	}

	return { .eventCaptured = false };
}

void ColumnWidget::Draw(
	Renderer &renderer,
	const DrawContext &drawContext,
	Rect<float> bounds,
	const Cursor &cursor
)
{
	for (auto &[elem, allocated] : children) {
		Rect<float> elemBounds = {
			.left = bounds.left,
			.right = bounds.right,
			.top = bounds.top,
			.bottom = bounds.top - allocated,
		};
		bounds.top -= allocated;

		elem->Draw(renderer, drawContext, elemBounds, cursor);
	}
}

Widget::MouseInteraction ColumnWidget::Interaction(
 Renderer &renderer,
 Rect<float> bounds,
 const Cursor &cursor
)
{
	for (auto &[elem, allocated] : children) {
		Rect<float> elemBounds = {
			.left = bounds.left,
			.right = bounds.right,
			.top = bounds.top,
			.bottom = bounds.top - allocated,
		};
		bounds.top -= allocated;

		if (elemBounds.Contains(cursor.position.x, cursor.position.y)) {
			return elem->Interaction(renderer, elemBounds, cursor);
		}
	}

	return Widget::MouseInteraction::IDLE;
}

ColumnWidget &&ColumnWidget::Add(Element element)
{
	children.emplace_back(std::move(element), 0);
	return std::move(*this);
}

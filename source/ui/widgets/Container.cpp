/* ui/widgets/Container.cpp
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

#include "Container.h"

#include "../Widget.h"
#include "../../Rectangle.h"
#include "../../shader/FillShader.h"

#include <type_traits>

using namespace std;

namespace ui {
namespace widget {

template<typename Message>
ContainerWidget<Message> &&ContainerWidget<Message>::Width(Length width)
{
	this->width = width;
	return std::move(*this);
}



template<typename Message>
ContainerWidget<Message> &&ContainerWidget<Message>::Height(Length height)
{
	this->height = height;
	return std::move(*this);
}



template<typename Message>
ContainerWidget<Message> &&ContainerWidget<Message>::Padding(ui::Padding padding)
{
	this->padding = padding;
	return std::move(*this);
}



template<typename Message>
ContainerWidget<Message> &&ContainerWidget<Message>::AlignX(Alignment horizontal)
{
	this->horizontal = horizontal;
	return std::move(*this);
}



template<typename Message>
ContainerWidget<Message> &&ContainerWidget<Message>::AlignY(Alignment vertical)
{
	this->vertical = vertical;
	return std::move(*this);
}



template<typename Message>
ContainerWidget<Message> &&ContainerWidget<Message>::Center()
{
	this->horizontal = Alignment::CENTER;
	this->vertical = Alignment::CENTER;
	return std::move(*this);
}



template<typename Message>
ContainerWidget<Message> &&ContainerWidget<Message>::Style(ContainerStyle::Default style)
{
	this->style = style;
	return std::move(*this);
}



template<typename Message>
ContainerWidget<Message> &&ContainerWidget<Message>::Style(ContainerStyle::None style)
{
	this->style = style;
	return std::move(*this);
}



template<typename Message>
Size<Length> ContainerWidget<Message>::BoundSize()
{
	Size child = widget->BoundSize();
	Length nWidth = width.value_or(child.width);
	nWidth.min += padding.left + padding.right;
	nWidth.max += padding.left + padding.right;
	Length nHeight = height.value_or(child.height);
	nHeight.min += padding.bottom + padding.top;
	nHeight.max += padding.bottom + padding.top;

	return Size<Length>{nWidth, nHeight};
}



template<typename Message>
void ContainerWidget<Message>::Draw(
	UiEngine<Message> &engine,
	const DrawContext &drawContext,
	Rect<float> bounds,
	Rect<float> viewport,
	const Cursor &cursor
)
{
	DrawContext context = drawContext;

	std::visit([&](auto &&style) {
		if constexpr (std::is_same_v<std::remove_cvref_t<decltype(style)>, ContainerStyle::Default>) {
			ContainerStyle::Default s = style;
			context.textColor = s.text.value_or(context.textColor);
			if (s.background) {
				std::optional<Rect<float>> intersection = bounds.Union(viewport);
				if (intersection) {
					FillShader::Fill(
						Rectangle::WithCorners(
							Point(intersection->left, intersection->bottom),
							Point(intersection->right, intersection->top)
						),
						*s.background
					);

				}
			}
		}
	}, style);

	widget->Draw(engine, drawContext, bounds.Padded(padding), viewport, cursor);
}



template<typename Message>
void ContainerWidget<Message>::Layout(
	UiEngine<Message> &engine,
	Size<float> limits
)
{
	widget->Layout(engine, Size<float>{
		limits.width - (padding.left + padding.right),
		limits.height - (padding.top + padding.bottom)
	});
}



template<typename Message>
Widget<Message>::Response ContainerWidget<Message>::Update(
	UiEngine<Message> &engine,
	const Event &event,
	Rect<float> bounds,
	const Cursor &cursor,
	Clipboard &clipboard,
	std::vector<Message> &messages
)
{
	return widget->Update(engine, event, bounds.Padded(padding), cursor, clipboard, messages);
}



template<typename Message>
Widget<Message>::MouseInteraction ContainerWidget<Message>::Interaction(
	UiEngine<Message> &engine,
	Rect<float> bounds,
	const Cursor &cursor
)
{
	return widget->Interaction(engine, bounds.Padded(padding), cursor);
}



}
}

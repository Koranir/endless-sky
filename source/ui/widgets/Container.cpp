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

#include "Container.h"

#include "../Widget.h"
#include "../../Rectangle.h"
#include "../../shader/FillShader.h"

#include <type_traits>

using namespace std;

namespace ui {
namespace widget {

ContainerWidget &&ContainerWidget::Width(Length width)
{
	this->width = width;
	return std::move(*this);
}



ContainerWidget &&ContainerWidget::Height(Length height)
{
	this->height = height;
	return std::move(*this);
}



ContainerWidget &&ContainerWidget::Padding(ui::Padding padding)
{
	this->padding = padding;
	return std::move(*this);
}



ContainerWidget &&ContainerWidget::AlignX(Alignment horizontal)
{
	this->horizontal = horizontal;
	return std::move(*this);
}



ContainerWidget &&ContainerWidget::AlignY(Alignment vertical)
{
	this->vertical = vertical;
	return std::move(*this);
}



ContainerWidget &&ContainerWidget::Center()
{
	this->horizontal = Alignment::CENTER;
	this->vertical = Alignment::CENTER;
	return std::move(*this);
}



ContainerWidget &&ContainerWidget::Style(ContainerStyle::Default style)
{
	this->style = style;
	return std::move(*this);
}



ContainerWidget &&ContainerWidget::Style(ContainerStyle::None style)
{
	this->style = style;
	return std::move(*this);
}



Size<Length> ContainerWidget::BoundSize()
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



void ContainerWidget::Draw(
	Renderer &renderer,
	const DrawContext &drawContext,
	Rect<float> bounds,
	const Cursor &cursor
)
{
	DrawContext context = drawContext;

	std::visit([&](auto &&style) {
		if constexpr (std::is_same_v<std::remove_cvref_t<decltype(style)>, ContainerStyle::Default>) {
			ContainerStyle::Default s = style;
			context.textColor = s.text.value_or(context.textColor);
			if (s.background) {
				FillShader::Fill(
					Rectangle::WithCorners(Point(bounds.left, bounds.bottom), Point(bounds.right, bounds.top)),
					*s.background
				);
			}
		}
	}, style);

	widget->Draw(renderer, drawContext, bounds.Padded(padding), cursor);
}



void ContainerWidget::Layout(
	Renderer &renderer,
	Size<float> limits
)
{
	widget->Layout(renderer, Size<float>{
		limits.width - (padding.left + padding.right),
		limits.height - (padding.top + padding.bottom)
	});
}



Widget::Response ContainerWidget::Update(
	Renderer &renderer,
	const Event &event,
	Rect<float> bounds,
	const Cursor &cursor,
	Clipboard &clipboard,
	std::vector<Message> &messages
)
{
	return widget->Update(renderer, event, bounds.Padded(padding), cursor, clipboard, messages);
}



Widget::MouseInteraction ContainerWidget::Interaction(
	Renderer &renderer,
	Rect<float> bounds,
	const Cursor &cursor
)
{
	return widget->Interaction(renderer, bounds.Padded(padding), cursor);
}



}
}

/* ui/Core.h
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

#include "../Color.h"

#include <limits>

namespace ui {
	enum class Alignment {
		START,
		CENTER,
		END,
	};

	struct Length {
	public:
		// constexpr Length(float size) noexcept : min(size), max(size) {}
		// constexpr Length(int fill, float min, float max) : fluidity(fluidity), min(min), max(max) {
		// 	if(min < 0 || max < 0 || min > max)
		// 		throw std::invalid_argument("Length min and max must be non-negative, and min must be <= max.");
		// };

		constexpr bool IsFill() const noexcept {
			return fill > 0;
		}

		constexpr void MergeLimits(Length other) {
			min = std::max(min, other.min);
			max = std::min(max, other.max);
		}

	public:
		int fill = 0;
		// should be >0
		float min = 0;
		float max = std::numeric_limits<float>::infinity();

	};

	template<typename T>
	struct Size {
		static Size<T> Splat(T elem) {
			return Size {
				.width = elem,
				.height = elem,
			};
		}

		T width;
		T height;
	};

	struct Padding {
		Padding() {}
		Padding(float all)
			: left(all), right(all), top(all), bottom(all) {}
		Padding(float horizontal, float vertical)
			: left(horizontal), right(horizontal), top(vertical), bottom(vertical) {}

		float left = 0.;
		float right = 0.;
		float top = 0.;
		float bottom = 0.;
	};

	template<typename T>
	struct Vector {
		T x;
		T y;
	};

	template<typename T>
	struct Rect {
		T left;
		T right;
		T top;
		T bottom;

		Rect<T> Padded(Padding pad_by) {
			return Rect {
				.left = left + pad_by.left,
				.right = right - pad_by.right,
				.top = top - pad_by.top,
				.bottom = bottom + pad_by.bottom
			};
		}

		Size<T> Dimensions() const noexcept {
			return Size<T> {
				.width = right - left,
				.height = top - bottom
			};
		}

		constexpr bool Contains(T x, T y) const noexcept {
			return x >= left && x <= right && y >= bottom && y <= top;
		}
	};

	struct BorderRadius {
		float tl = 0.0;
		float tr = 0.0;
		float bl = 0.0;
		float br = 0.0;
	};

	struct Border {
		Color color = Color(0.0);
		float width = 0.0;
		BorderRadius radius;
	};

	struct Shadow {
		Color color = Color(0.0);
		Vector<float> offset = {0, 0};
		float blurRadius = 0.0;
	};

	struct DrawContext {
		Color textColor;
	};

	struct Cursor {
		// Widget-local position (based on the bounds of the widget, not global)
		Vector<float> position;
	};

}

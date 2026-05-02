#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Macros.h"

struct Rect {
	vec2 position{ 0.f, 0.f }; // top-left
	vec2 size{ 0.f, 0.f };

	[[nodiscard]] AQUILA_FORCE_INLINE float Left() const { return position.x; }
	[[nodiscard]] AQUILA_FORCE_INLINE float Top() const { return position.y; }
	[[nodiscard]] AQUILA_FORCE_INLINE float Right() const { return position.x + size.x; }
	[[nodiscard]] AQUILA_FORCE_INLINE float Bottom() const { return position.y + size.y; }
	[[nodiscard]] AQUILA_FORCE_INLINE float Width() const { return size.x; }
	[[nodiscard]] AQUILA_FORCE_INLINE float Height() const { return size.y; }
	[[nodiscard]] AQUILA_FORCE_INLINE vec2 Center() const { return position + size * 0.5f; }

	[[nodiscard]] AQUILA_FORCE_INLINE bool Contains(vec2 p) const {
		return p.x >= Left() && p.x < Right() && p.y >= Top() && p.y < Bottom();
	}

	[[nodiscard]] AQUILA_FORCE_INLINE bool Overlaps(const Rect &other) const {
		return Left() < other.Right() && Right() > other.Left() && Top() < other.Bottom() && Bottom() > other.Top();
	}

	[[nodiscard]] Rect Intersect(const Rect &other) const {
		float left = std::max(Left(), other.Left());
		float top = std::max(Top(), other.Top());
		float right = std::min(Right(), other.Right());
		float bottom = std::min(Bottom(), other.Bottom());
		return { { left, top }, { std::max(0.f, right - left), std::max(0.f, bottom - top) } };
	}

	static Rect FromMinMax(vec2 min, vec2 max) { return { .position = min, .size = max - min }; }

	[[nodiscard]] bool IsEmpty() const { return size.x <= 0.f || size.y <= 0.f; }
};

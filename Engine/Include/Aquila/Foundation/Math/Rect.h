#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Macros.h"

struct Rect {
	Vec2 position{ 0.F, 0.F }; // top-left
	Vec2 size{ 0.F, 0.F };

	[[nodiscard]] AQUILA_FORCE_INLINE float left() const { return position.x; }
	[[nodiscard]] AQUILA_FORCE_INLINE float top() const { return position.y; }
	[[nodiscard]] AQUILA_FORCE_INLINE float right() const { return position.x + size.x; }
	[[nodiscard]] AQUILA_FORCE_INLINE float bottom() const { return position.y + size.y; }
	[[nodiscard]] AQUILA_FORCE_INLINE float width() const { return size.x; }
	[[nodiscard]] AQUILA_FORCE_INLINE float height() const { return size.y; }
	[[nodiscard]] AQUILA_FORCE_INLINE Vec2 center() const { return position + size * 0.5f; }

	[[nodiscard]] AQUILA_FORCE_INLINE bool contains(Vec2 p) const {
		return p.x >= left() && p.x < right() && p.y >= top() && p.y < bottom();
	}

	[[nodiscard]] AQUILA_FORCE_INLINE bool overlaps(const Rect &other) const {
		return left() <= other.right() && right() >= other.left() && top() <= other.bottom() && bottom() >= other.top();
	}

	[[nodiscard]] AQUILA_FORCE_INLINE Rect Union(const Rect &other) const {
		const float x = std::min(left(), other.left());
		const float y = std::min(top(), other.top());
		const float r = std::max(right(), other.right());
		const float b = std::max(bottom(), other.bottom());
		return { .position = { x, y }, .size = { r - x, b - y } };
	}

	[[nodiscard]] AQUILA_FORCE_INLINE Rect intersect(const Rect &other) const {
		float l = std::max(left(), other.left());
		float t = std::max(top(), other.top());
		float r = std::min(right(), other.right());
		float b = std::min(bottom(), other.bottom());
		return { .position = { l, t }, .size = { std::max(0.F, r - l), std::max(0.F, b - t) } };
	}

	static AQUILA_FORCE_INLINE Rect from_min_max(Vec2 min, Vec2 max) { return { .position = min, .size = max - min }; }

	[[nodiscard]] AQUILA_FORCE_INLINE bool is_empty() const { return size.x <= 0.F || size.y <= 0.F; }

	[[nodiscard]] AQUILA_FORCE_INLINE bool operator==(const Rect &other) const {
		return position == other.position && size == other.size;
	}
	[[nodiscard]] AQUILA_FORCE_INLINE bool operator!=(const Rect &other) const { return !(*this == other); }
};

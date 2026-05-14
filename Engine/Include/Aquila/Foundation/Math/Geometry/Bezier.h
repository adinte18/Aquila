#pragma once

#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/Math/Math.h"

namespace Aquila::Math::Bezier {
struct QuadraticBezier {
	vec2 p0;
	vec2 p1;
	vec2 p2;
};

struct BezierCoefficients {
	vec2 a;
	vec2 b;
	vec2 c;
};

struct BezierBounds {
	vec2 min;
	vec2 max;
};

struct BezierSplit {
	QuadraticBezier left;
	QuadraticBezier right;
	bool wasSplit;
};

struct BezierExtrema {
	Option<f32> tX;
	Option<f32> tY;
};

AQUILA_FORCE_INLINE vec2 Evaluate(const QuadraticBezier &curve, f32 t) {
	f32 u = 1.0F - t;
	return (u * u) * curve.p0 + (2.0F * u * t) * curve.p1 + (t * t) * curve.p2;
}

AQUILA_FORCE_INLINE vec2 EvaluateFirstDerivative(const QuadraticBezier &curve, f32 t) {
	f32 u = 1.0F - t;
	return (2.0F * u) * (curve.p1 - curve.p0) + (2.0F * t) * (curve.p2 - curve.p1);
}

AQUILA_FORCE_INLINE vec2 EvaluateSecondDerivative(const QuadraticBezier &curve) {
	return 2.0F * (curve.p2 - (2.0F * curve.p1) + curve.p0);
}

AQUILA_FORCE_INLINE BezierCoefficients ComputeCoefficients(const QuadraticBezier &curve) {
	// b(t) = at^2 + bt + c
	return {
		.a = curve.p0 - 2.0F * curve.p1 + curve.p2,
		.b = 2.0F * (curve.p1 - curve.p0),
		.c = curve.p0,
	};
}

// https://iquilezles.org/articles/bezierbbox/
AQUILA_FORCE_INLINE BezierBounds ComputeBounds(const QuadraticBezier &curve) {
	BezierBounds bounds{};
	bounds.min = Math::Min<vec2>(curve.p0, curve.p2);
	bounds.max = Math::Max<vec2>(curve.p0, curve.p2);

	if (curve.p1.x < bounds.min.x || curve.p1.x > bounds.max.x || curve.p1.y < bounds.min.y ||
		curve.p1.y > bounds.max.y) {
		vec2 a = curve.p0 - 2.0F * curve.p1 + curve.p2;
		vec2 b = 2.0F * (curve.p1 - curve.p0);

		vec2 t = Math::Clamp(-b / (2.0F * a), 0.0F, 1.0F);
		vec2 s = 1.0F - t;
		vec2 q = s * s * curve.p0 + 2.0F * s * t * curve.p1 + t * t * curve.p2;
		bounds.min = Math::Min<vec2>(bounds.min, q);
		bounds.max = Math::Max<vec2>(bounds.max, q);
	}

	return bounds;
}

AQUILA_FORCE_INLINE BezierExtrema FindExtrema(const QuadraticBezier &curve) {
	BezierCoefficients coefficients = ComputeCoefficients(curve);
	BezierExtrema extrema{};
	if (Math::Abs(coefficients.a.x) > Math::EPSILON) {
		extrema.tX = -coefficients.b.x / (2.0f * coefficients.a.x);
	}

	if (Math::Abs(coefficients.a.y) > Math::EPSILON) {
		extrema.tY = -coefficients.b.y / (2.0f * coefficients.a.y);
	}
	return extrema;
}

// le goat DeCasteljau
AQUILA_FORCE_INLINE BezierSplit Split(const QuadraticBezier &curve, f32 t) {
	vec2 q0 = Math::Lerp(curve.p0, curve.p1, t);
	vec2 q1 = Math::Lerp(curve.p1, curve.p2, t);
	vec2 mid = Math::Lerp(q0, q1, t);
	return {
		.left = QuadraticBezier{ .p0 = curve.p0, .p1 = q0, .p2 = mid },
		.right = QuadraticBezier{ .p0 = mid, .p1 = q1, .p2 = curve.p2 },
		.wasSplit = true,
	};
}

AQUILA_FORCE_INLINE BezierSplit SplitAtYExtrema(const QuadraticBezier &curve) {
	BezierExtrema extrema = FindExtrema(curve);
	if (extrema.tY.has_value()) {
		f32 t = extrema.tY.value();
		if (t > 0.0f && t < 1.0f) {
			return Split(curve, t);
		}
	}
	return { .left = curve, .right = curve, .wasSplit = false };
}

AQUILA_FORCE_INLINE BezierSplit SplitAtXExtrema(const QuadraticBezier &curve) {
	BezierExtrema extrema = FindExtrema(curve);
	if (extrema.tX.has_value()) {
		f32 t = extrema.tX.value();
		if (t > 0.0f && t < 1.0f) {
			return Split(curve, t);
		}
	}
	return { .left = curve, .right = curve, .wasSplit = false };
}

} // namespace Aquila::Math::Bezier

#ifndef AQUILA_MATH_H
#define AQUILA_MATH_H

#include "Aquila/Foundation/Math/MathTypes.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

#include <array>
#include <limits>
#include <numbers>

namespace Aquila::Math {

constexpr f32 PI = std::numbers::pi_v<float>;
constexpr f32 TAU = 2.0f * PI;
constexpr f32 HALF_PI = PI / 2.0f;
constexpr f32 EPSILON = 1e-6f;
constexpr f32 DEG_TO_RAD = PI / 180.0f;
constexpr f32 RAD_TO_DEG = 180.0f / PI;

// Scalar / generic

template <typename T, typename U> AQUILA_FORCE_INLINE T Clamp(T value, U min, U max) {
	return glm::clamp(value, T(min), T(max));
}

template <typename T> AQUILA_FORCE_INLINE T Lerp(const T a, const T b, const f32 t) {
	return glm::mix(a, b, t);
}

AQUILA_FORCE_INLINE f32 SmoothStep(const f32 edge0, const f32 edge1, const f32 x) {
	return glm::smoothstep(edge0, edge1, x);
}

AQUILA_FORCE_INLINE bool ApproxEqual(const f32 a, const f32 b, const f32 epsilon = EPSILON) {
	return std::abs(a - b) < epsilon;
}

AQUILA_FORCE_INLINE f32 Radians(const f32 degrees) {
	return glm::radians(degrees);
}

AQUILA_FORCE_INLINE f32 Degrees(const f32 radians) {
	return glm::degrees(radians);
}

AQUILA_FORCE_INLINE f32 Mod(const f32 a, const f32 b) {
	return glm::mod(a, b);
}

AQUILA_FORCE_INLINE f32 NormalizeAngle(f32 angle) {
	angle = Mod(angle, TAU);
	if (angle < 0.0f) {
		angle += TAU;
	}
	return angle;
}

AQUILA_FORCE_INLINE f32 AngleDifference(const f32 a, const f32 b) {
	const f32 diff = Mod(b - a + PI, TAU) - PI;
	return diff < -PI ? diff + TAU : diff;
}

// Component-wise vector ops

template <typename T> AQUILA_FORCE_INLINE T Min(const T &a, const T &b) {
	return glm::min(a, b);
}

template <typename T> AQUILA_FORCE_INLINE T Max(const T &a, const T &b) {
	return glm::max(a, b);
}

template <typename T> AQUILA_FORCE_INLINE T Abs(const T &v) {
	return glm::abs(v);
}

template <typename T> AQUILA_FORCE_INLINE T Floor(const T &v) {
	return glm::floor(v);
}

template <typename T> AQUILA_FORCE_INLINE T Ceil(const T &v) {
	return glm::ceil(v);
}

template <typename T> AQUILA_FORCE_INLINE T Sign(const T &v) {
	return glm::sign(v);
}

template <typename T> AQUILA_FORCE_INLINE T Fract(const T &v) {
	return glm::fract(v);
}

template <typename T> AQUILA_FORCE_INLINE T Sqrt(const T &v) {
	return glm::sqrt(v);
}

template <typename T> AQUILA_FORCE_INLINE T Pow(const T &base, const T &exp) {
	return glm::pow(base, exp);
}

// Vector ops

AQUILA_FORCE_INLINE f32 Dot(const vec2 &a, const vec2 &b) {
	return glm::dot(a, b);
}

AQUILA_FORCE_INLINE f32 Dot(const vec3 &a, const vec3 &b) {
	return glm::dot(a, b);
}

AQUILA_FORCE_INLINE f32 Dot(const vec4 &a, const vec4 &b) {
	return glm::dot(a, b);
}

AQUILA_FORCE_INLINE vec3 Cross(const vec3 &a, const vec3 &b) {
	return glm::cross(a, b);
}

template <typename T> AQUILA_FORCE_INLINE f32 Length(const T &v) {
	return glm::length(v);
}

template <typename T> AQUILA_FORCE_INLINE f32 LengthSq(const T &v) {
	return glm::dot(v, v);
}

template <typename T> AQUILA_FORCE_INLINE T Normalize(const T &v) {
	return glm::normalize(v);
}

template <typename T> AQUILA_FORCE_INLINE f32 Distance(const T &a, const T &b) {
	return glm::distance(a, b);
}

template <typename T> AQUILA_FORCE_INLINE T Reflect(const T &v, const T &normal) {
	return glm::reflect(v, normal);
}

template <typename T> AQUILA_FORCE_INLINE T Refract(const T &v, const T &normal, f32 eta) {
	return glm::refract(v, normal, eta);
}

// Matrix ops

AQUILA_FORCE_INLINE vec4 MatMulVec(const mat4 &m, const vec4 &v) {
	return m * v;
}

AQUILA_FORCE_INLINE mat4 MatMul(const mat4 &a, const mat4 &b) {
	return a * b;
}

AQUILA_FORCE_INLINE mat4 Inverse(const mat4 &m) {
	return glm::inverse(m);
}

AQUILA_FORCE_INLINE mat4 Transpose(const mat4 &m) {
	return glm::transpose(m);
}

// Projection matrices (Vulkan)

inline mat4 OrthoVulkan(f32 left, f32 right, f32 bottom, f32 top, f32 zNear, f32 zFar) {
	mat4 result(1.0f);
	result[0][0] = 2.0f / (right - left);
	result[1][1] = 2.0f / (top - bottom);
	result[2][2] = 1.0f / (zFar - zNear);
	result[3][0] = -(right + left) / (right - left);
	result[3][1] = -(top + bottom) / (top - bottom);
	result[3][2] = -zNear / (zFar - zNear);
	return result;
}

inline mat4 PerspectiveVulkan(const f32 fovY, const f32 aspect, const f32 zNear, const f32 zFar) {
	const f32 tanHalfFovy = std::tan(fovY / 2.0f);
	mat4 result(0.0f);
	result[0][0] = 1.0f / (aspect * tanHalfFovy);
	result[1][1] = 1.0f / tanHalfFovy;
	result[2][2] = zFar / (zFar - zNear);
	result[2][3] = 1.0f;
	result[3][2] = -(zFar * zNear) / (zFar - zNear);
	return result;
}

inline mat4 InfinitePerspectiveVulkan(const f32 fovY, const f32 aspect, const f32 zNear) {
	const f32 tanHalfFovy = std::tan(fovY / 2.0f);
	mat4 result(0.0f);
	result[0][0] = 1.0f / (aspect * tanHalfFovy);
	result[1][1] = 1.0f / tanHalfFovy;
	result[2][2] = 1.0f;
	result[2][3] = 1.0f;
	result[3][2] = -zNear;
	return result;
}

// View matrices

inline mat4 LookAt(const vec3 &position, const vec3 &target, const vec3 &up) {
	const vec3 w = Normalize(target - position);
	const vec3 u = Normalize(Cross(w, up));
	const vec3 v = Cross(w, u);

	mat4 result(1.0f);
	result[0][0] = u.x;
	result[1][0] = u.y;
	result[2][0] = u.z;
	result[0][1] = v.x;
	result[1][1] = v.y;
	result[2][1] = v.z;
	result[0][2] = w.x;
	result[1][2] = w.y;
	result[2][2] = w.z;
	result[3][0] = -Dot(u, position);
	result[3][1] = -Dot(v, position);
	result[3][2] = -Dot(w, position);
	return result;
}

inline mat4 LookInDirection(const vec3 &position, const vec3 &direction, const vec3 &worldUp = vec3(0, -1, 0)) {
	const vec3 w = Normalize(direction);
	const vec3 u = Normalize(Cross(w, worldUp));
	const vec3 v = Cross(w, u);

	mat4 result(1.0f);
	result[0][0] = u.x;
	result[1][0] = u.y;
	result[2][0] = u.z;
	result[0][1] = v.x;
	result[1][1] = v.y;
	result[2][1] = v.z;
	result[0][2] = w.x;
	result[1][2] = w.y;
	result[2][2] = w.z;
	result[3][0] = -Dot(u, position);
	result[3][1] = -Dot(v, position);
	result[3][2] = -Dot(w, position);
	return result;
}

inline mat4 ViewFromEuler(const vec3 &position, const vec3 &rotation) {
	const f32 c3 = std::cos(rotation.z), s3 = std::sin(rotation.z);
	const f32 c2 = std::cos(rotation.x), s2 = std::sin(rotation.x);
	const f32 c1 = std::cos(rotation.y), s1 = std::sin(rotation.y);

	const vec3 u = { c1 * c3 + s1 * s2 * s3, c2 * s3, c1 * s2 * s3 - c3 * s1 };
	const vec3 v = { c3 * s1 * s2 - c1 * s3, c2 * c3, c1 * c3 * s2 + s1 * s3 };
	const vec3 w = { c2 * s1, -s2, c1 * c2 };

	mat4 result(1.0f);
	result[0][0] = u.x;
	result[1][0] = u.y;
	result[2][0] = u.z;
	result[0][1] = v.x;
	result[1][1] = v.y;
	result[2][1] = v.z;
	result[0][2] = w.x;
	result[1][2] = w.y;
	result[2][2] = w.z;
	result[3][0] = -Dot(u, position);
	result[3][1] = -Dot(v, position);
	result[3][2] = -Dot(w, position);
	return result;
}

// Geometry utilities

inline std::array<vec4, 8> ExtractFrustumCorners(const mat4 &projView) {
	const mat4 inv = Inverse(projView);
	std::array<vec4, 8> corners;
	int idx = 0;
	for (int z = 0; z < 2; ++z) {
		for (int y = 0; y < 2; ++y) {
			for (int x = 0; x < 2; ++x) {
				vec4 pt = MatMulVec(inv, vec4(x ? 1.f : -1.f, y ? 1.f : -1.f, z ? 1.f : 0.f, 1.f));
				corners[idx++] = pt / pt.w;
			}
		}
	}
	return corners;
}

inline void ComputeAABB(const std::array<vec3, 8> &points, vec3 &outMin, vec3 &outMax) {
	outMin = vec3(std::numeric_limits<f32>::max());
	outMax = vec3(std::numeric_limits<f32>::lowest());
	for (const auto &p : points) {
		outMin = Min(outMin, p);
		outMax = Max(outMax, p);
	}
}

inline void ComputeLightSpaceAABB(const std::array<vec4, 8> &corners, const mat4 &lightView, vec3 &outMin,
								  vec3 &outMax) {
	outMin = vec3(std::numeric_limits<f32>::max());
	outMax = vec3(std::numeric_limits<f32>::lowest());
	for (const auto &corner : corners) {
		const vec4 ls = MatMulVec(lightView, corner);
		const vec3 lsp = vec3(ls.x, ls.y, ls.z);
		outMin = Min(outMin, lsp);
		outMax = Max(outMax, lsp);
	}
}

inline mat4 BuildLightViewMatrix(const vec3 &lightDirection, const vec3 &focusPoint, const f32 distance = 100.0f) {
	const vec3 lightDir = Normalize(lightDirection);
	vec3 worldUp = vec3(0.f, 1.f, 0.f);
	if (std::abs(Dot(lightDir, worldUp)) > 0.99f) {
		worldUp = vec3(0.f, 0.f, 1.f);
	}

	const vec3 w = Normalize(lightDir);
	const vec3 u = Normalize(Cross(w, worldUp));
	const vec3 v = Cross(w, u);
	const vec3 lightPos = focusPoint - lightDir * distance;

	mat4 result(1.0f);
	result[0][0] = u.x;
	result[1][0] = u.y;
	result[2][0] = u.z;
	result[0][1] = v.x;
	result[1][1] = v.y;
	result[2][1] = v.z;
	result[0][2] = w.x;
	result[1][2] = w.y;
	result[2][2] = w.z;
	result[3][0] = -Dot(u, lightPos);
	result[3][1] = -Dot(v, lightPos);
	result[3][2] = -Dot(w, lightPos);
	return result;
}

inline void SnapToTexelGrid(f32 &min, f32 &max, const f32 worldUnitsPerTexel) {
	min = std::floor(min / worldUnitsPerTexel) * worldUnitsPerTexel;
	max = std::floor(max / worldUnitsPerTexel) * worldUnitsPerTexel;
}

inline f32 FixZBoundsForShadows(const f32 minZ, const f32 maxZ, f32 &outNear, f32 &outFar) {
	f32 zTranslation = 0.0f;
	if (minZ < 0.0f) {
		zTranslation = -minZ;
		outNear = 0.0f;
		outFar = maxZ - minZ;
	} else {
		outNear = minZ;
		outFar = maxZ;
	}
	if (outFar <= outNear) {
		outFar = outNear + 1.0f;
	}
	return zTranslation;
}

AQUILA_FORCE_INLINE f32 DistanceToPlane(const vec3 &point, const vec3 &normal, const vec3 &planePoint) {
	return Dot(normal, point - planePoint);
}

AQUILA_FORCE_INLINE vec3 ProjectOntoPlane(const vec3 &point, const vec3 &normal, const vec3 &planePoint) {
	return point - normal * DistanceToPlane(point, normal, planePoint);
}

AQUILA_FORCE_INLINE vec3 Barycentric(const vec3 &p, const vec3 &a, const vec3 &b, const vec3 &c) {
	const vec3 v0 = b - a, v1 = c - a, v2 = p - a;
	const f32 d00 = Dot(v0, v0), d01 = Dot(v0, v1), d11 = Dot(v1, v1);
	const f32 d20 = Dot(v2, v0), d21 = Dot(v2, v1);
	const f32 denom = d00 * d11 - d01 * d01;
	if (std::abs(denom) < EPSILON) {
		return vec3(0.f);
	}
	const f32 vv = (d11 * d20 - d01 * d21) / denom;
	const f32 ww = (d00 * d21 - d01 * d20) / denom;
	return vec3(1.0f - vv - ww, vv, ww);
}

AQUILA_FORCE_INLINE f32 SignedVolume(const vec3 &a, const vec3 &b, const vec3 &c, const vec3 &d) {
	return Dot(Cross(b - a, c - a), d - a) / 6.0f;
}

} // namespace Aquila::Math

#endif // AQUILA_MATH_H

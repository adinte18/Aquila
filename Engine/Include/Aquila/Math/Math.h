#ifndef AQUILA_MATH_H
#define AQUILA_MATH_H

#include "Aquila/Platform/PrimitiveTypes.h"
#include <cmath>

#include <array>
#include <limits>
#include <algorithm>
#include <numbers>

namespace Aquila::Math {

constexpr f32 PI = std::numbers::pi_v<float>;
constexpr f32 TAU = 2.0f * PI;
constexpr f32 HALF_PI = PI / 2.0f;
constexpr f32 EPSILON = 1e-6f;
constexpr f32 DEG_TO_RAD = PI / 180.0f;
constexpr f32 RAD_TO_DEG = 180.0f / PI;

/// Dot product
AQUILA_FORCE_INLINE f32 Dot(const vec3 &a, const vec3 &b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

/// Cross product
AQUILA_FORCE_INLINE vec3 Cross(const vec3 &a, const vec3 &b) {
	return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

/// Vector length
AQUILA_FORCE_INLINE f32 Length(const vec3 &v) {
	return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

/// Normalize vector
AQUILA_FORCE_INLINE vec3 Normalize(const vec3 &v) {
	f32 len = Length(v);
	if (len < EPSILON) {
		return { 0.0f, 0.0f, 0.0f };
	}
	return { v.x / len, v.y / len, v.z / len };
}

/// Component-wise min
AQUILA_FORCE_INLINE vec3 Min(const vec3 &a, const vec3 &b) {
	return { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) };
}

/// Component-wise max
AQUILA_FORCE_INLINE vec3 Max(const vec3 &a, const vec3 &b) {
	return { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) };
}

// Matrix Operations

/// Matrix-vector multiplication (mat4 * vec4)
AQUILA_FORCE_INLINE vec4 MatMulVec(const mat4 &m, const vec4 &v) {
	return { (m[0][0] * v.x) + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w,
			 m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w,
			 m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w,
			 m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w };
}

/// Matrix multiplication
inline mat4 MatMul(const mat4 &a, const mat4 &b) {
	mat4 result;
	for (int col = 0; col < 4; ++col) {
		for (int row = 0; row < 4; ++row) {
			result[col][row] =
				(a[0][row] * b[col][0]) + (a[1][row] * b[col][1]) + (a[2][row] * b[col][2]) + (a[3][row] * b[col][3]);
		}
	}
	return result;
}

/// Matrix inverse (using Gaussian elimination)
inline mat4 Inverse(const mat4 &m) {
	mat4 inv;
	f32 det = NAN;

	inv[0][0] = m[1][1] * m[2][2] * m[3][3] - m[1][1] * m[2][3] * m[3][2] - m[2][1] * m[1][2] * m[3][3] +
				m[2][1] * m[1][3] * m[3][2] + m[3][1] * m[1][2] * m[2][3] - m[3][1] * m[1][3] * m[2][2];
	inv[1][0] = -m[1][0] * m[2][2] * m[3][3] + m[1][0] * m[2][3] * m[3][2] + m[2][0] * m[1][2] * m[3][3] -
				m[2][0] * m[1][3] * m[3][2] - m[3][0] * m[1][2] * m[2][3] + m[3][0] * m[1][3] * m[2][2];
	inv[2][0] = m[1][0] * m[2][1] * m[3][3] - m[1][0] * m[2][3] * m[3][1] - m[2][0] * m[1][1] * m[3][3] +
				m[2][0] * m[1][3] * m[3][1] + m[3][0] * m[1][1] * m[2][3] - m[3][0] * m[1][3] * m[2][1];
	inv[3][0] = -m[1][0] * m[2][1] * m[3][2] + m[1][0] * m[2][2] * m[3][1] + m[2][0] * m[1][1] * m[3][2] -
				m[2][0] * m[1][2] * m[3][1] - m[3][0] * m[1][1] * m[2][2] + m[3][0] * m[1][2] * m[2][1];

	det = m[0][0] * inv[0][0] + m[0][1] * inv[1][0] + m[0][2] * inv[2][0] + m[0][3] * inv[3][0];

	if (std::abs(det) < EPSILON) {
		return { 1.0f };
	}

	inv[0][1] = -m[0][1] * m[2][2] * m[3][3] + m[0][1] * m[2][3] * m[3][2] + m[2][1] * m[0][2] * m[3][3] -
				m[2][1] * m[0][3] * m[3][2] - m[3][1] * m[0][2] * m[2][3] + m[3][1] * m[0][3] * m[2][2];
	inv[1][1] = m[0][0] * m[2][2] * m[3][3] - m[0][0] * m[2][3] * m[3][2] - m[2][0] * m[0][2] * m[3][3] +
				m[2][0] * m[0][3] * m[3][2] + m[3][0] * m[0][2] * m[2][3] - m[3][0] * m[0][3] * m[2][2];
	inv[2][1] = -m[0][0] * m[2][1] * m[3][3] + m[0][0] * m[2][3] * m[3][1] + m[2][0] * m[0][1] * m[3][3] -
				m[2][0] * m[0][3] * m[3][1] - m[3][0] * m[0][1] * m[2][3] + m[3][0] * m[0][3] * m[2][1];
	inv[3][1] = m[0][0] * m[2][1] * m[3][2] - m[0][0] * m[2][2] * m[3][1] - m[2][0] * m[0][1] * m[3][2] +
				m[2][0] * m[0][2] * m[3][1] + m[3][0] * m[0][1] * m[2][2] - m[3][0] * m[0][2] * m[2][1];
	inv[0][2] = m[0][1] * m[1][2] * m[3][3] - m[0][1] * m[1][3] * m[3][2] - m[1][1] * m[0][2] * m[3][3] +
				m[1][1] * m[0][3] * m[3][2] + m[3][1] * m[0][2] * m[1][3] - m[3][1] * m[0][3] * m[1][2];
	inv[1][2] = -m[0][0] * m[1][2] * m[3][3] + m[0][0] * m[1][3] * m[3][2] + m[1][0] * m[0][2] * m[3][3] -
				m[1][0] * m[0][3] * m[3][2] - m[3][0] * m[0][2] * m[1][3] + m[3][0] * m[0][3] * m[1][2];
	inv[2][2] = m[0][0] * m[1][1] * m[3][3] - m[0][0] * m[1][3] * m[3][1] - m[1][0] * m[0][1] * m[3][3] +
				m[1][0] * m[0][3] * m[3][1] + m[3][0] * m[0][1] * m[1][3] - m[3][0] * m[0][3] * m[1][1];
	inv[3][2] = -m[0][0] * m[1][1] * m[3][2] + m[0][0] * m[1][2] * m[3][1] + m[1][0] * m[0][1] * m[3][2] -
				m[1][0] * m[0][2] * m[3][1] - m[3][0] * m[0][1] * m[1][2] + m[3][0] * m[0][2] * m[1][1];
	inv[0][3] = -m[0][1] * m[1][2] * m[2][3] + m[0][1] * m[1][3] * m[2][2] + m[1][1] * m[0][2] * m[2][3] -
				m[1][1] * m[0][3] * m[2][2] - m[2][1] * m[0][2] * m[1][3] + m[2][1] * m[0][3] * m[1][2];
	inv[1][3] = m[0][0] * m[1][2] * m[2][3] - m[0][0] * m[1][3] * m[2][2] - m[1][0] * m[0][2] * m[2][3] +
				m[1][0] * m[0][3] * m[2][2] + m[2][0] * m[0][2] * m[1][3] - m[2][0] * m[0][3] * m[1][2];
	inv[2][3] = -m[0][0] * m[1][1] * m[2][3] + m[0][0] * m[1][3] * m[2][1] + m[1][0] * m[0][1] * m[2][3] -
				m[1][0] * m[0][3] * m[2][1] - m[2][0] * m[0][1] * m[1][3] + m[2][0] * m[0][3] * m[1][1];
	inv[3][3] = m[0][0] * m[1][1] * m[2][2] - m[0][0] * m[1][2] * m[2][1] - m[1][0] * m[0][1] * m[2][2] +
				m[1][0] * m[0][2] * m[2][1] + m[2][0] * m[0][1] * m[1][2] - m[2][0] * m[0][2] * m[1][1];

	det = 1.0f / det;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			inv[i][j] *= det;
		}
	}

	return inv;
}

// Projection Matrices (Vulkan-specific)

/// Create an orthographic projection matrix for Vulkan
/// @param left Left clipping plane
/// @param right Right clipping plane
/// @param bottom Bottom clipping plane
/// @param top Top clipping plane
/// @param zNear Near clipping plane (must be positive and < zFar)
/// @param zFar Far clipping plane (must be positive and > zNear)
/// @return Orthographic projection matrix with [0,1] depth range
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

/// Create a perspective projection matrix for Vulkan

/// @param fovY Field of view in radians
/// @param aspect Aspect ratio (width/height)
/// @param zNear Near clipping plane (must be positive and < zFar)
/// @param zFar Far clipping plane (must be positive and > zNear)
/// @return Perspective projection matrix with [0,1] depth range
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

/// Create an infinite perspective projection (far plane at infinity)
/// Useful for sky boxes and avoiding far plane clipping
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

/// Build a view matrix from position and target

/// @param position Camera position
/// @param target Point to look at
/// @param up Up vector
/// @return View matrix
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

/// Build a view matrix from position and direction
/// @param position Camera position
/// @param direction Forward direction (normalized)
/// @param worldUp World up vector
/// @return View matrix
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

/// Build a view matrix from Euler angles (YXZ order)

/// @param position Camera position
/// @param rotation Rotation in radians (pitch, yaw, roll)
/// @return View matrix
inline mat4 ViewFromEuler(const vec3 &position, const vec3 &rotation) {
	const f32 c3 = std::cos(rotation.z);
	const f32 s3 = std::sin(rotation.z);
	const f32 c2 = std::cos(rotation.x);
	const f32 s2 = std::sin(rotation.x);
	const f32 c1 = std::cos(rotation.y);
	const f32 s1 = std::sin(rotation.y);

	const vec3 u = { (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
	const vec3 v = { (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
	const vec3 w = { (c2 * s1), (-s2), (c1 * c2) };

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

/// Extract frustum corners from a projection-view matrix
inline std::array<vec4, 8> ExtractFrustumCorners(const mat4 &projView) {
	mat4 inv = Inverse(projView);
	std::array<vec4, 8> corners;
	int idx = 0;

	for (int z = 0; z < 2; ++z) {
		for (int y = 0; y < 2; ++y) {
			for (int x = 0; x < 2; ++x) {
				f32 nx = x == 0 ? -1.0f : 1.0f;
				f32 ny = y == 0 ? -1.0f : 1.0f;
				f32 nz = z == 0 ? 0.0f : 1.0f;
				vec4 pt = MatMulVec(inv, vec4(nx, ny, nz, 1.0f));
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
		vec4 lightSpacePos = MatMulVec(lightView, corner);
		vec3 lsPos = vec3(lightSpacePos.x, lightSpacePos.y, lightSpacePos.z);
		outMin = Min(outMin, lsPos);
		outMax = Max(outMax, lsPos);
	}
}

inline mat4 BuildLightViewMatrix(const vec3 &lightDirection, const vec3 &focusPoint, const f32 distance = 100.0f) {
	const vec3 lightDir = Normalize(lightDirection);

	// Choose stable up vector
	vec3 worldUp = vec3(0.0f, 1.0f, 0.0f);
	if (std::abs(Dot(lightDir, worldUp)) > 0.99f) {
		worldUp = vec3(0.0f, 0.0f, 1.0f);
	}

	// Build orthonormal basis
	const vec3 w = Normalize(lightDir);
	const vec3 u = Normalize(Cross(w, worldUp));
	const vec3 v = Cross(w, u);

	// Position light back from focus point
	vec3 lightPos = focusPoint - lightDir * distance;

	// Build view matrix
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

/// Snap shadow map bounds to texel-sized increments for stability
/// Prevents shadow shimmering when camera moves
inline void SnapToTexelGrid(f32 &min, f32 &max, const f32 worldUnitsPerTexel) {
	min = std::floor(min / worldUnitsPerTexel) * worldUnitsPerTexel;
	max = std::floor(max / worldUnitsPerTexel) * worldUnitsPerTexel;
}

/// Handle negative Z bounds for shadow mapping
/// Returns the Z offset that needs to be applied to the light view matrix
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

/// Clamp a value between min and max
template <typename T> inline T Clamp(T value, T min, T max) {
	return std::max(min, std::min(value, max));
}

/// Linear interpolation
inline f32 Lerp(const f32 a, const f32 b, const f32 t) {
	return a + t * (b - a);
}

/// Smooth step interpolation (cubic Hermite)
inline f32 SmoothStep(const f32 edge0, const f32 edge1, const f32 x) {
	const f32 t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

/// Check if two floats are approximately equal
inline bool ApproxEqual(const f32 a, const f32 b, const f32 epsilon = EPSILON) {
	return std::abs(a - b) < epsilon;
}

/// Convert degrees to radians
inline f32 Radians(const f32 degrees) {
	return degrees * DEG_TO_RAD;
}

/// Convert radians to degrees
inline f32 Degrees(const f32 radians) {
	return radians * RAD_TO_DEG;
}

/// Modulo operation that works correctly with negative numbers
inline f32 Mod(f32 a, f32 b) {
	f32 result = std::fmod(a, b);
	if (result < 0.0f)
		result += b;
	return result;
}

/// Normalize an angle to [0, 2π) range
inline f32 NormalizeAngle(f32 angle) {
	angle = Mod(angle, TAU);
	if (angle < 0.0f)
		angle += TAU;
	return angle;
}

/// Get the shortest angular difference between two angles
inline f32 AngleDifference(const f32 a, const f32 b) {
	const f32 diff = Mod(b - a + PI, TAU) - PI;
	return diff < -PI ? diff + TAU : diff;
}

/// Calculate distance from point to plane
inline f32 DistanceToPlane(const vec3 &point, const vec3 &planeNormal, const vec3 &planePoint) {
	return Dot(planeNormal, point - planePoint);
}

/// Project a point onto a plane
inline vec3 ProjectOntoPlane(const vec3 &point, const vec3 &planeNormal, const vec3 &planePoint) {
	f32 distance = DistanceToPlane(point, planeNormal, planePoint);
	return point - planeNormal * distance;
}

/// Calculate barycentric coordinates of a point in a triangle
inline vec3 Barycentric(const vec3 &p, const vec3 &a, const vec3 &b, const vec3 &c) {
	const vec3 v0 = b - a;
	const vec3 v1 = c - a;
	const vec3 v2 = p - a;

	const f32 d00 = Dot(v0, v0);
	const f32 d01 = Dot(v0, v1);
	const f32 d11 = Dot(v1, v1);
	const f32 d20 = Dot(v2, v0);
	const f32 d21 = Dot(v2, v1);

	const f32 denom = d00 * d11 - d01 * d01;
	if (std::abs(denom) < EPSILON)
		return vec3(0.0f);

	const f32 v = (d11 * d20 - d01 * d21) / denom;
	const f32 w = (d00 * d21 - d01 * d20) / denom;
	const f32 u = 1.0f - v - w;

	return vec3(u, v, w);
}

/// Calculate signed volume of a tetrahedron (for winding order tests)
inline f32 SignedVolume(const vec3 &a, const vec3 &b, const vec3 &c, const vec3 &d) {
	return Dot(Cross(b - a, c - a), d - a) / 6.0f;
}

} // namespace Aquila::Math

#endif // AQUILA_MATH_H

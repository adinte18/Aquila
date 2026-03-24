#ifndef AQUILA_GEOMETRY_RAY_H
#define AQUILA_GEOMETRY_RAY_H

#include "Aquila/Foundation/Math/Math.h"

namespace Aquila::Math::Geometry {

struct Ray {
	vec3 origin;
	vec3 direction;

	Ray(const vec3 &o, const vec3 &d) : origin(o), direction(normalize(d)) {}

	vec3 GetPoint(const f32 t) const { return origin + direction * t; }

	bool IntersectAABB(const vec3 &aabbMin, const vec3 &aabbMax, f32 &distance) const {
		const vec3 invDir = 1.0f / direction;
		const vec3 t1 = (aabbMin - origin) * invDir;
		const vec3 t2 = (aabbMax - origin) * invDir;

		const vec3 tMin = min(t1, t2);
		const vec3 tMax = max(t1, t2);

		const f32 tNear = glm::max(glm::max(tMin.x, tMin.y), tMin.z);
		const f32 tFar = glm::min(glm::min(tMax.x, tMax.y), tMax.z);

		if (tNear > tFar || tFar < 0.0f) {
			return false;
		}

		distance = tNear > 0.0f ? tNear : tFar;
		return true;
	}

	bool IntersectSphere(const vec3 &center, const f32 radius, f32 &distance) const {
		const vec3 oc = origin - center;
		const f32 a = dot(direction, direction);
		const f32 b = 2.0f * dot(oc, direction);
		const f32 c = dot(oc, oc) - radius * radius;

		const f32 discriminant = b * b - 4 * a * c;

		if (discriminant < 0) {
			return false;
		}

		const f32 t1 = (-b - sqrt(discriminant)) / (2.0f * a);
		const f32 t2 = (-b + sqrt(discriminant)) / (2.0f * a);

		if (t1 > 0) {
			distance = t1;
			return true;
		}
		if (t2 > 0) {
			distance = t2;
			return true;
		}

		return false;
	}

	bool IntersectTriangle(const vec3 &v0, const vec3 &v1, const vec3 &v2, f32 &distance) const {
		vec3 edge1, edge2, h, s, q;
		f32 a, f, u, v;

		edge1 = v1 - v0;
		edge2 = v2 - v0;
		h = cross(direction, edge2);
		a = dot(edge1, h);

		if (a > -EPSILON && a < EPSILON) {
			return false;
		}

		f = 1.0f / a;
		s = origin - v0;
		u = f * dot(s, h);

		if (u < 0.0f || u > 1.0f) {
			return false;
		}

		q = cross(s, edge1);
		v = f * dot(direction, q);

		if (v < 0.0f || u + v > 1.0f) {
			return false;
		}

		if (f32 t = f * dot(edge2, q); t > EPSILON) {
			distance = t;
			return true;
		}

		return false;
	}

	bool IntersectLine(const vec3 &lineStart, const vec3 &lineEnd, f32 threshold, f32 &distance) const {
		const vec3 lineDir = lineEnd - lineStart;
		const vec3 rayToLine = lineStart - origin;

		const vec3 cross1 = cross(direction, lineDir);
		const vec3 cross2 = cross(rayToLine, lineDir);

		const f32 denominator = dot(cross1, cross1);

		if (denominator < 1e-6f) {
			return false;
		}

		const f32 t = dot(cross2, cross1) / denominator;
		const f32 u = dot(cross(rayToLine, direction), cross1) / denominator;

		if (t < 0.0f || u < 0.0f || u > 1.0f) {
			return false;
		}

		const vec3 closestPointOnRay = origin + t * direction;
		const vec3 closestPointOnLine = lineStart + u * lineDir;

		if (const f32 dist = length(closestPointOnRay - closestPointOnLine); dist <= threshold) {
			distance = t;
			return true;
		}

		return false;
	}

	bool IntersectPlane(const vec3 &planeNormal, f32 planeDistance, f32 &distance) const {
		const f32 denom = dot(planeNormal, direction);

		if (abs(denom) < 1e-6f) {
			return false;
		}

		if (const f32 t = (planeDistance - dot(planeNormal, origin)) / denom; t >= 0) {
			distance = t;
			return true;
		}

		return false;
	}

	bool IntersectCylinder(const vec3 &cylinderStart, const vec3 &cylinderEnd, f32 radius, f32 &distance) const {
		const vec3 cylinderAxis = normalize(cylinderEnd - cylinderStart);
		const vec3 toRayOrigin = origin - cylinderStart;

		const vec3 rayDirPerp = direction - dot(direction, cylinderAxis) * cylinderAxis;
		const vec3 toRayOriginPerp = toRayOrigin - dot(toRayOrigin, cylinderAxis) * cylinderAxis;

		const f32 a = dot(rayDirPerp, rayDirPerp);
		const f32 b = 2.0f * dot(toRayOriginPerp, rayDirPerp);
		const f32 c = dot(toRayOriginPerp, toRayOriginPerp) - radius * radius;

		const f32 discriminant = b * b - 4 * a * c;

		if (discriminant < 0) {
			return false;
		}

		const f32 t1 = (-b - sqrt(discriminant)) / (2.0f * a);
		const f32 t2 = (-b + sqrt(discriminant)) / (2.0f * a);

		auto isWithinCylinder = [&](const f32 t) -> bool {
			const vec3 point = GetPoint(t);
			const vec3 toPoint = point - cylinderStart;
			const f32 projection = dot(toPoint, cylinderAxis);
			const f32 cylinderLength = length(cylinderEnd - cylinderStart);
			return projection >= 0.0f && projection <= cylinderLength;
		};

		if (t1 > 0 && isWithinCylinder(t1)) {
			distance = t1;
			return true;
		}
		if (t2 > 0 && isWithinCylinder(t2)) {
			distance = t2;
			return true;
		}

		return false;
	}

	vec3 ClosestPointTo(const vec3 &point) const {
		const vec3 toPoint = point - origin;
		f32 t = dot(toPoint, direction);
		return GetPoint(glm::max(0.0f, t));
	}

	f32 DistanceToPoint(const vec3 &point) const { return length(point - ClosestPointTo(point)); }
};

inline Ray ScreenToWorldRay(const vec2 &screenPos, const vec2 &viewportSize, const mat4 &viewMatrix,
							const mat4 &projMatrix) {
	vec2 ndc;
	ndc.x = (2.0f * screenPos.x) / viewportSize.x - 1.0f;
	ndc.y = 1.0f - (2.0f * screenPos.y) / viewportSize.y;

	mat4 invViewProj = inverse(projMatrix * viewMatrix);

	auto nearPoint = vec4(ndc.x, ndc.y, 0.0f, 1.0f);
	auto farPoint = vec4(ndc.x, ndc.y, 1.0f, 1.0f);

	vec4 worldNear = invViewProj * nearPoint;
	vec4 worldFar = invViewProj * farPoint;

	worldNear /= worldNear.w;
	worldFar /= worldFar.w;

	auto rayOrigin = vec3(worldNear);
	auto rayDirection = normalize(vec3(worldFar) - vec3(worldNear));

	return Ray(rayOrigin, rayDirection);
}

inline Ray CameraRay(const vec2 &screenPos, const vec2 &viewportSize, const vec3 &cameraPos, const vec3 &cameraForward,
					 const vec3 &cameraUp, const vec3 &cameraRight, const f32 fov, const f32 aspectRatio) {
	const f32 x = (2.0f * screenPos.x) / viewportSize.x - 1.0f;
	const f32 y = 1.0f - (2.0f * screenPos.y) / viewportSize.y;

	const f32 tanHalfFov = tan(glm::radians(fov) / 2.0f);
	const vec3 rayDir =
		normalize(cameraForward + (x * aspectRatio * tanHalfFov) * cameraRight + (y * tanHalfFov) * cameraUp);

	return Ray(cameraPos, rayDir);
}

} // namespace Aquila::Math::Geometry

#endif

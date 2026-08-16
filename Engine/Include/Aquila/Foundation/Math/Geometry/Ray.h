#ifndef AQUILA_GEOMETRY_RAY_H
#define AQUILA_GEOMETRY_RAY_H

#include "Aquila/Foundation/Math/Math.h"

namespace Aquila::Math::Geometry::Ray {

struct Ray {
	Vec3 origin;
	Vec3 direction;

	Ray(const Vec3 &o, const Vec3 &d) : origin(o), direction(normalize(d)) {}

	Vec3 get_point(const F32 t) const { return origin + direction * t; }

	bool intersect_aabb(const Vec3 &aabb_min, const Vec3 &aabb_max, F32 &distance) const {
		const Vec3 inv_dir = 1.0f / direction;
		const Vec3 t1 = (aabb_min - origin) * inv_dir;
		const Vec3 t2 = (aabb_max - origin) * inv_dir;

		const Vec3 t_min = Math::min(t1, t2);
		const Vec3 t_max = Math::max(t1, t2);

		const F32 t_near = Math::max(Math::max(t_min.x, t_min.y), t_min.z);
		const F32 t_far = Math::min(Math::min(t_max.x, t_max.y), t_max.z);

		if (t_near > t_far || t_far < 0.0f) {
			return false;
		}

		distance = t_near > 0.0f ? t_near : t_far;
		return true;
	}

	bool intersect_sphere(const Vec3 &center, const F32 radius, F32 &distance) const {
		const Vec3 oc = origin - center;
		const F32 a = Math::dot(direction, direction);
		const F32 b = 2.0f * Math::dot(oc, direction);
		const F32 c = Math::dot(oc, oc) - radius * radius;

		const F32 discriminant = b * b - 4 * a * c;

		if (discriminant < 0) {
			return false;
		}

		const F32 t1 = (-b - Math::sqrt(discriminant)) / (2.0f * a);
		const F32 t2 = (-b + Math::sqrt(discriminant)) / (2.0f * a);

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

	bool intersect_triangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2, F32 &distance) const {
		Vec3 edge1;
		Vec3 edge2;
		Vec3 h;
		Vec3 s;
		Vec3 q;

		F32 a = NAN;
		F32 f = NAN;
		F32 u = NAN;
		F32 v = NAN;

		edge1 = v1 - v0;
		edge2 = v2 - v0;
		h = Math::cross(direction, edge2);
		a = Math::dot(edge1, h);

		if (a > -Math::EPSILON && a < Math::EPSILON) {
			return false;
		}

		f = 1.0f / a;
		s = origin - v0;
		u = f * Math::dot(s, h);

		if (u < 0.0f || u > 1.0f) {
			return false;
		}

		q = Math::cross(s, edge1);
		v = f * Math::dot(direction, q);

		if (v < 0.0f || u + v > 1.0f) {
			return false;
		}

		if (F32 t = f * Math::dot(edge2, q); t > Aquila::Math::EPSILON) {
			distance = t;
			return true;
		}

		return false;
	}

	bool intersect_line(const Vec3 &line_start, const Vec3 &line_end, F32 threshold, F32 &distance) const {
		const Vec3 line_dir = line_end - line_start;
		const Vec3 ray_to_line = line_start - origin;

		const Vec3 cross1 = Math::cross(direction, line_dir);
		const Vec3 cross2 = Math::cross(ray_to_line, line_dir);

		const F32 denominator = Math::dot(cross1, cross1);

		if (denominator < Math::EPSILON) {
			return false;
		}

		const F32 t = Math::dot(cross2, cross1) / denominator;
		const F32 u = Math::dot(Math::cross(ray_to_line, direction), cross1) / denominator;

		if (t < 0.0f || u < 0.0f || u > 1.0f) {
			return false;
		}

		const Vec3 closest_point_on_ray = origin + t * direction;
		const Vec3 closest_point_on_line = line_start + u * line_dir;

		if (const F32 dist = length(closest_point_on_ray - closest_point_on_line); dist <= threshold) {
			distance = t;
			return true;
		}

		return false;
	}

	bool intersect_plane(const Vec3 &plane_normal, F32 plane_distance, F32 &distance) const {
		const F32 denom = Math::dot(plane_normal, direction);

		if (abs(denom) < 1e-6f) {
			return false;
		}

		if (const F32 t = (plane_distance - Math::dot(plane_normal, origin)) / denom; t >= 0) {
			distance = t;
			return true;
		}

		return false;
	}

	bool intersect_cylinder(const Vec3 &cylinder_start, const Vec3 &cylinder_end, F32 radius, F32 &distance) const {
		const Vec3 cylinder_axis = normalize(cylinder_end - cylinder_start);
		const Vec3 to_ray_origin = origin - cylinder_start;

		const Vec3 ray_dir_perp = direction - Math::dot(direction, cylinder_axis) * cylinder_axis;
		const Vec3 to_ray_origin_perp = to_ray_origin - Math::dot(to_ray_origin, cylinder_axis) * cylinder_axis;

		const F32 a = Math::dot(ray_dir_perp, ray_dir_perp);
		const F32 b = 2.0f * Math::dot(to_ray_origin_perp, ray_dir_perp);
		const F32 c = Math::dot(to_ray_origin_perp, to_ray_origin_perp) - radius * radius;

		const F32 discriminant = b * b - 4 * a * c;

		if (discriminant < 0) {
			return false;
		}

		const F32 t1 = (-b - sqrt(discriminant)) / (2.0f * a);
		const F32 t2 = (-b + sqrt(discriminant)) / (2.0f * a);

		auto is_within_cylinder = [&](const F32 t) -> bool {
			const Vec3 point = get_point(t);
			const Vec3 to_point = point - cylinder_start;
			const F32 projection = Math::dot(to_point, cylinder_axis);
			const F32 cylinder_length = length(cylinder_end - cylinder_start);
			return projection >= 0.0f && projection <= cylinder_length;
		};

		if (t1 > 0 && is_within_cylinder(t1)) {
			distance = t1;
			return true;
		}
		if (t2 > 0 && is_within_cylinder(t2)) {
			distance = t2;
			return true;
		}

		return false;
	}

	Vec3 closest_point_to(const Vec3 &point) const {
		const Vec3 to_point = point - origin;
		F32 t = Math::dot(to_point, direction);
		return get_point(Math::max(0.0f, t));
	}

	F32 distance_to_point(const Vec3 &point) const { return length(point - closest_point_to(point)); }
};

inline Ray screen_to_world_ray(const Vec2 &screen_pos, const Vec2 &viewport_size, const Mat4 &view_matrix,
							   const Mat4 &proj_matrix) {
	Vec2 ndc;
	ndc.x = (2.0f * screen_pos.x) / viewport_size.x - 1.0f;
	ndc.y = 1.0f - (2.0f * screen_pos.y) / viewport_size.y;

	Mat4 inv_view_proj = inverse(proj_matrix * view_matrix);

	auto near_point = Vec4(ndc.x, ndc.y, 0.0f, 1.0f);
	auto far_point = Vec4(ndc.x, ndc.y, 1.0f, 1.0f);

	Vec4 world_near = inv_view_proj * near_point;
	Vec4 world_far = inv_view_proj * far_point;

	world_near /= world_near.w;
	world_far /= world_far.w;

	auto ray_origin = Vec3(world_near);
	auto ray_direction = normalize(Vec3(world_far) - Vec3(world_near));

	return Ray(ray_origin, ray_direction);
}

inline Ray camera_ray(const Vec2 &screen_pos, const Vec2 &viewport_size, const Vec3 &camera_pos,
					  const Vec3 &camera_forward, const Vec3 &camera_up, const Vec3 &camera_right, const F32 fov,
					  const F32 aspect_ratio) {
	const F32 x = (2.0f * screen_pos.x) / viewport_size.x - 1.0f;
	const F32 y = 1.0f - (2.0f * screen_pos.y) / viewport_size.y;

	const F32 tan_half_fov = tan(Math::radians(fov) / 2.0f);
	const Vec3 ray_dir =
		normalize(camera_forward + (x * aspect_ratio * tan_half_fov) * camera_right + (y * tan_half_fov) * camera_up);

	return Ray(camera_pos, ray_dir);
}

} // namespace Aquila::Math::Geometry::Ray

#endif

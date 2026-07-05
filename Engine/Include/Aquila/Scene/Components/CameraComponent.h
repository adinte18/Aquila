#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Aquila::SceneManagement::Components {
using namespace glm;

struct CameraComponent {
	F32 fov = 80.0f;
	F32 near_plane = 0.1f;
	F32 far_plane = 100.F;
	F32 aspect_ratio = 1.778f;
	bool is_orthographic = false;

	F32 ortho_left = -1.0f;
	F32 ortho_right = 1.0f;
	F32 ortho_top = 1.0f;
	F32 ortho_bottom = -1.0f;

	bool primary = false;

	Mat4 get_view_matrix(const Vec3 &position, const quat &rotation) const {
		Vec3 forward = rotation * Vec3(0.0f, 0.0f, 1.0f);
		Vec3 up = rotation * Vec3(0.0f, -1.0f, 0.0f);

		return get_view_matrix_from_direction(position, forward, up);
	}

	Mat4 get_view_matrix(const Vec3 &position, const Vec3 &target, const Vec3 &up = Vec3(0, -1, 0)) const {
		const Vec3 direction = glm::normalize(target - position);
		return get_view_matrix_from_direction(position, direction, up);
	}

	F32 get_near_plane() const { return near_plane; }

	F32 get_far_plane() const { return far_plane; }

	Mat4 get_view_matrix_from_direction(const Vec3 &position, const Vec3 &direction,
										const Vec3 &up = Vec3(0, -1, 0)) const {
		const Vec3 w{ glm::normalize(direction) };
		const Vec3 u{ glm::normalize(glm::cross(w, up)) };
		const Vec3 v{ glm::cross(w, u) };

		Mat4 view_matrix = Mat4{ 1.0f };
		view_matrix[0][0] = u.x;
		view_matrix[1][0] = u.y;
		view_matrix[2][0] = u.z;
		view_matrix[0][1] = v.x;
		view_matrix[1][1] = v.y;
		view_matrix[2][1] = v.z;
		view_matrix[0][2] = w.x;
		view_matrix[1][2] = w.y;
		view_matrix[2][2] = w.z;
		view_matrix[3][0] = -glm::dot(u, position);
		view_matrix[3][1] = -glm::dot(v, position);
		view_matrix[3][2] = -glm::dot(w, position);

		return view_matrix;
	}

	Mat4 get_inverse_view_matrix(const Vec3 &position, const quat &rotation) const {
		Vec3 forward = rotation * Vec3(0.0f, 0.0f, 1.0f);
		Vec3 up = rotation * Vec3(0.0f, -1.0f, 0.0f);
		Vec3 right = glm::cross(forward, up);

		Mat4 inverse_view_matrix = Mat4{ 1.0f };
		inverse_view_matrix[0][0] = right.x;
		inverse_view_matrix[0][1] = right.y;
		inverse_view_matrix[0][2] = right.z;
		inverse_view_matrix[1][0] = up.x;
		inverse_view_matrix[1][1] = up.y;
		inverse_view_matrix[1][2] = up.z;
		inverse_view_matrix[2][0] = forward.x;
		inverse_view_matrix[2][1] = forward.y;
		inverse_view_matrix[2][2] = forward.z;
		inverse_view_matrix[3][0] = position.x;
		inverse_view_matrix[3][1] = position.y;
		inverse_view_matrix[3][2] = position.z;

		return inverse_view_matrix;
	}

	Mat4 get_view_matrix_from_quaternion(const Vec3 &position, const quat &rotation) const {
		Mat4 rotation_matrix = mat4_cast(conjugate(rotation));
		Mat4 translation_matrix = Mat4(1.0f);
		translation_matrix[3] = Vec4(-position, 1.0f);
		return rotation_matrix * translation_matrix;
	}

	Mat4 get_projection_matrix() const {
		if (is_orthographic) {
			Mat4 proj_matrix = Mat4{ 1.0f };
			proj_matrix[0][0] = 2.0f / (ortho_right - ortho_left);
			proj_matrix[1][1] = 2.0f / (ortho_top - ortho_bottom);
			proj_matrix[2][2] = 1.0f / (far_plane - near_plane);
			proj_matrix[3][0] = -(ortho_right + ortho_left) / (ortho_right - ortho_left);
			proj_matrix[3][1] = -(ortho_top + ortho_bottom) / (ortho_top - ortho_bottom);
			proj_matrix[3][2] = -near_plane / (far_plane - near_plane);
			return proj_matrix;
		} else {
			const F32 tan_half_fovy = tan(glm::radians(fov) / 2.0f);
			Mat4 proj_matrix = Mat4{ 0.0f };
			proj_matrix[0][0] = 1.0f / (aspect_ratio * tan_half_fovy);
			proj_matrix[1][1] = 1.0f / (tan_half_fovy);
			proj_matrix[2][2] = far_plane / (far_plane - near_plane);
			proj_matrix[2][3] = 1.0f;
			proj_matrix[3][2] = -(far_plane * near_plane) / (far_plane - near_plane);

			proj_matrix[1][1] *= -1.0f;

			return proj_matrix;
		}
	}

	Mat4 get_view_projection_matrix(const Vec3 &position, const quat &rotation) const {
		return get_projection_matrix() * get_view_matrix(position, rotation);
	}

	Mat4 get_view_projection_matrix(const Vec3 &position, const Vec3 &target, const Vec3 &up = Vec3(0, -1, 0)) const {
		return get_projection_matrix() * get_view_matrix(position, target, up);
	}

	void get_frustum_corners(const Vec3 &position, const quat &rotation, Vec3 corners[8]) const {
		Mat4 inv_vp = inverse(get_view_projection_matrix(position, rotation));

		Vec4 frustum_corners[8] = { { -1, -1, -1, 1 }, { 1, -1, -1, 1 }, { 1, 1, -1, 1 }, { -1, 1, -1, 1 },
									{ -1, -1, 1, 1 },  { 1, -1, 1, 1 },	 { 1, 1, 1, 1 },  { -1, 1, 1, 1 } };

		for (int i = 0; i < 8; ++i) {
			Vec4 world_pos = inv_vp * frustum_corners[i];
			corners[i] = Vec3(world_pos) / world_pos.w;
		}
	}

	Vec3 get_forward_direction(const quat &rotation) const { return rotation * Vec3(0.0f, 0.0f, 1.0f); }

	Vec3 get_right_direction(const quat &rotation) const { return rotation * Vec3(1.0f, 0.0f, 0.0f); }

	Vec3 get_up_direction(const quat &rotation) const { return rotation * Vec3(0.0f, -1.0f, 0.0f); }

	void on_resize(F32 width, F32 height) { aspect_ratio = width / height; }

	void set_fov(F32 new_fov) { fov = glm::clamp(new_fov, 1.0f, 179.0f); }

	void zoom(F32 offset) { set_fov(fov - offset); }

	Vec3 quaternion_to_euler(const quat &q) const {
		Vec3 euler = eulerAngles(q);
		return Vec3(degrees(euler.x), degrees(euler.y), degrees(euler.z));
	}

	quat euler_to_quaternion(const Vec3 &euler) const {
		return quat(Vec3(radians(euler.x), radians(euler.y), radians(euler.z)));
	}
};

} // namespace Aquila::SceneManagement::Components

#endif

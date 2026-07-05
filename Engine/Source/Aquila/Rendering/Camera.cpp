#include "Aquila/Rendering/Camera.h"

namespace Aquila::Rendering {

void Camera::speed_up() {
	if (!m_is_sped_up) {
		m_movement_speed *= 5.0f;
		m_is_sped_up = true;
	}
}

void Camera::reset_speed() {
	if (m_is_sped_up) {
		m_movement_speed /= 5.0f;
		m_is_sped_up = false;
	}
}

void Camera::move_forward(F32 delta) {
	Mat4 view_matrix = get_inverse_view();
	Vec3 forward = Math::normalize(Vec3(view_matrix[2]));
	Vec3 move_dir = forward;

	if (Math::dot(move_dir, move_dir) > Math::EPSILON) {
		m_position += m_movement_speed * delta * Math::normalize(move_dir);

		if (m_camera_type == CameraType::Free) {
			m_orbit_target = m_position + forward * m_orbit_radius;
		}
	}
}

void Camera::move_backward(F32 delta) {
	Mat4 view_matrix = get_inverse_view();
	Vec3 forward = Math::normalize(Vec3(view_matrix[2]));
	Vec3 move_dir = -forward;

	if (Math::dot(move_dir, move_dir) > Math::EPSILON) {
		m_position += m_movement_speed * delta * Math::normalize(move_dir);

		if (m_camera_type == CameraType::Free) {
			m_orbit_target = m_position + forward * m_orbit_radius;
		}
	}
}

void Camera::move_right(F32 delta) {
	Mat4 view_matrix = get_inverse_view();
	Vec3 right = Math::normalize(Vec3(view_matrix[0]));
	m_position += right * m_movement_speed * delta;

	if (m_camera_type == CameraType::Free) {
		Vec3 forward = Math::normalize(Vec3(view_matrix[2]));
		m_orbit_target = m_position + forward * m_orbit_radius;
	}
}

void Camera::move_left(F32 delta) {
	Mat4 view_matrix = get_inverse_view();
	Vec3 right = Math::normalize(Vec3(view_matrix[0]));
	m_position -= right * m_movement_speed * delta;

	if (m_camera_type == CameraType::Free) {
		Vec3 forward = Math::normalize(Vec3(view_matrix[2]));
		m_orbit_target = m_position + forward * m_orbit_radius;
	}
}

void Camera::rotate(const double yaw, const double pitch) {
	m_rotation.x += pitch;
	m_rotation.y += yaw;
	m_rotation.x = Math::clamp(m_rotation.x, -89.0f, 89.0f);

	set_view_yxz(m_position, m_rotation);

	if (m_camera_type == CameraType::Free) {
		update_free_mode_look_direction();
	}
}

void Camera::update_free_mode_look_direction() {
	const F32 c1 = std::cos(m_rotation.y);
	const F32 s1 = std::sin(m_rotation.y);
	const F32 c2 = std::cos(m_rotation.x);
	const F32 s2 = std::sin(m_rotation.x);

	Vec3 forward = Math::normalize(Vec3(c2 * s1, -s2, c1 * c2));
	m_orbit_target = m_position + forward * m_orbit_radius;
}

void Camera::zoom(const F32 offset, const F32 aspect_ratio) {
	m_fov -= offset;
	m_fov = Math::clamp(m_fov, 1.0f, 90.0f);
	set_perspective_projection(Math::radians(m_fov), aspect_ratio, m_near, m_far);
}

void Camera::on_resize(const F32 width, const F32 height) {
	set_perspective_projection(m_fov, width / height, m_near, m_far);
}

void Camera::set_orthographic_projection(F32 left, F32 right, F32 top, F32 bottom, F32 near_plane, F32 far_plane) {
	m_projection_matrix = Math::ortho_vulkan(left, right, bottom, top, near_plane, far_plane);
}

void Camera::set_perspective_projection(F32 fov_y, F32 aspect, F32 near_plane, F32 far_plane) {
	AQUILA_ASSERT(std::abs(aspect - Math::EPSILON) > 0.0f, "Aspect ratio must not be zero");

	this->m_aspect_ratio = aspect;
	this->m_fov = fov_y;
	this->m_near = near_plane;
	this->m_far = far_plane;
	m_projection_matrix = Math::perspective_vulkan(Math::radians(m_fov), aspect, near_plane, far_plane);
}

void Camera::set_view_direction(Vec3 position, Vec3 direction, Vec3 up) {
	m_view_matrix = Math::look_in_direction(position, direction, up);

	const Vec3 w = Math::normalize(direction);
	const Vec3 u = Math::normalize(Math::cross(w, up));
	const Vec3 v = Math::cross(w, u);

	m_inverse_view_matrix = Mat4{ 1.F };
	m_inverse_view_matrix[0][0] = u.x;
	m_inverse_view_matrix[0][1] = u.y;
	m_inverse_view_matrix[0][2] = u.z;
	m_inverse_view_matrix[1][0] = v.x;
	m_inverse_view_matrix[1][1] = v.y;
	m_inverse_view_matrix[1][2] = v.z;
	m_inverse_view_matrix[2][0] = w.x;
	m_inverse_view_matrix[2][1] = w.y;
	m_inverse_view_matrix[2][2] = w.z;
	m_inverse_view_matrix[3][0] = position.x;
	m_inverse_view_matrix[3][1] = position.y;
	m_inverse_view_matrix[3][2] = position.z;
}

void Camera::set_view_target(Vec3 position, Vec3 target, Vec3 up) {
	m_view_matrix = Math::look_at(position, target, up);

	const Vec3 w = Math::normalize(target - position);
	const Vec3 u = Math::normalize(Math::cross(w, up));
	const Vec3 v = Math::cross(w, u);

	m_inverse_view_matrix = Mat4{ 1.F };
	m_inverse_view_matrix[0][0] = u.x;
	m_inverse_view_matrix[0][1] = u.y;
	m_inverse_view_matrix[0][2] = u.z;
	m_inverse_view_matrix[1][0] = v.x;
	m_inverse_view_matrix[1][1] = v.y;
	m_inverse_view_matrix[1][2] = v.z;
	m_inverse_view_matrix[2][0] = w.x;
	m_inverse_view_matrix[2][1] = w.y;
	m_inverse_view_matrix[2][2] = w.z;
	m_inverse_view_matrix[3][0] = position.x;
	m_inverse_view_matrix[3][1] = position.y;
	m_inverse_view_matrix[3][2] = position.z;
}

void Camera::set_view_yxz(Vec3 position, Vec3 rotation) {
	m_view_matrix = Math::view_from_euler(position, rotation);

	const F32 c3 = std::cos(rotation.z);
	const F32 s3 = std::sin(rotation.z);
	const F32 c2 = std::cos(rotation.x);
	const F32 s2 = std::sin(rotation.x);
	const F32 c1 = std::cos(rotation.y);
	const F32 s1 = std::sin(rotation.y);
	const Vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
	const Vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
	const Vec3 w{ (c2 * s1), (-s2), (c1 * c2) };

	m_inverse_view_matrix = Mat4{ 1.F };
	m_inverse_view_matrix[0][0] = u.x;
	m_inverse_view_matrix[0][1] = u.y;
	m_inverse_view_matrix[0][2] = u.z;
	m_inverse_view_matrix[1][0] = v.x;
	m_inverse_view_matrix[1][1] = v.y;
	m_inverse_view_matrix[1][2] = v.z;
	m_inverse_view_matrix[2][0] = w.x;
	m_inverse_view_matrix[2][1] = w.y;
	m_inverse_view_matrix[2][2] = w.z;
	m_inverse_view_matrix[3][0] = position.x;
	m_inverse_view_matrix[3][1] = position.y;
	m_inverse_view_matrix[3][2] = position.z;
}

void Camera::set_orbit_target(const Vec3 &target) {
	m_orbit_target = target;
	Vec3 offset = m_position - m_orbit_target;

	m_orbit_radius = Math::length(offset);
	if (m_orbit_radius < 0.1f) {
		m_orbit_radius = 5.0f;
	}

	m_orbit_pitch = std::asin(offset.y / m_orbit_radius);
	m_orbit_yaw = std::atan2(offset.x, offset.z);

	update_orbit_position();
}

void Camera::orbit_rotate(const F32 delta_yaw, const F32 delta_pitch) {
	m_orbit_yaw += delta_yaw;
	m_orbit_pitch += delta_pitch;
	m_orbit_pitch = std::clamp(m_orbit_pitch, -Math::HALF_PI + 0.01f, Math::HALF_PI - 0.01f);
	update_orbit_position();
}

void Camera::orbit_zoom(const F32 delta_radius) {
	m_orbit_radius = std::max(m_orbit_radius + delta_radius, 0.1f);
	update_orbit_position();
}

void Camera::update_orbit_position() {
	m_position.x = m_orbit_target.x + m_orbit_radius * std::cos(m_orbit_pitch) * std::sin(m_orbit_yaw);
	m_position.y = m_orbit_target.y + m_orbit_radius * std::sin(m_orbit_pitch);
	m_position.z = m_orbit_target.z + m_orbit_radius * std::cos(m_orbit_pitch) * std::cos(m_orbit_yaw);

	recalculate_view();
}

void Camera::recalculate_view() {
	set_view_target(m_position, m_orbit_target, Vec3(0, -1, 0));
	m_direction = Math::normalize(m_orbit_target - m_position);

	Vec3 forward = m_direction;
	m_rotation.x = std::asin(-forward.y);
	m_rotation.y = std::atan2(forward.x, forward.z);
	m_rotation.z = 0.0f;
}

void Camera::switch_to_type(const CameraType new_type, const Vec3 target_pos) {
	if (m_camera_type != new_type) {
		CameraType old_type = m_camera_type;
		m_camera_type = new_type;

		AQUILA_LOG_DEBUG("Camera Type: {}", (int)get_type());

		if (m_camera_type == CameraType::Orbit) {
			if (old_type == CameraType::Free) {
				Mat4 view_matrix = get_inverse_view();
				Vec3 forward = Math::normalize(Vec3(view_matrix[2]));
				Vec3 new_target = m_position + forward * m_orbit_radius;
				set_orbit_target(new_target);
			} else {
				set_orbit_target(target_pos);
			}
			update_orbit_position();
		} else if (m_camera_type == CameraType::Free) {
			update_free_mode_look_direction();
		}
	}
}

} // namespace Aquila::Rendering

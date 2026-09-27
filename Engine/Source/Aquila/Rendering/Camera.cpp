#include "Aquila/Rendering/Camera.h"

namespace Aquila::Rendering {

void Camera::speed_up() {
	if (!m_is_sped_up) {
		m_movement_speed *= 5.0F;
		m_is_sped_up = true;
	}
}

void Camera::reset_speed() {
	if (m_is_sped_up) {
		m_movement_speed /= 5.0F;
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
	m_rotation.x = Math::clamp(m_rotation.x, -89.0F, 89.0F);

	set_view_yxz(m_position, m_rotation);

	if (m_camera_type == CameraType::Free) {
		update_free_mode_look_direction();
	}
}

void Camera::update_free_mode_look_direction() {
	const F32 c1 = Math::cos(m_rotation.y);
	const F32 s1 = Math::sin(m_rotation.y);
	const F32 c2 = Math::cos(m_rotation.x);
	const F32 s2 = Math::sin(m_rotation.x);

	Vec3 forward = Math::normalize(Vec3(c2 * s1, -s2, c1 * c2));
	m_orbit_target = m_position + forward * m_orbit_radius;
}

void Camera::zoom(const F32 offset, const F32 aspect_ratio) {
	m_fov -= offset;
	m_fov = Math::clamp(m_fov, 1.0F, 90.0F);
	set_perspective_projection(Math::radians(m_fov), aspect_ratio, m_near, m_far);
}

void Camera::on_resize(const F32 width, const F32 height) {
	set_perspective_projection(m_fov, width / height, m_near, m_far);
}

void Camera::set_orthographic_projection(F32 left, F32 right, F32 top, F32 bottom, F32 near_plane, F32 far_plane) {
	m_projection_matrix = Math::ortho_vulkan(left, right, bottom, top, near_plane, far_plane);
}

void Camera::set_perspective_projection(F32 fov_y, F32 aspect, F32 near_plane, F32 far_plane) {
	AQUILA_ASSERT(Math::abs(aspect - Math::EPSILON) > 0.0F, "Aspect ratio must not be zero");

	this->m_aspect_ratio = aspect;
	this->m_fov = fov_y;
	this->m_near = near_plane;
	this->m_far = far_plane;
	m_projection_matrix = Math::perspective_vulkan(Math::radians(m_fov), aspect, near_plane, far_plane);
}

void Camera::set_view_direction(Vec3 position, Vec3 direction, Vec3 up) {
	m_view_matrix = Math::look_in_direction(position, direction, up);
	m_inverse_view_matrix = Math::inverse_view(m_view_matrix, position);
}

void Camera::set_view_target(Vec3 position, Vec3 target, Vec3 up) {
	m_view_matrix = Math::look_at(position, target, up);
	m_inverse_view_matrix = Math::inverse_view(m_view_matrix, position);
}

void Camera::set_view_yxz(Vec3 position, Vec3 rotation) {
	m_view_matrix = Math::view_from_euler(position, rotation);
	m_inverse_view_matrix = Math::inverse_view(m_view_matrix, position);
}

void Camera::set_orbit_target(const Vec3 &target) {
	m_orbit_target = target;
	Vec3 offset = m_position - m_orbit_target;

	m_orbit_radius = Math::length(offset);
	if (m_orbit_radius < 0.1F) {
		m_orbit_radius = 5.0F;
	}

	m_orbit_pitch = std::asin(offset.y / m_orbit_radius);
	m_orbit_yaw = Math::atan2(offset.x, offset.z);

	update_orbit_position();
}

void Camera::orbit_rotate(const F32 delta_yaw, const F32 delta_pitch) {
	m_orbit_yaw += delta_yaw;
	m_orbit_pitch += delta_pitch;
	m_orbit_pitch = Math::clamp(m_orbit_pitch, -Math::HALF_PI + 0.01F, Math::HALF_PI - 0.01F);
	update_orbit_position();
}

void Camera::orbit_zoom(const F32 delta_radius) {
	m_orbit_radius = Math::max(m_orbit_radius + delta_radius, 0.1F);
	update_orbit_position();
}

void Camera::update_orbit_position() {
	m_position.x = m_orbit_target.x + m_orbit_radius * Math::cos(m_orbit_pitch) * Math::sin(m_orbit_yaw);
	m_position.y = m_orbit_target.y + m_orbit_radius * Math::sin(m_orbit_pitch);
	m_position.z = m_orbit_target.z + m_orbit_radius * Math::cos(m_orbit_pitch) * Math::cos(m_orbit_yaw);

	recalculate_view();
}

void Camera::recalculate_view() {
	set_view_target(m_position, m_orbit_target, Vec3(0, -1, 0));
	m_direction = Math::normalize(m_orbit_target - m_position);

	Vec3 forward = m_direction;
	m_rotation.x = std::asin(-forward.y);
	m_rotation.y = Math::atan2(forward.x, forward.z);
	m_rotation.z = 0.0F;
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

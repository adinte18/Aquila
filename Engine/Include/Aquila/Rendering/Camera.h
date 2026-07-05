#ifndef AQUILA_CAMERA_H
#define AQUILA_CAMERA_H

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Math/Math.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/Defines.h"

namespace Aquila::Rendering {
class Camera {
  public:
	enum class CameraType { Free, Orbit };

	void set_orthographic_projection(F32 left, F32 right, F32 top, F32 bottom, F32 near, F32 far);

	void set_perspective_projection(F32 fov_y, F32 aspect, F32 near, F32 far);

	[[nodiscard]] const Mat4 &get_projection() const { return m_projection_matrix; }
	[[nodiscard]] const Mat4 &get_view() const { return m_view_matrix; }
	[[nodiscard]] const Mat4 &get_inverse_view() const { return m_inverse_view_matrix; }

	void set_view_direction(Vec3 position, Vec3 direction, Vec3 up = Vec3(0.F, -1.F, -0.F));
	void set_view_target(Vec3 position, Vec3 target, Vec3 up = Vec3(0.F, -1.F, -0.F));

	void set_view_yxz(Vec3 position, Vec3 rotation);

	void speed_up();

	void reset_speed();

	void move_forward(F32 delta);
	void move_backward(F32 delta);
	void move_right(F32 delta);
	void move_left(F32 delta);
	void rotate(double yaw, double pitch);
	void zoom(F32 offset, F32 aspect_ratio);

	[[nodiscard]] Vec3 &get_position() { return m_position; }
	[[nodiscard]] Vec3 &get_rotation() { return m_rotation; }
	[[nodiscard]] Vec3 &get_direction() { return m_direction; }
	[[nodiscard]] const F32 &get_aspect_ratio() const { return m_aspect_ratio; }
	[[nodiscard]] F32 &get_rotation_speed() { return m_rotation_speed; }
	[[nodiscard]] F32 &get_movement_speed() { return m_movement_speed; }
	[[nodiscard]] F32 &get_fov() { return m_fov; }
	[[nodiscard]] F32 &get_near_plane() { return m_near; }
	[[nodiscard]] F32 &get_far_plane() { return m_far; }
	[[nodiscard]] CameraType get_type() const { return m_camera_type; }
	[[nodiscard]] bool &orbit_around_entity() { return m_orbit_around_entity; }
	[[nodiscard]] Vec3 get_target() const { return m_orbit_target; }
	[[nodiscard]] Vec3 get_right_vector() const { return glm::normalize(Vec3(m_view_matrix[0])); }
	[[nodiscard]] Vec3 get_up_vector() const { return glm::normalize(Vec3(m_view_matrix[1])); }
	[[nodiscard]] Vec3 get_forward_vector() const { return glm::normalize(Vec3(m_view_matrix[2])); }
	void set_position(const Vec3 pos) { m_position = pos; }
	void set_rotation_speed(const F32 speed) { m_rotation_speed = speed; }

	void set_orbit_target(const Vec3 &target);
	void orbit_rotate(F32 delta_yaw, F32 delta_pitch);
	void orbit_zoom(F32 delta_radius);
	void update_orbit_position();

	void switch_to_type(CameraType new_type, Vec3 target_pos = Vec3{ 0.F });

	void on_resize(F32 width, F32 height);
	void update_free_mode_look_direction();

	void recalculate_view();

  private:
	CameraType m_camera_type{ CameraType::Free };
	Mat4 m_projection_matrix{ 1.F };
	Mat4 m_view_matrix{ 1.F };
	Mat4 m_inverse_view_matrix{ 1.F };

	Vec3 m_position{ 0.0f };
	Vec3 m_rotation{ 0.0f };

	F32 m_movement_speed{ 5.0f };
	F32 m_rotation_speed{ 0.001f };

	Vec3 m_direction{ 0.0f, 0.0f, -1.0f };

	F32 m_fov{ 80.0f };
	F32 m_near{ 0.1f };
	F32 m_far{ 100.F };
	F32 m_aspect_ratio{ 0.F };

	bool m_is_sped_up{ false };

	Vec3 m_orbit_target{ 0.0f, 0.0f, 0.0f };
	F32 m_orbit_radius{ 10.0f };
	F32 m_orbit_yaw{ 0.0f };
	F32 m_orbit_pitch{ 0.0f };

	bool m_orbit_around_entity{ false };
};
} // namespace Aquila::Rendering

#endif // CAMERA_H

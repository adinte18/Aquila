#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/Signal.h"
#include <functional>
namespace Aquila::SceneManagement::Components {
struct TransformComponent {
  public:
	Signal<void()> on_changed;

	TransformComponent(const Vec3 &position = Vec3{ 0.F }, const glm::quat &rotation = glm::quat{ 1.F, 0.F, 0.F, 0.F },
					   const Vec3 &scale = Vec3{ 1.F })
		: m_local_position(position), m_local_rotation(rotation), m_local_scale(scale), m_world_matrix(1.0F),
		  m_world_matrix_dirty(true) {}

	void set_local_position(const Vec3 &position) {
		m_local_position = position;
		mark_world_matrix_dirty();
	}

	void set_local_rotation(const glm::quat &rotation) {
		m_local_rotation = rotation;
		mark_world_matrix_dirty();
	}

	void set_local_scale(const Vec3 &scale) {
		m_local_scale = scale;
		mark_world_matrix_dirty();
	}

	[[nodiscard]] const Vec3 &get_local_position() const { return m_local_position; }
	[[nodiscard]] const glm::quat &get_local_rotation() const { return m_local_rotation; }
	[[nodiscard]] const Vec3 &get_local_scale() const { return m_local_scale; }

	// Mutable versions only when you need to modify directly
	Vec3 &get_local_position_mut() {
		mark_world_matrix_dirty();
		return m_local_position;
	}
	glm::quat &get_local_rotation_mut() {
		mark_world_matrix_dirty();
		return m_local_rotation;
	}
	Vec3 &get_local_scale_mut() {
		mark_world_matrix_dirty();
		return m_local_scale;
	}

	[[nodiscard]] glm::mat4 get_local_transform_matrix() const {
		glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0F), m_local_position);
		glm::mat4 rotation_matrix = glm::toMat4(m_local_rotation);
		glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0F), m_local_scale);
		return translation_matrix * rotation_matrix * scale_matrix;
	}

	void update_world_matrix(const glm::mat4 &parent_matrix = glm::mat4(1.0F)) {
		m_world_matrix = parent_matrix * get_local_transform_matrix();
		m_world_matrix_dirty = false;
	}

	// Get world matrix (const version - use cached value)
	[[nodiscard]] const glm::mat4 &get_world_matrix() const {
		AQUILA_ASSERT(!m_world_matrix_dirty, "World matrix is dirty! Call UpdateWorldMatrix() before rendering.");
		return m_world_matrix;
	}

	// Get world matrix (mutable version - lazy update)
	// WARNING: Only use this outside of render loops!
	const glm::mat4 &get_world_matrix_lazy() {
		if (m_world_matrix_dirty) {
			update_world_matrix(m_parent_matrix);
		}
		return m_world_matrix;
	}

	// Extract world position from world matrix
	[[nodiscard]] Vec3 get_world_position() const { return Vec3(m_world_matrix[3]); }

	// Extract world scale from world matrix
	[[nodiscard]] Vec3 get_world_scale() const {
		Vec3 scale;
		scale.x = glm::length(Vec3(m_world_matrix[0]));
		scale.y = glm::length(Vec3(m_world_matrix[1]));
		scale.z = glm::length(Vec3(m_world_matrix[2]));
		return scale;
	}

	// Extract world rotation from world matrix
	[[nodiscard]] glm::quat get_world_rotation() const {
		Vec3 scale = get_world_scale();

		// Create rotation matrix by removing scale
		glm::mat3 rotation_matrix;
		rotation_matrix[0] = Vec3(m_world_matrix[0]) / scale.x;
		rotation_matrix[1] = Vec3(m_world_matrix[1]) / scale.y;
		rotation_matrix[2] = Vec3(m_world_matrix[2]) / scale.z;

		return glm::quat_cast(rotation_matrix);
	}

	[[nodiscard]] glm::mat3 get_normal_matrix() const {
		glm::mat3 normal_matrix = glm::mat3(m_world_matrix);
		return glm::transpose(glm::inverse(normal_matrix));
	}

	[[nodiscard]] glm::mat3 get_normal_matrix_fast() const { return glm::mat3(m_world_matrix); }

	void set_parent_matrix(const glm::mat4 &parent_matrix) {
		m_parent_matrix = parent_matrix;
		mark_world_matrix_dirty();
	}

	[[nodiscard]] bool is_world_matrix_dirty() const { return m_world_matrix_dirty; }

	void set_dirty_callback(std::function<void()> fn) { m_on_dirty = std::move(fn); }

  private:
	void mark_world_matrix_dirty() {
		m_world_matrix_dirty = true;
		if (m_on_dirty) {
			m_on_dirty();
		}
	}

	Vec3 m_local_position{ 0.F };
	glm::quat m_local_rotation{ 1.F, 0.F, 0.F, 0.F };
	Vec3 m_local_scale{ 1.F };
	glm::mat4 m_world_matrix{ 1.F };
	glm::mat4 m_parent_matrix{ 1.F };
	bool m_world_matrix_dirty{ true };
	std::function<void()> m_on_dirty;
};
} // namespace Aquila::SceneManagement::Components
#endif

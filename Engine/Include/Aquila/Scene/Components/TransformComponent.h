#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include "Aquila/Core/AquilaCore.h"
namespace Aquila::SceneManagement::Components {
struct TransformComponent {
  public:
	TransformComponent(const vec3 &position = vec3{ 0.f }, const glm::quat &rotation = glm::quat{ 1.f, 0.f, 0.f, 0.f },
					   const vec3 &scale = vec3{ 1.f })
		: m_LocalPosition(position), m_LocalRotation(rotation), m_LocalScale(scale), m_WorldMatrix(1.0f),
		  m_WorldMatrixDirty(true) {}

	void SetLocalPosition(const vec3 &position) {
		m_LocalPosition = position;
		MarkWorldMatrixDirty();
	}

	void SetLocalRotation(const glm::quat &rotation) {
		m_LocalRotation = rotation;
		MarkWorldMatrixDirty();
	}

	void SetLocalScale(const vec3 &scale) {
		m_LocalScale = scale;
		MarkWorldMatrixDirty();
	}

	[[nodiscard]] const vec3 &GetLocalPosition() const { return m_LocalPosition; }
	[[nodiscard]] const glm::quat &GetLocalRotation() const { return m_LocalRotation; }
	[[nodiscard]] const vec3 &GetLocalScale() const { return m_LocalScale; }

	// Mutable versions only when you need to modify directly
	vec3 &GetLocalPositionMut() {
		MarkWorldMatrixDirty();
		return m_LocalPosition;
	}
	glm::quat &GetLocalRotationMut() {
		MarkWorldMatrixDirty();
		return m_LocalRotation;
	}
	vec3 &GetLocalScaleMut() {
		MarkWorldMatrixDirty();
		return m_LocalScale;
	}

	[[nodiscard]] glm::mat4 GetLocalTransformMatrix() const {
		glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), m_LocalPosition);
		glm::mat4 rotationMatrix = glm::toMat4(m_LocalRotation);
		glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), m_LocalScale);
		return translationMatrix * rotationMatrix * scaleMatrix;
	}

	void UpdateWorldMatrix(const glm::mat4 &parentMatrix = glm::mat4(1.0f)) {
		m_WorldMatrix = parentMatrix * GetLocalTransformMatrix();
		m_WorldMatrixDirty = false;
	}

	// Get world matrix (const version - use cached value)
	[[nodiscard]] const glm::mat4 &GetWorldMatrix() const {
		AQUILA_ASSERT(!m_WorldMatrixDirty, "World matrix is dirty! Call UpdateWorldMatrix() before rendering.");
		return m_WorldMatrix;
	}

	// Get world matrix (mutable version - lazy update)
	// WARNING: Only use this outside of render loops!
	const glm::mat4 &GetWorldMatrixLazy() {
		if (m_WorldMatrixDirty) {
			UpdateWorldMatrix(m_ParentMatrix);
		}
		return m_WorldMatrix;
	}

	// Extract world position from world matrix
	[[nodiscard]] vec3 GetWorldPosition() const { return vec3(m_WorldMatrix[3]); }

	// Extract world scale from world matrix
	[[nodiscard]] vec3 GetWorldScale() const {
		vec3 scale;
		scale.x = glm::length(vec3(m_WorldMatrix[0]));
		scale.y = glm::length(vec3(m_WorldMatrix[1]));
		scale.z = glm::length(vec3(m_WorldMatrix[2]));
		return scale;
	}

	// Extract world rotation from world matrix
	[[nodiscard]] glm::quat GetWorldRotation() const {
		vec3 scale = GetWorldScale();

		// Create rotation matrix by removing scale
		glm::mat3 rotationMatrix;
		rotationMatrix[0] = vec3(m_WorldMatrix[0]) / scale.x;
		rotationMatrix[1] = vec3(m_WorldMatrix[1]) / scale.y;
		rotationMatrix[2] = vec3(m_WorldMatrix[2]) / scale.z;

		return glm::quat_cast(rotationMatrix);
	}

	[[nodiscard]] glm::mat3 GetNormalMatrix() const {
		glm::mat3 normalMatrix = glm::mat3(m_WorldMatrix);
		return glm::transpose(glm::inverse(normalMatrix));
	}

	[[nodiscard]] glm::mat3 GetNormalMatrixFast() const { return glm::mat3(m_WorldMatrix); }

	void SetParentMatrix(const glm::mat4 &parentMatrix) {
		m_ParentMatrix = parentMatrix;
		MarkWorldMatrixDirty();
	}

	[[nodiscard]] bool IsWorldMatrixDirty() const { return m_WorldMatrixDirty; }

  private:
	void MarkWorldMatrixDirty() { m_WorldMatrixDirty = true; }

	vec3 m_LocalPosition{ 0.f };
	glm::quat m_LocalRotation{ 1.f, 0.f, 0.f, 0.f };
	vec3 m_LocalScale{ 1.f };
	glm::mat4 m_WorldMatrix{ 1.f };
	glm::mat4 m_ParentMatrix{ 1.f };
	bool m_WorldMatrixDirty{ true };
};
} // namespace Aquila::SceneManagement::Components
#endif

#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Aquila::SceneManagement::Components {
using namespace glm;

struct CameraComponent {
	f32 fov = 80.0f;
	f32 nearPlane = 0.1f;
	f32 farPlane = 100.f;
	f32 aspectRatio = 1.778f;
	bool isOrthographic = false;

	f32 orthoLeft = -1.0f;
	f32 orthoRight = 1.0f;
	f32 orthoTop = 1.0f;
	f32 orthoBottom = -1.0f;

	bool primary = false;

	mat4 GetViewMatrix(const vec3 &position, const quat &rotation) const {
		vec3 forward = rotation * vec3(0.0f, 0.0f, 1.0f);
		vec3 up = rotation * vec3(0.0f, -1.0f, 0.0f);

		return GetViewMatrixFromDirection(position, forward, up);
	}

	mat4 GetViewMatrix(const vec3 &position, const vec3 &target, const vec3 &up = vec3(0, -1, 0)) const {
		const vec3 direction = glm::normalize(target - position);
		return GetViewMatrixFromDirection(position, direction, up);
	}

	f32 GetNearPlane() const { return nearPlane; }

	f32 GetFarPlane() const { return farPlane; }

	mat4 GetViewMatrixFromDirection(const vec3 &position, const vec3 &direction,
									const vec3 &up = vec3(0, -1, 0)) const {
		const vec3 w{ glm::normalize(direction) };
		const vec3 u{ glm::normalize(glm::cross(w, up)) };
		const vec3 v{ glm::cross(w, u) };

		mat4 viewMatrix = mat4{ 1.0f };
		viewMatrix[0][0] = u.x;
		viewMatrix[1][0] = u.y;
		viewMatrix[2][0] = u.z;
		viewMatrix[0][1] = v.x;
		viewMatrix[1][1] = v.y;
		viewMatrix[2][1] = v.z;
		viewMatrix[0][2] = w.x;
		viewMatrix[1][2] = w.y;
		viewMatrix[2][2] = w.z;
		viewMatrix[3][0] = -glm::dot(u, position);
		viewMatrix[3][1] = -glm::dot(v, position);
		viewMatrix[3][2] = -glm::dot(w, position);

		return viewMatrix;
	}

	mat4 GetInverseViewMatrix(const vec3 &position, const quat &rotation) const {
		vec3 forward = rotation * vec3(0.0f, 0.0f, 1.0f);
		vec3 up = rotation * vec3(0.0f, -1.0f, 0.0f);
		vec3 right = glm::cross(forward, up);

		mat4 inverseViewMatrix = mat4{ 1.0f };
		inverseViewMatrix[0][0] = right.x;
		inverseViewMatrix[0][1] = right.y;
		inverseViewMatrix[0][2] = right.z;
		inverseViewMatrix[1][0] = up.x;
		inverseViewMatrix[1][1] = up.y;
		inverseViewMatrix[1][2] = up.z;
		inverseViewMatrix[2][0] = forward.x;
		inverseViewMatrix[2][1] = forward.y;
		inverseViewMatrix[2][2] = forward.z;
		inverseViewMatrix[3][0] = position.x;
		inverseViewMatrix[3][1] = position.y;
		inverseViewMatrix[3][2] = position.z;

		return inverseViewMatrix;
	}

	mat4 GetViewMatrixFromQuaternion(const vec3 &position, const quat &rotation) const {
		mat4 rotationMatrix = mat4_cast(conjugate(rotation));
		mat4 translationMatrix = mat4(1.0f);
		translationMatrix[3] = vec4(-position, 1.0f);
		return rotationMatrix * translationMatrix;
	}

	mat4 GetProjectionMatrix() const {
		if (isOrthographic) {
			mat4 projMatrix = mat4{ 1.0f };
			projMatrix[0][0] = 2.0f / (orthoRight - orthoLeft);
			projMatrix[1][1] = 2.0f / (orthoTop - orthoBottom);
			projMatrix[2][2] = 1.0f / (farPlane - nearPlane);
			projMatrix[3][0] = -(orthoRight + orthoLeft) / (orthoRight - orthoLeft);
			projMatrix[3][1] = -(orthoTop + orthoBottom) / (orthoTop - orthoBottom);
			projMatrix[3][2] = -nearPlane / (farPlane - nearPlane);
			return projMatrix;
		} else {
			const f32 tanHalfFovy = tan(glm::radians(fov) / 2.0f);
			mat4 projMatrix = mat4{ 0.0f };
			projMatrix[0][0] = 1.0f / (aspectRatio * tanHalfFovy);
			projMatrix[1][1] = 1.0f / (tanHalfFovy);
			projMatrix[2][2] = farPlane / (farPlane - nearPlane);
			projMatrix[2][3] = 1.0f;
			projMatrix[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);

			projMatrix[1][1] *= -1.0f;

			return projMatrix;
		}
	}

	mat4 GetViewProjectionMatrix(const vec3 &position, const quat &rotation) const {
		return GetProjectionMatrix() * GetViewMatrix(position, rotation);
	}

	mat4 GetViewProjectionMatrix(const vec3 &position, const vec3 &target, const vec3 &up = vec3(0, -1, 0)) const {
		return GetProjectionMatrix() * GetViewMatrix(position, target, up);
	}

	void GetFrustumCorners(const vec3 &position, const quat &rotation, vec3 corners[8]) const {
		mat4 invVP = inverse(GetViewProjectionMatrix(position, rotation));

		vec4 frustumCorners[8] = { { -1, -1, -1, 1 }, { 1, -1, -1, 1 }, { 1, 1, -1, 1 }, { -1, 1, -1, 1 },
								   { -1, -1, 1, 1 },  { 1, -1, 1, 1 },	{ 1, 1, 1, 1 },	 { -1, 1, 1, 1 } };

		for (int i = 0; i < 8; ++i) {
			vec4 worldPos = invVP * frustumCorners[i];
			corners[i] = vec3(worldPos) / worldPos.w;
		}
	}

	vec3 GetForwardDirection(const quat &rotation) const { return rotation * vec3(0.0f, 0.0f, 1.0f); }

	vec3 GetRightDirection(const quat &rotation) const { return rotation * vec3(1.0f, 0.0f, 0.0f); }

	vec3 GetUpDirection(const quat &rotation) const { return rotation * vec3(0.0f, -1.0f, 0.0f); }

	void OnResize(f32 width, f32 height) { aspectRatio = width / height; }

	void SetFOV(f32 newFov) { fov = glm::clamp(newFov, 1.0f, 179.0f); }

	void Zoom(f32 offset) { SetFOV(fov - offset); }

	vec3 QuaternionToEuler(const quat &q) const {
		vec3 euler = eulerAngles(q);
		return vec3(degrees(euler.x), degrees(euler.y), degrees(euler.z));
	}

	quat EulerToQuaternion(const vec3 &euler) const {
		return quat(vec3(radians(euler.x), radians(euler.y), radians(euler.z)));
	}
};

} // namespace Aquila::SceneManagement::Components

#endif

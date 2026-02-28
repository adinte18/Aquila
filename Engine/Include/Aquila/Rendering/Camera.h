#ifndef AQUILA_CAMERA_H
#define AQUILA_CAMERA_H

#include "Aquila/Platform/PrimitiveTypes.h"

namespace Aquila::Rendering {
class Camera {
  public:
	enum class CameraType { Free, Orbit };

	void SetOrthographicProjection(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far);

	void SetPerspectiveProjection(f32 FOV_y, f32 aspect, f32 near, f32 far);

	[[nodiscard]] const mat4 &GetProjection() const { return m_ProjectionMatrix; }
	[[nodiscard]] const mat4 &GetView() const { return m_ViewMatrix; }
	[[nodiscard]] const mat4 &GetInverseView() const { return m_InverseViewMatrix; }

	void SetViewDirection(vec3 position, vec3 direction, vec3 up = vec3(0.f, -1.f, -0.f));
	void SetViewTarget(vec3 position, vec3 target, vec3 up = vec3(0.f, -1.f, -0.f));

	void SetViewYXZ(vec3 position, vec3 rotation);

	void SpeedUp();

	void ResetSpeed();

	void MoveForward(f32 delta);
	void MoveBackward(f32 delta);
	void MoveRight(f32 delta);
	void MoveLeft(f32 delta);
	void Rotate(double yaw, double pitch);
	void Zoom(f32 offset, f32 aspectRatio);

	[[nodiscard]] vec3 &GetPosition() { return m_Position; }
	[[nodiscard]] vec3 &GetRotation() { return m_Rotation; }
	[[nodiscard]] vec3 &GetDirection() { return m_Direction; }
	[[nodiscard]] const f32 &GetAspectRatio() const { return m_AspectRatio; }
	[[nodiscard]] f32 &GetRotationSpeed() { return m_RotationSpeed; }
	[[nodiscard]] f32 &GetMovementSpeed() { return m_MovementSpeed; }
	[[nodiscard]] f32 &GetFOV() { return m_Fov; }
	[[nodiscard]] f32 &GetNearPlane() { return m_Near; }
	[[nodiscard]] f32 &GetFarPlane() { return m_Far; }
	[[nodiscard]] CameraType GetType() const { return m_CameraType; }
	[[nodiscard]] bool &OrbitAroundEntity() { return m_OrbitAroundEntity; }
	[[nodiscard]] vec3 GetTarget() const { return m_OrbitTarget; }
	[[nodiscard]] vec3 GetRightVector() const { return glm::normalize(vec3(m_ViewMatrix[0])); }
	[[nodiscard]] vec3 GetUpVector() const { return glm::normalize(vec3(m_ViewMatrix[1])); }
	[[nodiscard]] vec3 GetForwardVector() const { return glm::normalize(vec3(m_ViewMatrix[2])); }
	void SetPosition(const vec3 pos) { m_Position = pos; }
	void SetRotationSpeed(const f32 speed) { m_RotationSpeed = speed; }

	void SetOrbitTarget(const vec3 &target);
	void OrbitRotate(f32 deltaYaw, f32 deltaPitch);
	void OrbitZoom(f32 deltaRadius);
	void UpdateOrbitPosition();

	void SwitchToType(CameraType newType, vec3 targetPos = vec3{ 0.f });

	void OnResize(f32 width, f32 height);
	void UpdateFreeModeLookDirection();

	void RecalculateView();

  private:
	CameraType m_CameraType{ CameraType::Free };
	mat4 m_ProjectionMatrix{ 1.f };
	mat4 m_ViewMatrix{ 1.f };
	mat4 m_InverseViewMatrix{ 1.f };

	vec3 m_Position{ 0.0f };
	vec3 m_Rotation{ 0.0f };

	f32 m_MovementSpeed{ 5.0f };
	f32 m_RotationSpeed{ 0.001f };

	vec3 m_Direction{ 0.0f, 0.0f, -1.0f };

	f32 m_Fov{ 80.0f };
	f32 m_Near{ 0.1f };
	f32 m_Far{ 100.f };
	f32 m_AspectRatio{ 0.f };

	bool m_IsSpedUp{ false };

	vec3 m_OrbitTarget{ 0.0f, 0.0f, 0.0f };
	f32 m_OrbitRadius{ 10.0f };
	f32 m_OrbitYaw{ 0.0f };
	f32 m_OrbitPitch{ 0.0f };

	bool m_OrbitAroundEntity{ false };
};
} // namespace Aquila::Rendering

#endif // CAMERA_H

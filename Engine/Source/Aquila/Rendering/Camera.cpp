#include "Aquila/Rendering/Camera.h"
#include "Aquila/Core/AquilaCore.h"
#include "Aquila/Core/Defines.h"

namespace Aquila::Rendering {

void Camera::SpeedUp() {
	if (!m_IsSpedUp) {
		m_MovementSpeed *= 5.0f;
		m_IsSpedUp = true;
	}
}

void Camera::ResetSpeed() {
	if (m_IsSpedUp) {
		m_MovementSpeed /= 5.0f;
		m_IsSpedUp = false;
	}
}

void Camera::MoveForward(f32 delta) {
	mat4 viewMatrix = GetInverseView();
	vec3 forward = Math::Normalize(vec3(viewMatrix[2]));
	vec3 moveDir = forward;

	if (Math::Dot(moveDir, moveDir) > Math::EPSILON) {
		m_Position += m_MovementSpeed * delta * Math::Normalize(moveDir);

		if (m_CameraType == CameraType::Free) {
			m_OrbitTarget = m_Position + forward * m_OrbitRadius;
		}
	}
}

void Camera::MoveBackward(f32 delta) {
	mat4 viewMatrix = GetInverseView();
	vec3 forward = Math::Normalize(vec3(viewMatrix[2]));
	vec3 moveDir = -forward;

	if (Math::Dot(moveDir, moveDir) > Math::EPSILON) {
		m_Position += m_MovementSpeed * delta * Math::Normalize(moveDir);

		if (m_CameraType == CameraType::Free) {
			m_OrbitTarget = m_Position + forward * m_OrbitRadius;
		}
	}
}

void Camera::MoveRight(f32 delta) {
	mat4 viewMatrix = GetInverseView();
	vec3 right = Math::Normalize(vec3(viewMatrix[0]));
	m_Position += right * m_MovementSpeed * delta;

	if (m_CameraType == CameraType::Free) {
		vec3 forward = Math::Normalize(vec3(viewMatrix[2]));
		m_OrbitTarget = m_Position + forward * m_OrbitRadius;
	}
}

void Camera::MoveLeft(f32 delta) {
	mat4 viewMatrix = GetInverseView();
	vec3 right = Math::Normalize(vec3(viewMatrix[0]));
	m_Position -= right * m_MovementSpeed * delta;

	if (m_CameraType == CameraType::Free) {
		vec3 forward = Math::Normalize(vec3(viewMatrix[2]));
		m_OrbitTarget = m_Position + forward * m_OrbitRadius;
	}
}

void Camera::Rotate(const double yaw, const double pitch) {
	m_Rotation.x += pitch;
	m_Rotation.y += yaw;
	m_Rotation.x = Math::Clamp(m_Rotation.x, -89.0f, 89.0f);

	SetViewYXZ(m_Position, m_Rotation);

	if (m_CameraType == CameraType::Free) {
		UpdateFreeModeLookDirection();
	}
}

void Camera::UpdateFreeModeLookDirection() {
	const f32 c1 = std::cos(m_Rotation.y);
	const f32 s1 = std::sin(m_Rotation.y);
	const f32 c2 = std::cos(m_Rotation.x);
	const f32 s2 = std::sin(m_Rotation.x);

	vec3 forward = Math::Normalize(vec3(c2 * s1, -s2, c1 * c2));
	m_OrbitTarget = m_Position + forward * m_OrbitRadius;
}

void Camera::Zoom(const f32 offset, const f32 aspectRatio) {
	m_Fov -= offset;
	m_Fov = Math::Clamp(m_Fov, 1.0f, 90.0f);
	SetPerspectiveProjection(Math::Radians(m_Fov), aspectRatio, m_Near, m_Far);
}

void Camera::OnResize(const f32 width, const f32 height) {
	SetPerspectiveProjection(m_Fov, width / height, m_Near, m_Far);
}

void Camera::SetOrthographicProjection(f32 left, f32 right, f32 top, f32 bottom, f32 nearPlane, f32 farPlane) {
	m_ProjectionMatrix = Math::OrthoVulkan(left, right, bottom, top, nearPlane, farPlane);
}

void Camera::SetPerspectiveProjection(f32 FOV_y, f32 aspect, f32 nearPlane, f32 farPlane) {
	AQUILA_ASSERT(std::abs(aspect - Math::EPSILON) > 0.0f, "Aspect ratio must not be zero");

	this->m_AspectRatio = aspect;
	this->m_Fov = FOV_y;
	this->m_Near = nearPlane;
	this->m_Far = farPlane;
	m_ProjectionMatrix = Math::PerspectiveVulkan(Math::Radians(m_Fov), aspect, nearPlane, farPlane);
}

void Camera::SetViewDirection(vec3 position, vec3 direction, vec3 up) {
	m_ViewMatrix = Math::LookInDirection(position, direction, up);

	const vec3 w = Math::Normalize(direction);
	const vec3 u = Math::Normalize(Math::Cross(w, up));
	const vec3 v = Math::Cross(w, u);

	m_InverseViewMatrix = mat4{ 1.f };
	m_InverseViewMatrix[0][0] = u.x;
	m_InverseViewMatrix[0][1] = u.y;
	m_InverseViewMatrix[0][2] = u.z;
	m_InverseViewMatrix[1][0] = v.x;
	m_InverseViewMatrix[1][1] = v.y;
	m_InverseViewMatrix[1][2] = v.z;
	m_InverseViewMatrix[2][0] = w.x;
	m_InverseViewMatrix[2][1] = w.y;
	m_InverseViewMatrix[2][2] = w.z;
	m_InverseViewMatrix[3][0] = position.x;
	m_InverseViewMatrix[3][1] = position.y;
	m_InverseViewMatrix[3][2] = position.z;
}

void Camera::SetViewTarget(vec3 position, vec3 target, vec3 up) {
	m_ViewMatrix = Math::LookAt(position, target, up);

	const vec3 w = Math::Normalize(target - position);
	const vec3 u = Math::Normalize(Math::Cross(w, up));
	const vec3 v = Math::Cross(w, u);

	m_InverseViewMatrix = mat4{ 1.f };
	m_InverseViewMatrix[0][0] = u.x;
	m_InverseViewMatrix[0][1] = u.y;
	m_InverseViewMatrix[0][2] = u.z;
	m_InverseViewMatrix[1][0] = v.x;
	m_InverseViewMatrix[1][1] = v.y;
	m_InverseViewMatrix[1][2] = v.z;
	m_InverseViewMatrix[2][0] = w.x;
	m_InverseViewMatrix[2][1] = w.y;
	m_InverseViewMatrix[2][2] = w.z;
	m_InverseViewMatrix[3][0] = position.x;
	m_InverseViewMatrix[3][1] = position.y;
	m_InverseViewMatrix[3][2] = position.z;
}

void Camera::SetViewYXZ(vec3 position, vec3 rotation) {
	m_ViewMatrix = Math::ViewFromEuler(position, rotation);

	const f32 c3 = std::cos(rotation.z);
	const f32 s3 = std::sin(rotation.z);
	const f32 c2 = std::cos(rotation.x);
	const f32 s2 = std::sin(rotation.x);
	const f32 c1 = std::cos(rotation.y);
	const f32 s1 = std::sin(rotation.y);
	const vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
	const vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
	const vec3 w{ (c2 * s1), (-s2), (c1 * c2) };

	m_InverseViewMatrix = mat4{ 1.f };
	m_InverseViewMatrix[0][0] = u.x;
	m_InverseViewMatrix[0][1] = u.y;
	m_InverseViewMatrix[0][2] = u.z;
	m_InverseViewMatrix[1][0] = v.x;
	m_InverseViewMatrix[1][1] = v.y;
	m_InverseViewMatrix[1][2] = v.z;
	m_InverseViewMatrix[2][0] = w.x;
	m_InverseViewMatrix[2][1] = w.y;
	m_InverseViewMatrix[2][2] = w.z;
	m_InverseViewMatrix[3][0] = position.x;
	m_InverseViewMatrix[3][1] = position.y;
	m_InverseViewMatrix[3][2] = position.z;
}

void Camera::SetOrbitTarget(const vec3 &target) {
	m_OrbitTarget = target;
	vec3 offset = m_Position - m_OrbitTarget;

	m_OrbitRadius = Math::Length(offset);
	if (m_OrbitRadius < 0.1f)
		m_OrbitRadius = 5.0f;

	m_OrbitPitch = std::asin(offset.y / m_OrbitRadius);
	m_OrbitYaw = std::atan2(offset.x, offset.z);

	UpdateOrbitPosition();
}

void Camera::OrbitRotate(const f32 deltaYaw, const f32 deltaPitch) {
	m_OrbitYaw += deltaYaw;
	m_OrbitPitch += deltaPitch;
	m_OrbitPitch = std::clamp(m_OrbitPitch, -Math::HALF_PI + 0.01f, Math::HALF_PI - 0.01f);
	UpdateOrbitPosition();
}

void Camera::OrbitZoom(const f32 deltaRadius) {
	m_OrbitRadius = std::max(m_OrbitRadius + deltaRadius, 0.1f);
	UpdateOrbitPosition();
}

void Camera::UpdateOrbitPosition() {
	m_Position.x = m_OrbitTarget.x + m_OrbitRadius * std::cos(m_OrbitPitch) * std::sin(m_OrbitYaw);
	m_Position.y = m_OrbitTarget.y + m_OrbitRadius * std::sin(m_OrbitPitch);
	m_Position.z = m_OrbitTarget.z + m_OrbitRadius * std::cos(m_OrbitPitch) * std::cos(m_OrbitYaw);

	RecalculateView();
}

void Camera::RecalculateView() {
	SetViewTarget(m_Position, m_OrbitTarget, vec3(0, -1, 0));
	m_Direction = Math::Normalize(m_OrbitTarget - m_Position);

	vec3 forward = m_Direction;
	m_Rotation.x = std::asin(-forward.y);
	m_Rotation.y = std::atan2(forward.x, forward.z);
	m_Rotation.z = 0.0f;
}

void Camera::SwitchToType(const CameraType newType, const vec3 targetPos) {
	if (m_CameraType != newType) {
		CameraType oldType = m_CameraType;
		m_CameraType = newType;

		AQUILA_LOG_DEBUG("Camera Type: {}", (int)GetType());

		if (m_CameraType == CameraType::Orbit) {
			if (oldType == CameraType::Free) {
				mat4 viewMatrix = GetInverseView();
				vec3 forward = Math::Normalize(vec3(viewMatrix[2]));
				vec3 newTarget = m_Position + forward * m_OrbitRadius;
				SetOrbitTarget(newTarget);
			} else {
				SetOrbitTarget(targetPos);
			}
			UpdateOrbitPosition();
		} else if (m_CameraType == CameraType::Free) {
			UpdateFreeModeLookDirection();
		}
	}
}

} // namespace Aquila::Rendering

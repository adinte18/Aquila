//
// Created by alexa on 24/10/2024.
//

#include "Engine/EditorCamera.h"
#include "Defines.h"

namespace Engine {

void EditorCamera::SpeedUp() {
  if (!m_IsSpedUp) {
    m_MovementSpeed *= 5.0f;
    m_IsSpedUp = true;
  }
}

void EditorCamera::ResetSpeed() {
  if (m_IsSpedUp) {
    m_MovementSpeed /= 5.0f;
    m_IsSpedUp = false;
  }
}

void EditorCamera::MoveForward(f32 delta) {
  glm::mat4 viewMatrix = GetInverseView();

  vec3 forward = glm::normalize(vec3(viewMatrix[2]));

  vec3 moveDir = forward;

  if (glm::dot(moveDir, moveDir) > std::numeric_limits<f32>::epsilon()) {
    m_Position += m_MovementSpeed * delta * glm::normalize(moveDir);
  }
}

void EditorCamera::MoveBackward(f32 delta) {
  glm::mat4 viewMatrix = GetInverseView();

  vec3 forward = glm::normalize(vec3(viewMatrix[2]));

  vec3 moveDir = -forward;

  if (glm::dot(moveDir, moveDir) > std::numeric_limits<f32>::epsilon()) {
    m_Position += m_MovementSpeed * delta * glm::normalize(moveDir);
  }
}

void EditorCamera::MoveRight(f32 delta) {
  glm::mat4 viewMatrix = GetInverseView();

  vec3 right = glm::normalize(vec3(viewMatrix[0]));

  m_Position += right * m_MovementSpeed * delta;
}

void EditorCamera::MoveLeft(f32 delta) {
  glm::mat4 viewMatrix = GetInverseView();

  vec3 right = glm::normalize(vec3(viewMatrix[0]));

  m_Position -= right * m_MovementSpeed * delta;
}

void EditorCamera::Rotate(const double yaw, const double pitch) {

  m_Rotation.x += pitch;
  m_Rotation.y += yaw;

  m_Rotation.x = glm::clamp(m_Rotation.x, -89.0f, 89.0f);

  SetViewYXZ(m_Position, m_Rotation);
}

void EditorCamera::Zoom(const f32 offset, const f32 aspectRatio) {
  m_Fov -= offset;
  m_Fov = glm::clamp(m_Fov, 1.0f, 90.0f);

  SetPerspectiveProjection(glm::radians(m_Fov), aspectRatio, m_Near, m_Far);
}

void EditorCamera::OnResize(const f32 width, const f32 height) {
  SetPerspectiveProjection(m_Fov, width / height, m_Near, m_Far);
}

void EditorCamera::SetOrthographicProjection(f32 left, f32 right, f32 top,
                                             f32 bottom, f32 nearPlane,
                                             f32 farPlane) {
  m_ProjectionMatrix = glm::mat4{1.0f};
  m_ProjectionMatrix[0][0] = 2.f / (right - left);
  m_ProjectionMatrix[1][1] = 2.f / (top - bottom);
  m_ProjectionMatrix[2][2] = 1.f / (farPlane - nearPlane);
  m_ProjectionMatrix[3][0] = -(right + left) / (right - left);
  m_ProjectionMatrix[3][1] = -(top + bottom) / (top - bottom);
  m_ProjectionMatrix[3][2] = -nearPlane / (farPlane - nearPlane);
}

void EditorCamera::SetPerspectiveProjection(f32 FOV_y, f32 aspect,
                                            f32 nearPlane, f32 farPlane) {
  AQUILA_ASSERT(glm::abs(aspect - std::numeric_limits<f32>::epsilon()) > 0.0f,
                "Aspect ratio must not be zero");

  // save them
  this->m_AspectRatio = aspect;
  this->m_Fov = FOV_y;
  this->m_Near = nearPlane;
  this->m_Far = farPlane;

  const f32 tanHalfFovy = tan(glm::radians(m_Fov) / 2.f);
  m_ProjectionMatrix = glm::mat4{0.0f};
  m_ProjectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
  m_ProjectionMatrix[1][1] = 1.f / (tanHalfFovy);
  m_ProjectionMatrix[2][2] = farPlane / (farPlane - nearPlane);
  m_ProjectionMatrix[2][3] = 1.f;
  m_ProjectionMatrix[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);
}

void EditorCamera::SetViewDirection(vec3 position, vec3 direction, vec3 up) {
  const vec3 w{glm::normalize(direction)};
  const vec3 u{glm::normalize(glm::cross(w, up))};
  const vec3 v{glm::cross(w, u)};

  m_ViewMatrix = glm::mat4{1.f};
  m_ViewMatrix[0][0] = u.x;
  m_ViewMatrix[1][0] = u.y;
  m_ViewMatrix[2][0] = u.z;
  m_ViewMatrix[0][1] = v.x;
  m_ViewMatrix[1][1] = v.y;
  m_ViewMatrix[2][1] = v.z;
  m_ViewMatrix[0][2] = w.x;
  m_ViewMatrix[1][2] = w.y;
  m_ViewMatrix[2][2] = w.z;
  m_ViewMatrix[3][0] = -glm::dot(u, position);
  m_ViewMatrix[3][1] = -glm::dot(v, position);
  m_ViewMatrix[3][2] = -glm::dot(w, position);

  m_InverseViewMatrix = glm::mat4{1.f};
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

void EditorCamera::SetViewTarget(vec3 position, vec3 target, vec3 up) {
  SetViewDirection(position, target - position, up);
}

void EditorCamera::SetViewYXZ(vec3 position, vec3 rotation) {
  const f32 c3 = glm::cos(rotation.z);
  const f32 s3 = glm::sin(rotation.z);
  const f32 c2 = glm::cos(rotation.x);
  const f32 s2 = glm::sin(rotation.x);
  const f32 c1 = glm::cos(rotation.y);
  const f32 s1 = glm::sin(rotation.y);
  const vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
  const vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
  const vec3 w{(c2 * s1), (-s2), (c1 * c2)};
  m_ViewMatrix = glm::mat4{1.f};
  m_ViewMatrix[0][0] = u.x;
  m_ViewMatrix[1][0] = u.y;
  m_ViewMatrix[2][0] = u.z;
  m_ViewMatrix[0][1] = v.x;
  m_ViewMatrix[1][1] = v.y;
  m_ViewMatrix[2][1] = v.z;
  m_ViewMatrix[0][2] = w.x;
  m_ViewMatrix[1][2] = w.y;
  m_ViewMatrix[2][2] = w.z;
  m_ViewMatrix[3][0] = -glm::dot(u, position);
  m_ViewMatrix[3][1] = -glm::dot(v, position);
  m_ViewMatrix[3][2] = -glm::dot(w, position);

  m_InverseViewMatrix = glm::mat4{1.f};
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

void EditorCamera::SetOrbitTarget(const vec3 &target) {
  m_OrbitTarget = target;
  vec3 offset = m_Position - m_OrbitTarget;

  m_OrbitRadius = glm::length(offset);
  m_OrbitPitch = glm::asin(offset.y / m_OrbitRadius);
  m_OrbitYaw = glm::atan(offset.x, offset.z);

  UpdateOrbitPosition();
}

void EditorCamera::OrbitRotate(const f32 deltaYaw, const f32 deltaPitch) {
  m_OrbitYaw += deltaYaw;
  m_OrbitPitch += deltaPitch;
  m_OrbitPitch = std::clamp(m_OrbitPitch, -glm::half_pi<f32>() + 0.01f,
                            glm::half_pi<f32>() - 0.01f);
  UpdateOrbitPosition();
}

void EditorCamera::OrbitZoom(const f32 deltaRadius) {
  m_OrbitRadius = std::max(m_OrbitRadius + deltaRadius, 0.1f);
  UpdateOrbitPosition();
}

void EditorCamera::UpdateOrbitPosition() {
  m_Position.x =
      m_OrbitTarget.x + m_OrbitRadius * cos(m_OrbitPitch) * sin(m_OrbitYaw);
  m_Position.y = m_OrbitTarget.y + m_OrbitRadius * sin(m_OrbitPitch);
  m_Position.z =
      m_OrbitTarget.z + m_OrbitRadius * cos(m_OrbitPitch) * cos(m_OrbitYaw);

  RecalculateView();
}

void EditorCamera::RecalculateView() {
  SetViewTarget(m_Position, m_OrbitTarget, vec3(0, -1, 0));
  m_Direction = glm::normalize(m_OrbitTarget - m_Position);
}

void EditorCamera::SwitchToType(const CameraType newType,
                                const vec3 targetPos) {
  if (m_CameraType != newType) {
    m_CameraType = newType;
    std::cout << "Camera Type: " << (int)GetType() << std::endl;

    if (m_CameraType == CameraType::Orbit) {
      SetOrbitTarget(targetPos);

      UpdateOrbitPosition();
    }
  }
}
} // namespace Engine

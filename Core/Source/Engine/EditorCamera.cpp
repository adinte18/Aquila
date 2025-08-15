//
// Created by alexa on 24/10/2024.
//

#include "Engine/EditorCamera.h"

#include <iostream>
#include <glm/ext/matrix_transform.hpp>

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

void EditorCamera::MoveForward(float delta) {
    glm::mat4 viewMatrix = GetInverseView();  // Get the camera's inverse view matrix

    glm::vec3 forward = glm::normalize(glm::vec3(viewMatrix[2]));

    glm::vec3 moveDir = forward;

    // Only move if moveDir is not zero (which should always be the case)
    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        m_Position += m_MovementSpeed * delta * glm::normalize(moveDir);
    }
}


void EditorCamera::MoveBackward(float delta) {
    glm::mat4 viewMatrix = GetInverseView();  // Get the camera's inverse view matrix

    glm::vec3 forward = glm::normalize(glm::vec3(viewMatrix[2]));

    glm::vec3 moveDir = -forward;

    // Only move if moveDir is not zero (which should always be the case)
    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        m_Position += m_MovementSpeed * delta * glm::normalize(moveDir);
    }
}


void EditorCamera::MoveRight(float delta) {
    glm::mat4 viewMatrix = GetInverseView();  // Get the camera's inverse view matrix

    // Get the right vector (first column of the inverse view matrix)
    glm::vec3 right = glm::normalize(glm::vec3(viewMatrix[0]));

    // Move in the right direction
    m_Position += right * m_MovementSpeed * delta;
}

void EditorCamera::MoveLeft(float delta) {
    glm::mat4 viewMatrix = GetInverseView();  // Get the camera's inverse view matrix

    // Get the right vector (first column of the inverse view matrix)
    glm::vec3 right = glm::normalize(glm::vec3(viewMatrix[0]));

    // Move in the opposite direction of the right vector (i.e., left)
    m_Position -= right * m_MovementSpeed * delta;
}


void EditorCamera::Rotate(const double yaw, const double pitch) {

    m_Rotation.x += pitch;
    m_Rotation.y += yaw;

    m_Rotation.x = glm::clamp(m_Rotation.x, -89.0f, 89.0f);

    SetViewYXZ(m_Position, m_Rotation);
}

void EditorCamera::Zoom(const float offset, const float aspectRatio) {
    m_Fov -= offset;
    m_Fov = glm::clamp(m_Fov, 1.0f, 90.0f);

    SetPerspectiveProjection(glm::radians(m_Fov), aspectRatio, m_Near, m_Far);
}

void EditorCamera::OnResize(const float width, const float height) {
    SetPerspectiveProjection(m_Fov, width / height, m_Near, m_Far);
}


void EditorCamera::SetOrthographicProjection(float left, float right, float top, float bottom, float nearPlane, float farPlane) {
    m_ProjectionMatrix = glm::mat4{1.0f};
    m_ProjectionMatrix[0][0] = 2.f / (right - left);
    m_ProjectionMatrix[1][1] = 2.f / (top - bottom);
    m_ProjectionMatrix[2][2] = 1.f / (farPlane - nearPlane);
    m_ProjectionMatrix[3][0] = -(right + left) / (right - left);
    m_ProjectionMatrix[3][1] = -(top + bottom) / (top - bottom);
    m_ProjectionMatrix[3][2] = -nearPlane / (farPlane - nearPlane);
}

void EditorCamera::SetPerspectiveProjection(float FOV_y, float aspect, float nearPlane, float farPlane) {
    AQUILA_CORE_ASSERT(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);

    //save them
    this->m_AspectRatio = aspect;
    this->m_Fov = FOV_y;
    this->m_Near = nearPlane;
    this->m_Far = farPlane;

    const float tanHalfFovy = tan(glm::radians(m_Fov) / 2.f);
    m_ProjectionMatrix = glm::mat4{0.0f};
    m_ProjectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
    m_ProjectionMatrix[1][1] = 1.f / (tanHalfFovy);
    m_ProjectionMatrix[2][2] = farPlane / (farPlane - nearPlane);
    m_ProjectionMatrix[2][3] = 1.f;
    m_ProjectionMatrix[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);
}

void EditorCamera::SetViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
    const glm::vec3 w{glm::normalize(direction)};
    const glm::vec3 u{glm::normalize(glm::cross(w, up))};
    const glm::vec3 v{glm::cross(w, u)};

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

void EditorCamera::SetViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
    SetViewDirection(position, target - position, up);
}

void EditorCamera::SetViewYXZ(glm::vec3 position, glm::vec3 rotation) {
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
    const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
    const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
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

void EditorCamera::SetOrbitTarget(const glm::vec3& target) {
    m_OrbitTarget = target;
    glm::vec3 offset = m_Position - m_OrbitTarget;

    m_OrbitRadius = glm::length(offset);
    m_OrbitPitch = glm::asin(offset.y / m_OrbitRadius);
    m_OrbitYaw = glm::atan(offset.x, offset.z);

    UpdateOrbitPosition();
}

void EditorCamera::OrbitRotate(const float deltaYaw, const float deltaPitch) {
    m_OrbitYaw += deltaYaw;
    m_OrbitPitch += deltaPitch;
    m_OrbitPitch = std::clamp(m_OrbitPitch, -glm::half_pi<float>() + 0.01f, glm::half_pi<float>() - 0.01f);
    UpdateOrbitPosition();
}

void EditorCamera::OrbitZoom(const float deltaRadius) {
    m_OrbitRadius = std::max(m_OrbitRadius + deltaRadius, 0.1f);
    UpdateOrbitPosition();
}

void EditorCamera::UpdateOrbitPosition() {
    m_Position.x = m_OrbitTarget.x + m_OrbitRadius * cos(m_OrbitPitch) * sin(m_OrbitYaw);
    m_Position.y = m_OrbitTarget.y + m_OrbitRadius * sin(m_OrbitPitch);
    m_Position.z = m_OrbitTarget.z + m_OrbitRadius * cos(m_OrbitPitch) * cos(m_OrbitYaw);

    RecalculateView();
}

void EditorCamera::RecalculateView() {
    SetViewTarget(m_Position, m_OrbitTarget, glm::vec3(0,-1,0));
    m_Direction = glm::normalize(m_OrbitTarget - m_Position);
}

void EditorCamera::SwitchToType(const CameraType newType, const glm::vec3 targetPos) {
    if (m_CameraType != newType) {
        m_CameraType = newType;
        std::cout << "Camera Type: " << (int)GetType() << std::endl;

        if (m_CameraType == CameraType::Orbit) {
            SetOrbitTarget(targetPos);

            UpdateOrbitPosition();
        }
    }
}
}

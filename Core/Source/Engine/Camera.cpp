//
// Created by alexa on 24/10/2024.
//

#include "Engine/Camera.h"

#include <iostream>

namespace Engine {

void Camera::SpeedUp() {
    if (!isSpedUp) {
        movementSpeed *= 5.0f;
        isSpedUp = true;
    }
}

void Camera::ResetSpeed() {
    if (isSpedUp) {
        movementSpeed /= 5.0f;
        isSpedUp = false;
    }
}

void Camera::MoveForward(float delta) {
    glm::mat4 viewMatrix = GetInverseView();  // Get the camera's inverse view matrix

    glm::vec3 forward = glm::normalize(glm::vec3(viewMatrix[2]));

    glm::vec3 moveDir = forward;

    // Only move if moveDir is not zero (which should always be the case)
    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        position += movementSpeed * delta * glm::normalize(moveDir);
    }
}


void Camera::MoveBackward(float delta) {
    glm::mat4 viewMatrix = GetInverseView();  // Get the camera's inverse view matrix

    glm::vec3 forward = glm::normalize(glm::vec3(viewMatrix[2]));

    glm::vec3 moveDir = -forward;

    // Only move if moveDir is not zero (which should always be the case)
    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        position += movementSpeed * delta * glm::normalize(moveDir);
    }
}


void Camera::MoveRight(float delta) {
    glm::mat4 viewMatrix = GetInverseView();  // Get the camera's inverse view matrix

    // Get the right vector (first column of the inverse view matrix)
    glm::vec3 right = glm::normalize(glm::vec3(viewMatrix[0]));

    // Move in the right direction
    position += right * movementSpeed * delta;
}

void Camera::MoveLeft(float delta) {
    glm::mat4 viewMatrix = GetInverseView();  // Get the camera's inverse view matrix

    // Get the right vector (first column of the inverse view matrix)
    glm::vec3 right = glm::normalize(glm::vec3(viewMatrix[0]));

    // Move in the opposite direction of the right vector (i.e., left)
    position -= right * movementSpeed * delta;
}


void Camera::Rotate(double yaw, double pitch) {

    rotation.x += pitch;
    rotation.y += yaw;

    rotation.x = glm::clamp(rotation.x, -89.0f, 89.0f);

    SetViewYXZ(position, rotation);
}

void Camera::Zoom(float offset, float aspectRatio) {
    // Update zoom level and clamp it
    zoom -= offset;
    zoom = glm::clamp(zoom, 1.0f, 90.0f); // Clamping to avoid extreme zoom levels

    // Update the perspective projection matrix based on new zoom level
    SetPerspectiveProjection(glm::radians(zoom), aspectRatio, 0.1, 100.f);
}

void Camera::OnResize(float width, float height) {
    SetPerspectiveProjection(glm::radians(zoom), width / height, 0.1, 1000.f);
}


void Camera::SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
    projectionMatrix = glm::mat4{1.0f};
    projectionMatrix[0][0] = 2.f / (right - left);
    projectionMatrix[1][1] = 2.f / (top - bottom);
    projectionMatrix[2][2] = 1.f / (far - near);
    projectionMatrix[3][0] = -(right + left) / (right - left);
    projectionMatrix[3][1] = -(top + bottom) / (top - bottom);
    projectionMatrix[3][2] = -near / (far - near);
}

void Camera::SetPerspectiveProjection(float FOV_y, float aspect, float near, float far) {
    assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
    const float tanHalfFovy = tan(FOV_y / 2.f);
    projectionMatrix = glm::mat4{0.0f};
    projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
    projectionMatrix[1][1] = 1.f / (tanHalfFovy);
    projectionMatrix[2][2] = far / (far - near);
    projectionMatrix[2][3] = 1.f;
    projectionMatrix[3][2] = -(far * near) / (far - near);
}

void Camera::SetViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
    const glm::vec3 w{glm::normalize(direction)};
    const glm::vec3 u{glm::normalize(glm::cross(w, up))};
    const glm::vec3 v{glm::cross(w, u)};

    viewMatrix = glm::mat4{1.f};
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

    inverseViewMatrix = glm::mat4{1.f};
    inverseViewMatrix[0][0] = u.x;
    inverseViewMatrix[0][1] = u.y;
    inverseViewMatrix[0][2] = u.z;
    inverseViewMatrix[1][0] = v.x;
    inverseViewMatrix[1][1] = v.y;
    inverseViewMatrix[1][2] = v.z;
    inverseViewMatrix[2][0] = w.x;
    inverseViewMatrix[2][1] = w.y;
    inverseViewMatrix[2][2] = w.z;
    inverseViewMatrix[3][0] = position.x;
    inverseViewMatrix[3][1] = position.y;
    inverseViewMatrix[3][2] = position.z;
}

void Camera::SetViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
    SetViewDirection(position, target - position, up);
}

void Camera::SetViewYXZ(glm::vec3 position, glm::vec3 rotation) {
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
    const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
    const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
    viewMatrix = glm::mat4{1.f};
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

    inverseViewMatrix = glm::mat4{1.f};
    inverseViewMatrix[0][0] = u.x;
    inverseViewMatrix[0][1] = u.y;
    inverseViewMatrix[0][2] = u.z;
    inverseViewMatrix[1][0] = v.x;
    inverseViewMatrix[1][1] = v.y;
    inverseViewMatrix[1][2] = v.z;
    inverseViewMatrix[2][0] = w.x;
    inverseViewMatrix[2][1] = w.y;
    inverseViewMatrix[2][2] = w.z;
    inverseViewMatrix[3][0] = position.x;
    inverseViewMatrix[3][1] = position.y;
    inverseViewMatrix[3][2] = position.z;
}


}

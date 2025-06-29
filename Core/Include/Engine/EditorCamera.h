//
// Created by alexa on 24/10/2024.
//

#ifndef CAMERA_H
#define CAMERA_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <algorithm>

#include "AquilaCore.h"

namespace Engine
{
    class EditorCamera
    {
        public:
            enum class CameraType {
                Free,
                Orbit
            };

            void SetOrthographicProjection(float left, float right, float top, float bottom,
                                          float near, float far);

            void SetPerspectiveProjection(float FOV_y, float aspect, float near, float far);

            [[nodiscard]] const glm::mat4& GetProjection() const {return projectionMatrix;}
            [[nodiscard]] const glm::mat4& GetView() const { return viewMatrix; }
            [[nodiscard]] const glm::mat4& GetInverseView() const { return inverseViewMatrix; }

            void SetViewDirection(glm::vec3 position,
                glm::vec3 direction, glm::vec3 up = glm::vec3(0.f, -1.f, -0.f));
            void SetViewTarget(glm::vec3 position,
                glm::vec3 target, glm::vec3 up = glm::vec3(0.f, -1.f, -0.f));

            void SetViewYXZ(glm::vec3 position, glm::vec3 rotation);

            void SpeedUp();

            void ResetSpeed();

            void MoveForward(float delta);
            void MoveBackward(float delta);
            void MoveRight(float delta);
            void MoveLeft(float delta);
            void Rotate(double yaw, double pitch);
            void Zoom(float offset, float aspectRatio);

            [[nodiscard]] glm::vec3& GetPosition() { return position; }
            [[nodiscard]] glm::vec3& GetRotation() { return rotation; }
            [[nodiscard]] glm::vec3& GetDirection() { return direction; }
            [[nodiscard]] const float& GetAspectRatio() const { return aspectRatio; }
            [[nodiscard]] float& GetRotationSpeed() { return rotationSpeed; }
            [[nodiscard]] float& GetMovementSpeed() { return movementSpeed; }
            [[nodiscard]] float& GetFOV() { return fov; }
            [[nodiscard]] float& GetNearPlane() { return near; }
            [[nodiscard]] float& GetFarPlane() { return far;}
            [[nodiscard]] CameraType GetType() const { return type; }
            [[nodiscard]] bool& OrbitAroundEntity() { return orbitAroundEntity; }
            [[nodiscard]] glm::vec3 GetTarget() const { return orbitTarget; }
            [[nodiscard]] glm::vec3 GetRightVector() const { return glm::normalize(glm::vec3(viewMatrix[0])); }
            [[nodiscard]] glm::vec3 GetUpVector() const { return glm::normalize(glm::vec3(viewMatrix[2])); }
            void SetPosition(const glm::vec3 pos) { position = pos; }
            void SetRotationSpeed(const float speed) {rotationSpeed = speed;}

            void SetOrbitTarget(const glm::vec3& target);
            void OrbitRotate(float deltaYaw, float deltaPitch);
            void OrbitZoom(float deltaRadius);
            void UpdateOrbitPosition();


            void SwitchToType(CameraType newType, glm::vec3 targetPos = glm::vec3{0.f});

            void OnResize(float width, float height);

        private:
            CameraType type{CameraType::Free};
            glm::mat4 projectionMatrix{1.f};
            glm::mat4 viewMatrix{1.f};
            glm::mat4 inverseViewMatrix{1.f};

            glm::vec3 position{0.0f}; 
            glm::vec3 rotation{0.0f}; 

            float movementSpeed{5.0f};
            float rotationSpeed{0.001f}; 

            glm::vec3 direction{0.0f, 0.0f, -1.0f};

            float fov{80.0f};
            float near{0.1f};
            float far{100.f};
            float aspectRatio{0.f};

            bool isSpedUp{false};

            glm::vec3 orbitTarget{0.0f, 0.0f, 0.0f};
            float orbitRadius{10.0f};
            float orbitYaw{0.0f};
            float orbitPitch{0.0f};

            bool orbitAroundEntity{false};

            void RecalculateView();


    };
}



#endif //CAMERA_H

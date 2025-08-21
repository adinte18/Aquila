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

            [[nodiscard]] const glm::mat4& GetProjection() const {return m_ProjectionMatrix;}
            [[nodiscard]] const glm::mat4& GetView() const { return m_ViewMatrix; }
            [[nodiscard]] const glm::mat4& GetInverseView() const { return m_InverseViewMatrix; }

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

            [[nodiscard]] glm::vec3& GetPosition() { return m_Position; }
            [[nodiscard]] glm::vec3& GetRotation() { return m_Rotation; }
            [[nodiscard]] glm::vec3& GetDirection() { return m_Direction; }
            [[nodiscard]] const float& GetAspectRatio() const { return m_AspectRatio; }
            [[nodiscard]] float& GetRotationSpeed() { return m_RotationSpeed; }
            [[nodiscard]] float& GetMovementSpeed() { return m_MovementSpeed; }
            [[nodiscard]] float& GetFOV() { return m_Fov; }
            [[nodiscard]] float& GetNearPlane() { return m_Near; }
            [[nodiscard]] float& GetFarPlane() { return m_Far; }
            [[nodiscard]] CameraType GetType() const { return m_CameraType; }
            [[nodiscard]] bool& OrbitAroundEntity() { return m_OrbitAroundEntity; }
            [[nodiscard]] glm::vec3 GetTarget() const { return m_OrbitTarget; }
            [[nodiscard]] glm::vec3 GetRightVector() const { return glm::normalize(glm::vec3(m_ViewMatrix[0])); }
            [[nodiscard]] glm::vec3 GetUpVector() const { return glm::normalize(glm::vec3(m_ViewMatrix[2])); }
            void SetPosition(const glm::vec3 pos) { m_Position = pos; }
            void SetRotationSpeed(const float speed) {m_RotationSpeed = speed;}

            void SetOrbitTarget(const glm::vec3& target);
            void OrbitRotate(float deltaYaw, float deltaPitch);
            void OrbitZoom(float deltaRadius);
            void UpdateOrbitPosition();


            void SwitchToType(CameraType newType, glm::vec3 targetPos = glm::vec3{0.f});

            void OnResize(float width, float height);

        private:
            CameraType m_CameraType{CameraType::Free};
            glm::mat4 m_ProjectionMatrix{1.f};
            glm::mat4 m_ViewMatrix{1.f};
            glm::mat4 m_InverseViewMatrix{1.f};

            glm::vec3 m_Position{0.0f}; 
            glm::vec3 m_Rotation{0.0f}; 

            float m_MovementSpeed{5.0f};
            float m_RotationSpeed{0.001f}; 

            glm::vec3 m_Direction{0.0f, 0.0f, -1.0f};

            float m_Fov{80.0f};
            float m_Near{0.1f};
            float m_Far{100.f};
            float m_AspectRatio{0.f};

            bool m_IsSpedUp{false};

            glm::vec3 m_OrbitTarget{0.0f, 0.0f, 0.0f};
            float m_OrbitRadius{10.0f};
            float m_OrbitYaw{0.0f};
            float m_OrbitPitch{0.0f};

            bool m_OrbitAroundEntity{false};

            void RecalculateView();


    };
}



#endif //CAMERA_H

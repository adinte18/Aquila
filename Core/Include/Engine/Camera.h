//
// Created by alexa on 24/10/2024.
//

#ifndef CAMERA_H
#define CAMERA_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Engine
{
    class Camera
    {
        public:
            void SetOrthographicProjection(float left, float right, float top, float bottom,
                                          float near, float far);

            void SetPerspectiveProjection(float FOV_y, float aspect, float near, float far);

            [[nodiscard]] const glm::mat4& GetProjection() const {return projectionMatrix;}
            [[nodiscard]] const glm::mat4& GetView() const { return viewMatrix; }
            [[nodiscard]] const glm::mat4& GetInverseView() const { return inverseViewMatrix; }

            void SetViewDirection(glm::vec3 position,
                glm::vec3 direction, glm::vec3 up = glm::vec3(0.f, 1.f, -0.f));
            void SetViewTarget(glm::vec3 position,
                glm::vec3 target, glm::vec3 up = glm::vec3(0.f, 1.f, -0.f));

            void SetViewYXZ(glm::vec3 position, glm::vec3 rotation);

            void SpeedUp();

            void ResetSpeed();

            void MoveForward(float delta);
            void MoveBackward(float delta);
            void MoveRight(float delta);
            void MoveLeft(float delta);
            void Rotate(double yaw, double pitch); // Yaw and pitch for camera rotation
            void Zoom(float offset, float aspectRatio);

            [[nodiscard]] const glm::vec3& GetPosition() const { return position; }
            [[nodiscard]] const glm::vec3& GetRotation() const { return rotation; }
            [[nodiscard]] const glm::vec3& GetDirection() const { return direction; }
            [[nodiscard]] float GetZoom() const { return zoom; }

            void SetPosition(const glm::vec3 pos) { position = pos; }

            void OnResize(float width, float height);

        private:
            glm::mat4 projectionMatrix{1.f};
            glm::mat4 viewMatrix{1.f};
            glm::mat4 inverseViewMatrix{1.f};

            glm::vec3 position{0.0f}; // Camera position
            glm::vec3 rotation{0.0f}; // Camera rotation (yaw, pitch, roll)
            float movementSpeed{5.0f}; // Speed of movement
            float rotationSpeed{0.1f};  // Speed of rotation
            glm::vec3 direction{0.0f, 0.0f, -1.0f}; // Camera direction
            float zoom{45.0f}; // Field of view

            bool isSpedUp{false};

    };
}



#endif //CAMERA_H

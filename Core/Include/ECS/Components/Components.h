#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <memory>
#include "Engine/Model.h"
#include<glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Common.h"

namespace ECS {
    struct Mesh {
        std::shared_ptr<Engine::Model3D> mesh{};
    };

    struct Light {
        enum class LightType { Directional, Point, Spot };
        LightType type;

        glm::vec3 color{1.f};
        float intensity{0.5f};
        glm::vec3 direction{0.f};
        glm::vec3 position{0.f};

        glm::mat4 view{1.0f};
        glm::mat4 projection{1.0f};
        glm::mat4 lightSpaceMatrix{1.0f};

        explicit Light(const glm::vec3& pos = glm::vec3{-10.0f, 10.0f, -10.0f},
                       const glm::vec3& col = glm::vec3{1.f},
                       float intens = 0.5f,
                       LightType t = LightType::Directional)
            : type(t), color(col), intensity(intens), position(pos) {
            UpdateMatrices();
        }

        void SetLightViewMatrix(const glm::vec3& target = glm::vec3(0.f),
                                const glm::vec3& up = glm::vec3(0.0f, -1.0f, 0.0f)) {
            view = glm::lookAtRH(position, target, up);
            direction = glm::normalize(target - position);
        }

        void SetLightProjection(float left, float right, float top, float bottom, float near, float far) {
            projection = glm::orthoRH_ZO(left, right, bottom, top, near, far);
            projection[1][1] *= -1;
        }

        void UpdateMatrices() {
            //Day/Night simulation
            float angle = glfwGetTime() * 0.5f;
            float radius = 10.0f;

            position.x = cos(angle) * radius;
            position.z = sin(angle) * radius;
            position.y = 10.0f;

            SetLightViewMatrix(glm::vec3(0.f));
            SetLightProjection(-50.f, 50.f, -50.f, 50.f, 0.1f, 100.f);

            lightSpaceMatrix = projection * view;

            auto isnan_mat4 = [](const glm::mat4& mat) {
                for (int i = 0; i < 4; ++i) {
                    for (int j = 0; j < 4; ++j) {
                        if (glm::isnan(mat[i][j])) return true;
                    }
                }
                return false;
            };

            assert(!isnan_mat4(view) && "View matrix contains NaN values");
            assert(!isnan_mat4(projection) && "Projection matrix contains NaN values");
            assert(!isnan_mat4(lightSpaceMatrix) && "Light space matrix contains NaN values");
        }
    };



    struct Transform {
        glm::vec3 position{glm::vec3{0.f}};
        glm::quat rotation{glm::quat{1.f, 0.f, 0.f, 0.f}};
        glm::vec3 scale{glm::vec3{1.f}};

        explicit Transform(const glm::vec3 pos = glm::vec3{0.f}, const glm::quat rot = glm::quat{1.f, 0.f, 0.f, 0.f},
                           const glm::vec3 scl = glm::vec3{1.f})
            : position(pos), rotation(rot), scale(scl) {}

        [[nodiscard]] glm::mat4 TransformMatrix() const {
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
            glm::mat4 rotationMatrix = glm::toMat4(rotation);
            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
            return translationMatrix * rotationMatrix * scaleMatrix;
        }

        [[nodiscard]] glm::mat3 NormalMatrix() const {
            return glm::mat3_cast(rotation);
        }
    };

}

#endif //COMPONENTS_H

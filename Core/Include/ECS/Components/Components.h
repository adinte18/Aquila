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
        enum class LightType { Directional, Point } type { LightType::Directional };
        glm::vec3 color{ glm::vec3{1.f} };
        float intensity{ 0.5f };
        glm::vec3 direction{ glm::vec3{0.f} }; // For directional/spotlights
        glm::vec3 position{ glm::vec3{0.f, 0.f, 0.f} }; // For point/spotlights

        // The view and projection matrices for the light
        glm::mat4 view{ 1.0f };
        glm::mat4 projection{ 1.0f };

        explicit Light(const LightType t = LightType::Directional, const glm::vec3 col = glm::vec3{ 1.f },
                       const float intens = 0.5f, const glm::vec3 dir = glm::vec3{ 0.f, 0.f, -1.f },
                       const glm::vec3 pos = glm::vec3{ 0.f, -1.f, 0.f })
            : type(t), color(col), intensity(intens), direction(dir), position(pos)
        {
            updateMatrices();
        }

        void updateMatrices() {
            if (type == LightType::Directional) {
                view = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
                projection = glm::ortho(-1.5f, 1.5f, -1.5f, 1.5f, 0.1f, 50.f);
            } else if (type == LightType::Point) {
                view = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));
                projection = glm::ortho(-1.5f, 1.5f, -1.5f, 1.5f, 0.1f, 10.f);
            }
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

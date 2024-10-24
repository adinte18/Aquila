#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <memory>
#include "Engine/Model.h"
#include <glm/glm.hpp>
#include "Common.h"

namespace ECS {
    struct Mesh {
        std::shared_ptr<Engine::Model3D> mesh{};
    };

    struct Light {
        enum class LightType { Directional, Point, Spot } type {LightType::Directional};
        glm::vec3 color{glm::vec3{1.f}};
        float intensity{0.5f};
        glm::vec3 direction{glm::vec3{0.f}}; // For directional/spotlights
        glm::vec3 position{glm::vec3{0.f, -1.f, 0.f}};  // For point/spotlights

        explicit Light(LightType t = LightType::Directional, glm::vec3 col = glm::vec3{1.f},
          float inten = 0.5f, glm::vec3 dir = glm::vec3{0.f, 0.f, -1.f},
          glm::vec3 pos = glm::vec3{0.f, -1.f, 0.f})
        : type(t), color(col), intensity(inten), direction(dir), position(pos) {}
    };

    struct Transform {
        glm::vec3 position{glm::vec3{0.f}};
        glm::vec3 rotation{};
        glm::vec3 scale{glm::vec3{1.f}};

        explicit Transform(glm::vec3 pos = glm::vec3{0.f}, glm::vec3 rot = glm::vec3{0.f},
                  glm::vec3 scl = glm::vec3{1.f})
            : position(pos), rotation(rot), scale(scl) {}

        [[nodiscard]] glm::mat4 TransformMatrix() const {
            const float c3 = glm::cos(rotation.z);
            const float s3 = glm::sin(rotation.z);
            const float c2 = glm::cos(rotation.x);
            const float s2 = glm::sin(rotation.x);
            const float c1 = glm::cos(rotation.y);
            const float s1 = glm::sin(rotation.y);
            return glm::mat4{
                // Column 1 - X-axis transformation
                {
                    scale.x * (c1 * c3 + s1 * s2 * s3),  // Rotated and scaled X
                    scale.x * (c2 * s3),                 // Rotated and scaled Y
                    scale.x * (c1 * s2 * s3 - c3 * s1),  // Rotated and scaled Z
                    0.0f
                },
                // Column 2 - Y-axis transformation
                {
                    scale.y * (c3 * s1 * s2 - c1 * s3),  // Rotated and scaled X
                    scale.y * (c2 * c3),                 // Rotated and scaled Y
                    scale.y * (c1 * c3 * s2 + s1 * s3),  // Rotated and scaled Z
                    0.0f
                },
                // Column 3 - Z-axis transformation
                {
                    scale.z * (c2 * s1),                 // Rotated and scaled X
                    scale.z * (-s2),                     // Rotated and scaled Y
                    scale.z * (c1 * c2),                 // Rotated and scaled Z
                    0.0f
                },
                // Column 4 - Translation
                {
                    position.x,                       // Translate X
                    position.y,                       // Translate Y
                    position.z,                       // Translate Z
                    1.0f                                 // Homogeneous coordinate
                }
            };

        }

        [[nodiscard]] glm::mat3 NormalMatrix() const {
            const float c3 = glm::cos(rotation.z);
            const float s3 = glm::sin(rotation.z);
            const float c2 = glm::cos(rotation.x);
            const float s2 = glm::sin(rotation.x);
            const float c1 = glm::cos(rotation.y);
            const float s1 = glm::sin(rotation.y);
            const glm::vec3 invScale = 1.0f / scale;

            return glm::mat3{
              {
                  invScale.x * (c1 * c3 + s1 * s2 * s3),
                  invScale.x * (c2 * s3),
                  invScale.x * (c1 * s2 * s3 - c3 * s1),
              },
              {
                  invScale.y * (c3 * s1 * s2 - c1 * s3),
                  invScale.y * (c2 * c3),
                  invScale.y * (c1 * c3 * s2 + s1 * s3),
              },
              {
                  invScale.z * (c2 * s1),
                  invScale.z * (-s2),
                  invScale.z * (c1 * c2),
              },
          };
        }
};

}

#endif //COMPONENTS_H

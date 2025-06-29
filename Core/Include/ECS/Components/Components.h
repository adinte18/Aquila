#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "AquilaCore.h"
#include "GLFW/glfw3.h"

#include <memory>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

// Forward declarations
namespace Engine {
    class Mesh;
    class Texture2D;
}

namespace Engine {
    struct MeshCmp {
        Ref<Engine::Mesh> mesh{};
    };

    struct PBRMaterial {
        // PBR Textures
        Ref<Engine::Texture2D> albedoTexture;
        Ref<Engine::Texture2D> normalTexture;
        Ref<Engine::Texture2D> metallicRoughnessTexture;
        Ref<Engine::Texture2D> emissiveTexture;
        Ref<Engine::Texture2D> aoTexture;
        Ref<Engine::Texture2D> displacementTexture;

        // Texture file paths
        std::string albedoTexturePath;
        std::string normalTexturePath;
        std::string metallicRoughnessTexturePath;
        std::string emissiveTexturePath;
        std::string aoTexturePath;
        std::string displacementTexturePath;

        // PBR Material Properties
        glm::vec3 albedoColor {1.0f, 1.0f, 1.0f};         // Default white
        float metallic = 0.0f;                                  // Default non-metal
        float roughness = 0.5f;                                 // Default mid-roughness
        glm::vec3 emissionColor {0.0f, 0.0f, 0.0f};       // No emission by default
        float emissiveIntensity = 1.0f;                         // Default intensity
        float aoIntensity = 1.0f;                               // Ambient occlusion intensity

        int invertNormalMap = 0;                           // Invert normal map flag

        // UV Transform (Tiling & Offset)
        glm::vec2 tiling {1.0f, 1.0f};  // Default tiling
        glm::vec2 offset {0.0f, 0.0f};  // Default offset

        VkDescriptorSet descriptorSet;

        bool dirty;
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
            // Day/Night simulation
            float angle = glfwGetTime() * 0.5f;
            float radius = 10.0f;

            position.x = cos(angle) * radius;
            position.z = sin(angle) * radius;
            position.y = 10.0f;

            SetLightViewMatrix(glm::vec3(0.f));
            SetLightProjection(-50.f, 50.f, -50.f, 50.f, 0.1f, 60.f);

            lightSpaceMatrix = projection * view;

            auto isnan_mat4 = [](const glm::mat4& mat) {
                for (int i = 0; i < 4; ++i) {
                    for (int j = 0; j < 4; ++j) {
                        if (glm::isnan(mat[i][j])) return true;
                    }
                }
                return false;
            };

            AQUILA_CORE_ASSERT(!isnan_mat4(view) && "View matrix contains NaN values");
            AQUILA_CORE_ASSERT(!isnan_mat4(projection) && "Projection matrix contains NaN values");
            AQUILA_CORE_ASSERT(!isnan_mat4(lightSpaceMatrix) && "Light space matrix contains NaN values");
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

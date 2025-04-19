#ifndef SCENE_H
#define SCENE_H

#include "Entity.h"
#include <memory>
#include <Engine/Buffer.h>
#include <Engine/Descriptor.h>
#include <Engine/OffscreenRenderer.h>
#include <glm/glm.hpp>
#include <chrono>
#include <queue>

#include "Engine/Camera.h"

namespace ECS {
    struct uboLight {
        alignas(4) int type;
        alignas(16) glm::vec3 color;
        alignas(4) float intensity;
        alignas(16) glm::vec3 direction;
    };

    struct UniformData {
        alignas(16) glm::mat4 projection{1.f};
        alignas(16) glm::mat4 view{1.f};
        alignas(16) glm::mat4 inverseView{1.f};
        alignas(16) uboLight light{};
        alignas(16) glm::mat4 lightSpaceMatrix{1.f};
        alignas(16) glm::vec3 cameraPos{0.f};
    };

    struct MaterialData {
        alignas(16) glm::vec3 albedoColor {1.0f};
        alignas(4) float metallic = 0.0f;
        alignas(4) float roughness = 0.5f;
        alignas(16) glm::vec3 emissionColor {0.0f};
        alignas(4) float emissiveIntensity = 1.0f;
        alignas(4) float aoIntensity = 1.0f;
        alignas(8) glm::vec2 tiling {1.0f};
        alignas(8) glm::vec2 offset {0.0f};
        alignas(4) int invertNormalMap = 0;
    };

    class Scene {
    public:
        Scene();
        ~Scene() = default;

        std::shared_ptr<Entity> CreateEntity();
        void DestroyEntity(const Entity& entity);
        void Clear();

        [[nodiscard]] entt::registry& GetRegistry();
        [[nodiscard]] Engine::Camera& GetActiveCamera();
        [[nodiscard]] std::vector<Entity>& GetEntitesToDelete();
        void QueueForDestruction(entt::entity entity);
        void ClearQueue();

    private:
        Engine::Camera m_Camera;
        entt::registry m_Registry;
        std::vector<Entity> m_QueuedForDestruction{};
        std::queue<entt::entity> recycledEntityIDs;
    };
}



#endif

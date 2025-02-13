#ifndef SCENE_H
#define SCENE_H

#include "Entity.h"
#include <memory>
#include <Engine/Buffer.h>
#include <Engine/Descriptor.h>
#include <Engine/OffscreenRenderer.h>
#include <glm/glm.hpp>
#include <chrono>

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
        alignas(16) glm::mat4 lightView{1.f};
        alignas(16) glm::mat4 lightProjection{1.f};
    };

    class Scene {
    public:
        explicit Scene(Engine::Device &device, Engine::OffscreenRenderer &offscreenRenderer);
        ~Scene() = default;

        [[nodiscard]] VkDescriptorSet GetSceneDescriptorSet() const { return sceneDescriptorSet; }

        [[nodiscard]] Engine::DescriptorPool& GetMaterialDescriptorPool() const { return *materialDescriptorPool; }
        [[nodiscard]] Engine::DescriptorPool& GetSceneDescriptorPool() const { return *sceneDescriptorPool; }

        [[nodiscard]] Engine::DescriptorSetLayout& GetSceneDescriptorSetLayout() const { return *sceneDescriptorSetLayout; }
        [[nodiscard]] Engine::DescriptorSetLayout& GetMaterialDescriptorSetLayout() const { return *materialDescriptorSetLayout; }

        std::vector<Entity> queuedForDestruction{};

        VkDescriptorSet sceneView{};
        VkDescriptorSet sceneDescriptorSet{};

        VkSampler depthSampler{};
        VkImageView depthImageView{};

        // Create a new entity in the scene using the shared registry
        std::shared_ptr<Entity> CreateEntity();

        // Destroy an entity in the scene
        void DestroyEntity(const Entity &entity);

        // Clear all entities from the scene
        void Clear();

        // Update all entities or components in the scene (e.g., per frame)
        void OnUpdate();

        // Access the registry directly if needed
        entt::registry& GetRegistry();

        Engine::Camera& GetActiveCamera() { return camera; }
        [[nodiscard]] float GetFrameTime() const { return frameTime; }

    private:
        Engine::Camera camera;

        std::shared_ptr<Engine::DescriptorSetLayout> sceneDescriptorSetLayout;
        std::shared_ptr<Engine::DescriptorPool> sceneDescriptorPool;

        std::shared_ptr<Engine::DescriptorSetLayout> materialDescriptorSetLayout;
        std::shared_ptr<Engine::DescriptorPool> materialDescriptorPool;

        Engine::Buffer sceneBuffer;

        Engine::Device *device;
        entt::registry registry;
        Engine::OffscreenRenderer * renderer;
        float frameTime{0.f};
        std::chrono::time_point<std::chrono::steady_clock> currentTime{};
    };
}


#endif

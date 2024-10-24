#ifndef SCENE_H
#define SCENE_H

#include "Entity.h"
#include <memory>
#include <Engine/Buffer.h>
#include <Engine/Descriptor.h>
#include <glm/glm.hpp>

#include "Engine/Camera.h"

namespace ECS {
    struct DirectionalLight {
        glm::vec4 direction{0.f, 0.f, 1.f, 0.f};
        glm::vec4 color{1.f, 1.f, 1.f, 1.f};
    };


    struct UniformData {
        glm::mat4 projection{1.f};
        glm::mat4 view{1.f};
        glm::mat4 inverseView{1.f};
        glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};  // w is intensity
        DirectionalLight directionalLight;
        float shininess{1.f};
        float reflectionIntensity{1.f};
        float lightIntensity{1.f};
    };

    class Scene {
    public:
        explicit Scene(Engine::Device &device);
        ~Scene() = default;

        [[nodiscard]] Engine::DescriptorPool& GetMaterialDescriptorPool() const { return *materialDescriptorPool; }
        [[nodiscard]] Engine::DescriptorPool& GetSceneDescriptorPool() const { return *sceneDescriptorPool; }

        [[nodiscard]] Engine::DescriptorSetLayout& GetSceneDescriptorSetLayout() const { return *sceneDescriptorSetLayout; }
        [[nodiscard]] Engine::DescriptorSetLayout& GetMaterialDescriptorSetLayout() const { return *materialDescriptorSetLayout; }


        VkDescriptorSet sceneView{};
        VkDescriptorSet sceneDescriptorSet{};

        // Create a new entity in the scene using the shared registry
        std::shared_ptr<Entity> CreateEntity();

        // Destroy an entity in the scene
        void DestroyEntity(const std::shared_ptr<Entity> &entity);

        // Clear all entities from the scene
        void Clear();

        // Update all entities or components in the scene (e.g., per frame)
        void OnUpdate();

        // Access the registry directly if needed
        entt::registry& GetRegistry();

        Engine::Camera& GetActiveCamera() { return camera; }

    private:
        Engine::Camera camera;

        std::shared_ptr<Engine::DescriptorSetLayout> sceneDescriptorSetLayout;
        std::shared_ptr<Engine::DescriptorPool> sceneDescriptorPool;

        std::shared_ptr<Engine::DescriptorSetLayout> materialDescriptorSetLayout;
        std::shared_ptr<Engine::DescriptorPool> materialDescriptorPool;

        Engine::Buffer sceneBuffer;

        Engine::Device *device;
        entt::registry registry;
    };
}


#endif

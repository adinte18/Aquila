#include "ECS/Scene.h"

#include <Engine/Buffer.h>
#include <Engine/OffscreenRenderer.h>

#include "Components.h"

namespace ECS {
    Scene::Scene(Engine::Device& device, Engine::OffscreenRenderer& offscreenRenderer) :
        sceneBuffer{device, sizeof(UniformData), 1,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT},
        device(&device),
        renderer(&offscreenRenderer)
    {
        // Create a descriptor pool and set for the scene
        sceneDescriptorPool = Engine::DescriptorPool::Builder(device)
                .setMaxSets(100)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
                .build();

        sceneDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS) // ubo
                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //shadow map
                .build();

        sceneBuffer.map();

        auto sceneDescriptorInfo = sceneBuffer.vk_DescriptorInfo();

        Engine::DescriptorWriter writer(*sceneDescriptorSetLayout, *sceneDescriptorPool);
        writer.writeBuffer(0, &sceneDescriptorInfo);
        if (!writer.build(sceneDescriptorSet)) {
            throw std::runtime_error("Failed to update scene descriptor set!");
        }

        // Material descriptor pool and set. This is used for materials that are shared across multiple entities in the scene
        materialDescriptorPool = Engine::DescriptorPool::Builder(device)
                .setMaxSets(2048)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000) //Albedo
                .build();

        materialDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build();

        camera = Engine::Camera{};

        currentTime = std::chrono::steady_clock::now();
    }

    // Create a new entity in the scene using the shared registry
    std::shared_ptr<Entity> Scene::CreateEntity() {
        return std::make_shared<Entity>(registry, entt::entity{registry.create()});
    }

    // Destroy an entity in the scene
    void Scene::DestroyEntity(const Entity &entity) {
        registry.destroy(entity.GetHandle());
    }

    // Clear all entities from the scene
    void Scene::Clear() {
        registry.clear();
    }

    // Update all entities or components in the scene (e.g., per frame)
    void Scene::OnUpdate() {
        auto newTime = std::chrono::steady_clock::now();
        frameTime = std::chrono::duration<float>(newTime - currentTime).count();
        currentTime = newTime;

        UniformData ubo{};
        ubo.projection = camera.GetProjection();
        ubo.view = camera.GetView();
        ubo.inverseView = camera.GetInverseView();
        ubo.cameraPos = camera.GetPosition();

        bool lightFound = false;

        registry.view<ECS::Light>().each([&](auto entity, const Light& light) {
            if (registry.valid(entity)) {
                lightFound = true;
                ubo.light.type = static_cast<int>(light.type);
                ubo.light.direction = glm::normalize(light.direction);
                ubo.light.color = light.color;
                ubo.light.intensity = light.intensity;
                ubo.lightSpaceMatrix = light.lightSpaceMatrix;
            }
        });

        if (!lightFound) {
            ubo.light.type = -1;
        }

        sceneBuffer.vk_WriteToBuffer(&ubo);
        sceneBuffer.vk_Flush();
}

    // Access the registry directly if needed
    entt::registry& Scene::GetRegistry() {
        return registry;
    }

}

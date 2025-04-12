//
// Created by alexa on 06/04/2025.
//

#include "SceneContext.h"

ECS::SceneContext::SceneContext(Engine::Device &device, Scene &scene)
:   m_Device(device),
    m_Scene(scene),
    sceneBuffer{device, sizeof(UniformData), 1,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT}{
        // Create a descriptor pool and set for the scene
        m_SceneDescriptorPool = Engine::DescriptorPool::Builder(device)
                        .setMaxSets(100)
                        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
                        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
                        .build();

        m_SceneDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(device)
                        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS) // ubo
                        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //shadow map
                        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // env map
                        .build();

        sceneBuffer.map();

        auto sceneDescriptorInfo = sceneBuffer.vk_DescriptorInfo();

        Engine::DescriptorWriter writer(*m_SceneDescriptorSetLayout, *m_SceneDescriptorPool);
        writer.writeBuffer(0, &sceneDescriptorInfo);
        if (!writer.build(sceneDescriptorSet)) {
                throw std::runtime_error("Failed to update scene descriptor set!");
        }

        // Material descriptor pool and set. This is used for materials that are shared across multiple entities in the scene.
        m_MaterialDescriptorPool = Engine::DescriptorPool::Builder(device)
                        .setMaxSets(2048)
                        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000) // Albedo
                        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000) // Metallic-Roughness
                        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000) // Normal
                        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000) // Emission
                        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000) // AO
                        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000) // Material data (colors, factors)
                        .build();

        m_MaterialDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(device)
                        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                        // Albedo Texture
                        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                        // Metallic-Roughness Texture
                        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                        // Normal Texture
                        .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                        // Emission Texture
                        .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                    VK_SHADER_STAGE_FRAGMENT_BIT) // AO Texture
                        .addBinding(6, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT) // Material data
                        .build();
}

void ECS::SceneContext::UpdateGPUData() {
    auto newTime = std::chrono::steady_clock::now();

    UniformData ubo{};
    ubo.projection = m_Scene.GetActiveCamera().GetProjection();
    ubo.view = m_Scene.GetActiveCamera().GetView();
    ubo.inverseView = m_Scene.GetActiveCamera().GetInverseView();
    ubo.cameraPos = m_Scene.GetActiveCamera().GetPosition();

    bool lightFound = false;

    m_Scene.GetRegistry().view<ECS::Light>().each([&](auto entity, const Light& light) {
        if (m_Scene.GetRegistry().valid(entity)) {
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

    m_Scene.GetRegistry().view<ECS::Mesh, ECS::PBRMaterial>().each([&](auto entity, ECS::Mesh& model, ECS::PBRMaterial& material) {
        if (!m_Scene.GetRegistry().valid(entity)) return;

        MaterialData mubo{};
        mubo.albedoColor = material.albedoColor;
        mubo.metallic = material.metallic;
        mubo.roughness = material.roughness;
        mubo.emissionColor = material.emissionColor;
        mubo.emissiveIntensity = material.emissiveIntensity;
        mubo.aoIntensity = material.aoIntensity;
        mubo.tiling = material.tiling;
        mubo.offset = material.offset;
        mubo.invertNormalMap = material.invertNormalMap ? true : false;
        model.mesh->GetMaterialBuffer()->vk_WriteToBuffer(&mubo);
        model.mesh->GetMaterialBuffer()->vk_Flush();
    });


    sceneBuffer.vk_WriteToBuffer(&ubo);
    sceneBuffer.vk_Flush();
}

void ECS::SceneContext::RecreateDescriptors() {
}

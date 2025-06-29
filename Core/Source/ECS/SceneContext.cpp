//
// Created by alexa on 06/04/2025.
//

#include "ECS/SceneContext.h"
#include "Engine/DescriptorAllocator.h"

Engine::SceneContext::SceneContext(Engine::Device &device, Scene &scene)
:   m_Device(device),
    m_Scene(scene),
    sceneBuffer{device, sizeof(UniformData), 1,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT}{
        // Create a descriptor pool and set for the scene
        m_SceneDescriptorSetLayout = Engine::DescriptorSetLayout::Builder(device)
                        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS) // ubo
                        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //shadow map
                        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // env map
                        .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // prefilter map
                        .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // brdf LUT
                        .build();

        sceneBuffer.map();

        DescriptorAllocator::Allocate(m_SceneDescriptorSetLayout->GetDescriptorSetLayout(), sceneDescriptorSet);

        auto sceneDescriptorInfo = sceneBuffer.vk_DescriptorInfo();

        Engine::DescriptorWriter writer(*m_SceneDescriptorSetLayout, *DescriptorAllocator::GetSharedPool());
        writer.writeBuffer(0, &sceneDescriptorInfo);
        writer.overwrite(sceneDescriptorSet);

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

void Engine::SceneContext::UpdateGPUData() {
    UniformData ubo{};
    ubo.projection = m_Scene.GetActiveCamera().GetProjection();
    ubo.view = m_Scene.GetActiveCamera().GetView();
    ubo.inverseView = m_Scene.GetActiveCamera().GetInverseView();

    bool lightFound = false;

    m_Scene.GetRegistry().view<Engine::Light>().each([&](auto entity, const Light& light) {
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

    m_Scene.GetRegistry().view<Engine::MeshCmp, Engine::PBRMaterial>().each([&](auto entity, Engine::MeshCmp& model, Engine::PBRMaterial& material) {
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
        // model.mesh->GetMaterialBuffer()->vk_WriteToBuffer(&mubo);
        // model.mesh->GetMaterialBuffer()->vk_Flush();
    });


    sceneBuffer.vk_WriteToBuffer(&ubo);
    sceneBuffer.vk_Flush();
}

void Engine::SceneContext::RecreateDescriptors() {
}

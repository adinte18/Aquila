//
// Created by alexa on 06/04/2025.
//

#ifndef SCENECONTEXT_H
#define SCENECONTEXT_H



#include "Scene.h"
#include <Engine/Buffer.h>
#include <Engine/Descriptor.h>
#include <Components.h>
#include <Engine/Model.h>

namespace ECS {

    class SceneContext {
    public:
        SceneContext(Engine::Device& device, Scene& scene);
        ~SceneContext() = default;

        void UpdateGPUData();
        void RecreateDescriptors();
        void UpdateDescriptorSets(Engine::Texture2D &texture);

        [[nodiscard]] VkDescriptorSet GetSceneDescriptorSet() const { return sceneDescriptorSet; }

        [[nodiscard]] Engine::DescriptorPool& GetMaterialDescriptorPool() const { return *m_MaterialDescriptorPool; }
        [[nodiscard]] Engine::DescriptorPool& GetSceneDescriptorPool() const { return *m_SceneDescriptorPool; }

        [[nodiscard]] Engine::DescriptorSetLayout& GetSceneDescriptorSetLayout() const { return *m_SceneDescriptorSetLayout; }
        [[nodiscard]] Engine::DescriptorSetLayout& GetMaterialDescriptorSetLayout() const { return *m_MaterialDescriptorSetLayout; }

        [[nodiscard]] Scene& GetScene() const { return m_Scene; }
    private:
        Engine::Device& m_Device;
        Scene& m_Scene;

        std::shared_ptr<Engine::DescriptorSetLayout> m_SceneDescriptorSetLayout;
        std::shared_ptr<Engine::DescriptorPool> m_SceneDescriptorPool;

        std::shared_ptr<Engine::DescriptorSetLayout> m_MaterialDescriptorSetLayout;
        std::shared_ptr<Engine::DescriptorPool> m_MaterialDescriptorPool;

        Engine::Buffer sceneBuffer;
        VkDescriptorSet sceneDescriptorSet{};
    };
}




#endif //SCENECONTEXT_H

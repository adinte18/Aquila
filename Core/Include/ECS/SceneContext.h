//
// Created by alexa on 06/04/2025.
//

#ifndef SCENECONTEXT_H
#define SCENECONTEXT_H



#include "ECS/Scene.h"
#include "Engine/Buffer.h"
#include "Engine/Descriptor.h"
#include "ECS/Components/Components.h"
#include "Engine/Mesh.h"

namespace Engine {

    class SceneContext {
    public:
        SceneContext(Device& device, Scene& scene);
        ~SceneContext() = default;

        void UpdateGPUData();
        void RecreateDescriptors();
        void UpdateDescriptorSets(Texture2D &texture);

        [[nodiscard]] VkDescriptorSet GetSceneDescriptorSet() const { return sceneDescriptorSet; }

        [[nodiscard]] DescriptorPool& GetMaterialDescriptorPool() const { return *m_MaterialDescriptorPool; }
        [[nodiscard]] DescriptorPool& GetSceneDescriptorPool() const { return *m_SceneDescriptorPool; }

        [[nodiscard]] DescriptorSetLayout& GetSceneDescriptorSetLayout() const { return *m_SceneDescriptorSetLayout; }
        [[nodiscard]] DescriptorSetLayout& GetMaterialDescriptorSetLayout() const { return *m_MaterialDescriptorSetLayout; }

        [[nodiscard]] Scene& GetScene() const { return m_Scene; }
    private:
        Device& m_Device;
        Scene& m_Scene;

        Ref<DescriptorSetLayout> m_SceneDescriptorSetLayout;
        Ref<DescriptorPool> m_SceneDescriptorPool;

        Ref<DescriptorSetLayout> m_MaterialDescriptorSetLayout;
        Ref<DescriptorPool> m_MaterialDescriptorPool;

        Buffer sceneBuffer;
        VkDescriptorSet sceneDescriptorSet{};
    };
}




#endif //SCENECONTEXT_H

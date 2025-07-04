#ifndef RENDERING_SYSTEM_BASE_H
#define RENDERING_SYSTEM_BASE_H

#include "Engine/EditorCamera.h"
#include "Scene/Scene.h"
#include "Engine/Device.h"
#include "Engine/Pipeline.h"
#include "Engine/Buffer.h"
#include "Engine/Descriptor.h"
#include "Engine/DescriptorAllocator.h"

namespace Engine {

    
class RenderingSystemBase {
public:
    
    struct RenderContext {
        VkCommandBuffer commandBuffer;
        AquilaScene* scene = nullptr;
        VkDescriptorSet desciptorSet = VK_NULL_HANDLE;
        EditorCamera* camera;
        //...
    };

    RenderingSystemBase(Device& device);
    virtual ~RenderingSystemBase();

    RenderingSystemBase(const RenderingSystemBase&) = delete;
    RenderingSystemBase& operator=(const RenderingSystemBase&) = delete;

    void SetUniformData(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo);
    void SetTexture(uint32_t binding, const VkDescriptorImageInfo* imageInfo);
    
    virtual void SendDataToGPU();

    virtual void Render(const RenderContext& context) = 0;

protected:
    Device& device;

    Unique<Pipeline> m_Pipeline;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
    Unique<DescriptorSetLayout> m_Layout;

    std::unordered_map<uint32_t, VkDescriptorBufferInfo> m_UniformBuffers;
    std::unordered_map<uint32_t, VkDescriptorImageInfo> m_Textures;

    virtual void CreateDescriptorSetLayout() = 0;
    virtual void CreatePipeline(VkRenderPass renderPass) = 0;
    virtual void CreatePipelineLayout() = 0;

    void AllocateDescriptorSet();
};

} // namespace Engine

#endif

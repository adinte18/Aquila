//
// Created by alexa on 09/06/2025.
//

#include "Engine/Renderer/DescriptorAllocator.h"
#include "Engine/Renderer/Descriptor.h"

namespace Engine {

    Ref<DescriptorPool> DescriptorAllocator::s_GlobalPool = nullptr;

    /**
     * Init a global descriptor pool for all textures, materials, etc.
     * @param device
     */
    void DescriptorAllocator::Init(Device& device) {
        s_GlobalPool = DescriptorPool::Builder(device)
            .setMaxSets(1024)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128)
            .build();
    }

    bool DescriptorAllocator::Allocate(const VkDescriptorSetLayout layout, VkDescriptorSet &set) {
        return s_GlobalPool->allocateDescriptor(layout, set);
    }

    Ref<DescriptorPool> DescriptorAllocator::GetSharedPool() {
        return s_GlobalPool;
    }

    void DescriptorAllocator::Cleanup() {
        s_GlobalPool.reset();
    }

    void DescriptorAllocator::Release(std::vector<VkDescriptorSet>& sets) {
        s_GlobalPool->freeDescriptors(sets);
    }

} // namespace Engine

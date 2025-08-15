//
// Created by alexa on 09/06/2025.
//

#ifndef DESCRIPTOR_ALLOCATOR_H
#define DESCRIPTOR_ALLOCATOR_H

#include "Descriptor.h"

namespace Engine {
    class DescriptorAllocator {
    public:
        static void Init(Device& device);

        static bool Allocate(VkDescriptorSetLayout layout, VkDescriptorSet& set);
        static Ref<DescriptorPool> GetSharedPool();
        static void Cleanup();
        static void Release(std::vector<VkDescriptorSet>& sets);


    private:
        static Ref<DescriptorPool> s_GlobalPool; // NB : this is better, less memory overhead. efficient - i like it
    };
}

#endif // DESCRIPTOR_ALLOCATOR_H

#include "Engine/Framebuffer.h"

#include <array>

namespace Engine {
    Framebuffer::Framebuffer(Device& device, VkRenderPass renderPass)
        : device(device), fbRenderPass(renderPass) {}

    Framebuffer::~Framebuffer() {
        DestroyFramebuffer();
    }

	void Framebuffer::Resize(VkExtent2D newExtent) {
    	vkDeviceWaitIdle(device.vk_GetDevice());

    	vkDestroyFramebuffer(device.vk_GetDevice(), framebuffer, nullptr);
    	vkDestroyImageView(device.vk_GetDevice(), colorImageView, nullptr);
    	vkFreeMemory(device.vk_GetDevice(), colorImageMemory, nullptr);
    	vkDestroyImage(device.vk_GetDevice(), colorImage, nullptr);

    	CreateFramebuffer(newExtent, fbRenderPass);
	}


    void Framebuffer::CreateFramebuffer(VkExtent2D extent, VkRenderPass renderPass) {
    	fbRenderPass = renderPass;
		std::cout << "Got extent : " << extent.width << " " << extent.height << std::endl;

	    // Create the color attachment image
	    VkImageCreateInfo imageInfo{};
	    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	    imageInfo.imageType = VK_IMAGE_TYPE_2D;
	    imageInfo.extent.width = extent.width;
    	imageInfo.extent.height = extent.height;
	    imageInfo.extent.depth = 1;
	    imageInfo.mipLevels = 1;
	    imageInfo.arrayLayers = 1;
	    imageInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
	    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // Allow sampled usage for ImGui
	    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	    // Create the color image
	    vkCreateImage(device.vk_GetDevice(), &imageInfo, nullptr, &colorImage);

	    // Allocate memory for the color image
	    VkMemoryRequirements memRequirements;
	    vkGetImageMemoryRequirements(device.vk_GetDevice(), colorImage, &memRequirements);

	    VkMemoryAllocateInfo allocInfo{};
	    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	    allocInfo.allocationSize = memRequirements.size;
	    allocInfo.memoryTypeIndex = device.vk_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	    vkAllocateMemory(device.vk_GetDevice(), &allocInfo, nullptr, &colorImageMemory);
	    vkBindImageMemory(device.vk_GetDevice(), colorImage, colorImageMemory, 0);

	    // Create the color image view
	    VkImageViewCreateInfo viewInfo{};
	    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	    viewInfo.image = colorImage;
	    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	    viewInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
	    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	    viewInfo.subresourceRange.baseMipLevel = 0;
	    viewInfo.subresourceRange.levelCount = 1;
	    viewInfo.subresourceRange.baseArrayLayer = 0;
	    viewInfo.subresourceRange.layerCount = 1;

	    vkCreateImageView(device.vk_GetDevice(), &viewInfo, nullptr, &colorImageView);

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(device.vk_GetPhysicalDevice(), &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU =  VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV =  VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW =  VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		if (vkCreateSampler(device.vk_GetDevice(), &samplerInfo, nullptr, &colorSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}

	    // Create depth attachment image
	    VkImageCreateInfo depthImageInfo{};
	    depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	    depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
	    depthImageInfo.extent.width = extent.width;
	    depthImageInfo.extent.height = extent.height;
	    depthImageInfo.extent.depth = 1;
	    depthImageInfo.mipLevels = 1;
	    depthImageInfo.arrayLayers = 1;
	    depthImageInfo.format = VK_FORMAT_D32_SFLOAT; // or VK_FORMAT_D24_UNORM_S8_UINT
	    depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	    depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	    depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; // Use as depth stencil
	    depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	    depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	    vkCreateImage(device.vk_GetDevice(), &depthImageInfo, nullptr, &depthImage);

	    // Allocate memory for the depth image
	    vkGetImageMemoryRequirements(device.vk_GetDevice(), depthImage, &memRequirements);

	    allocInfo.allocationSize = memRequirements.size;
	    allocInfo.memoryTypeIndex = device.vk_FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	    vkAllocateMemory(device.vk_GetDevice(), &allocInfo, nullptr, &depthImageMemory);
	    vkBindImageMemory(device.vk_GetDevice(), depthImage, depthImageMemory, 0);

	    // Create depth image view
	    VkImageViewCreateInfo depthViewInfo{};
	    depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	    depthViewInfo.image = depthImage;
	    depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	    depthViewInfo.format = VK_FORMAT_D32_SFLOAT; // or VK_FORMAT_D24_UNORM_S8_UINT
	    depthViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	    depthViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	    depthViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	    depthViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	    depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // Use depth aspect
	    depthViewInfo.subresourceRange.baseMipLevel = 0;
	    depthViewInfo.subresourceRange.levelCount = 1;
	    depthViewInfo.subresourceRange.baseArrayLayer = 0;
	    depthViewInfo.subresourceRange.layerCount = 1;

	    vkCreateImageView(device.vk_GetDevice(), &depthViewInfo, nullptr, &depthImageView);

    	std::array<VkImageView, 2> attachments = { colorImageView, depthImageView };

        // Create the framebuffer info
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
    	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        // Create the framebuffer
        if (vkCreateFramebuffer(device.vk_GetDevice(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }

    	descriptorPool = DescriptorPool::Builder(device)
							  .setMaxSets(1)
							  .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
							  .build();

    	descriptorSetLayout = DescriptorSetLayout::Builder(device)
								 .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
								 .build();

    	VkDescriptorImageInfo descriptorInfo{};
    	descriptorInfo.sampler = colorSampler;
    	descriptorInfo.imageView = colorImageView;
    	descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    	DescriptorWriter writer(*descriptorSetLayout, *descriptorPool);
    	writer.writeImage(0, &descriptorInfo);
    	writer.build(descriptorSet);
    }

    void Framebuffer::DestroyFramebuffer() {
        if (framebuffer != VK_NULL_HANDLE) {
        	vkDestroyImageView(device.vk_GetDevice(), colorImageView, nullptr);
        	vkFreeMemory(device.vk_GetDevice(), colorImageMemory, nullptr);
        	vkDestroyImage(device.vk_GetDevice(), colorImage, nullptr);
        	vkDestroyImageView(device.vk_GetDevice(), depthImageView, nullptr);
        	vkFreeMemory(device.vk_GetDevice(), depthImageMemory, nullptr);
        	vkDestroyImage(device.vk_GetDevice(), depthImage, nullptr);
        	vkDestroySampler(device.vk_GetDevice(), colorSampler, nullptr);
            vkDestroyFramebuffer(device.vk_GetDevice(), framebuffer, nullptr);
            framebuffer = VK_NULL_HANDLE;
        }
    }
}

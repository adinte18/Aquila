#include "Engine/Renderpass.h"

namespace Engine {
    void Renderpass::DestroyAll() {
        // Destroy all render passes
        for (auto&[fst, snd] : renderPasses) {
            if (snd != VK_NULL_HANDLE) vkDestroyRenderPass(device.vk_GetDevice(), snd, nullptr);
        }

        // Destroy all framebuffers
        for (auto&[fst, snd] : framebuffers) {
            if (snd != VK_NULL_HANDLE) vkDestroyFramebuffer(device.vk_GetDevice(), snd, nullptr);
        }

        // Destroy all render targets
        for (auto& [fst, snd] : renderTargets) {
            if (snd->colorImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device.vk_GetDevice(), snd->colorImageView, nullptr);
                snd->colorImageView = VK_NULL_HANDLE;
            }
            if (snd->depthImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device.vk_GetDevice(), snd->depthImageView, nullptr);
                snd->depthImageView = VK_NULL_HANDLE;
            }
            if (snd->colorImage != VK_NULL_HANDLE) {
                vkDestroyImage(device.vk_GetDevice(), snd->colorImage, nullptr);
                snd->colorImage = VK_NULL_HANDLE;
            }
            if (snd->depthImage != VK_NULL_HANDLE) {
                vkDestroyImage(device.vk_GetDevice(), snd->depthImage, nullptr);
                snd->depthImage = VK_NULL_HANDLE;
            }
            if (snd->colorMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device.vk_GetDevice(), snd->colorMemory, nullptr);
                snd->colorMemory = VK_NULL_HANDLE;
            }
            if (snd->depthMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device.vk_GetDevice(), snd->depthMemory, nullptr);
                snd->depthMemory = VK_NULL_HANDLE;
            }
            if (snd->colorSampler != VK_NULL_HANDLE) {
                vkDestroySampler(device.vk_GetDevice(), snd->colorSampler, nullptr);
                snd->colorSampler = VK_NULL_HANDLE;
            }
            if (snd->depthSampler != VK_NULL_HANDLE) {
                vkDestroySampler(device.vk_GetDevice(), snd->depthSampler, nullptr);
                snd->depthSampler = VK_NULL_HANDLE;
            }
        }

    }

    void Renderpass::SetExtent(VkExtent2D extent) {
        this->extent = extent;
    }

    void Renderpass::CreateRenderTarget(Device& device, uint32_t width, uint32_t height) {
        // For SCENE, we need both color and depth attachments
        renderTargets[RenderPassType::SCENE] = std::make_shared<RenderTarget>(
            device, width, height, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, RenderTarget::AttachmentType::BOTH);

        // SHADOW pass, only depth attachment
        renderTargets[RenderPassType::SHADOW] = std::make_shared<RenderTarget>(
            device, 4096, 4096, VK_FORMAT_UNDEFINED, 0,
            VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, RenderTarget::AttachmentType::DEPTH);

        // SSAO pass, only color attachment
        renderTargets[RenderPassType::SSAO] = std::make_shared<RenderTarget>(
            device, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_FORMAT_UNDEFINED, 0, RenderTarget::AttachmentType::COLOR);

        // POST_PROCESSING pass, only color attachment
        renderTargets[RenderPassType::POST_PROCESSING] = std::make_shared<RenderTarget>(
            device, width, height, VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_FORMAT_UNDEFINED, 0, RenderTarget::AttachmentType::COLOR);
    }


    VkDescriptorSet Renderpass::GetImage(RenderPassType type) const {
        auto it = imageToViewport.find(type);
        if (it != imageToViewport.end()) {
            return it->second;
        } else {
            throw std::runtime_error("Descriptor Set for this RenderPassType not found!");
        }
    }


    VkImageView Renderpass::GetRenderTarget(RenderPassType type) const {
        auto it = renderTargets.find(type);
        if (it != renderTargets.end()) {
            if (it->second->colorImageView) {
                return it->second->colorImageView;
            }
            if (it->second->depthImageView) {
                return it->second->depthImageView;
            }
        } else {
            throw std::runtime_error("Render target for this RenderPassType not found!");
        }
        return VK_NULL_HANDLE;
    }

    VkSampler Renderpass::GetRenderSampler(RenderPassType type) const {
        auto it = renderTargets.find(type);
        if (it != renderTargets.end()) {
            if (it->second->colorSampler) {
                return it->second->colorSampler;
            }
            if (it->second->depthSampler) {
                return it->second->depthSampler;
            }
        } else {
            throw std::runtime_error("Render target for this RenderPassType not found!");
        }
        return VK_NULL_HANDLE;
    }

    void Renderpass::WriteImageToDescriptor(RenderPassType type){
        descriptorPools[type] = DescriptorPool::Builder(device)
        .setMaxSets(1)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
        .build();

       descriptorSetLayouts[type] = DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build();

        VkDescriptorImageInfo descriptorInfo{};
        if (renderTargets[type]->colorImageView) {
            descriptorInfo.sampler = renderTargets[type]->colorSampler;
            descriptorInfo.imageView = renderTargets[type]->colorImageView;
            descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  // For color
        } else if (renderTargets[type]->depthImageView) {
            descriptorInfo.sampler = renderTargets[type]->depthSampler;
            descriptorInfo.imageView = renderTargets[type]->depthImageView;
            descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;  // For depth
        }

        DescriptorWriter writer(*descriptorSetLayouts[type], *descriptorPools[type]);
        writer.writeImage(0, &descriptorInfo);
        writer.build(imageToViewport[type]);
    }


    void Renderpass::CreateFramebuffer(RenderPassType type){
        VkRenderPass renderPass = renderPasses[type];

        std::vector<VkImageView> attachments;
    
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        if (type == RenderPassType::SHADOW) {
            framebufferInfo.width = 4096;
            framebufferInfo.height = 4096;
        }else {
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
        }
        framebufferInfo.layers = 1;
    
        switch (type) {    
            case RenderPassType::SHADOW:
                attachments = { renderTargets[RenderPassType::SHADOW]->depthImageView }; // Depth-only
                break;
    
            case RenderPassType::SSAO:
                attachments = { renderTargets[RenderPassType::SSAO]->colorImageView }; // SSAO color output
                break;
    
            case RenderPassType::POST_PROCESSING:
                attachments = { renderTargets[RenderPassType::POST_PROCESSING]->colorImageView }; // Post-processing output
                break;
    
            case RenderPassType::SCENE:
                attachments = {
                    renderTargets[RenderPassType::SCENE]->colorImageView,
                    renderTargets[RenderPassType::SCENE]->depthImageView
                };
                break;
    
            default:
                throw std::runtime_error("Unsupported RenderPassType for framebuffer creation!");
        }
    
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
    
        if (vkCreateFramebuffer(device.vk_GetDevice(), &framebufferInfo, nullptr, &framebuffers[type]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer for render pass: " + std::to_string(static_cast<int>(type)));
        }

        WriteImageToDescriptor(type);
    }



    void Renderpass::CreateRenderPass(RenderPassType type) {
        switch (type) {
            case RenderPassType::G_BUFFER:
                CreateGBufferPass();
                break;
            case RenderPassType::SHADOW:
                CreateShadowPass();
                break;
            case RenderPassType::SSAO:
                CreateSSAOPass();
                break;
            case RenderPassType::POST_PROCESSING:
                CreatePostProcessingPass();
                break;
            case RenderPassType::SCENE:
                CreateScenePass();
                break;
            default:
                throw std::runtime_error("Unsupported RenderPassType!");
        }
    }

    VkRenderPass Renderpass::GetRenderPass(RenderPassType type) const {
        auto it = renderPasses.find(type);
        if (it != renderPasses.end()) {
            return it->second;
        }
        throw std::runtime_error("Unsupported RenderPassType!");
    }

    VkFramebuffer Renderpass::GetFramebuffer(RenderPassType type) const {
        auto it = framebuffers.find(type);
        if (it != framebuffers.end()) {
            return it->second;
        }
        throw std::runtime_error("Unsupported RenderPassType!");
    }
    

    // *********************************************
    // ***         RENDER PASSES CREATION        ***
    // *********************************************

    void Renderpass::CreateGBufferPass() {
    }

    void Renderpass::CreateShadowPass() {
        // Create the shadow render pass
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;  // Depth format
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // Clear the depth buffer at the start of the pass
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;  // Store the depth value
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // The depth image is in an undefined state at the start
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;  // Transition to read-only for depth comparison

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 0;  // Reference to the depth attachment
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Create the render pass for shadow rendering
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;  // No color attachments for the shadow pass
        subpass.pDepthStencilAttachment = &depthAttachmentRef;  // Attach the depth attachment

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;  // One attachment: depth
        renderPassInfo.pAttachments = &depthAttachment;
        renderPassInfo.subpassCount = 1;  // One subpass
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(device.vk_GetDevice(), &renderPassInfo, nullptr, &renderPasses[RenderPassType::SHADOW]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow render pass!");
        }

        CreateFramebuffer(RenderPassType::SHADOW);

        std::cout << "Created shadow render pass!" << std::endl;
    }

    void Renderpass::CreateSSAOPass() {
    }

    void Renderpass::CreatePostProcessingPass() {
    }

    void Renderpass::CreateScenePass()
    {
        //Define the color attachment
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB; // or VK_FORMAT_B8G8R8A8_SRGB for sRGB
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear before rendering
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the result
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil buffer
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Undefined layout at the start
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Layout for presentation

        // Define the depth attachment
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT; // or VK_FORMAT_D24_UNORM_S8_UINT for depth and stencil
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear before rendering
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Do not store depth
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Undefined layout at the start
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Layout for depth attachment

        // Specify the attachment references
        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0; // Index of the color attachment in the attachments array
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout during rendering

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1; // Index of the depth attachment in the attachments array
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Layout during rendering

        // Create a subpass
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Use graphics pipeline
        subpass.colorAttachmentCount = 1; // One color attachment
        subpass.pColorAttachments = &colorAttachmentRef; // Reference to the color attachment
        subpass.pDepthStencilAttachment = &depthAttachmentRef; // Reference to the depth attachment

        // Specify dependencies for the render pass
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // External subpass (before the first subpass)
        dependency.dstSubpass = 0; // First subpass
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Stage for the source
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Stage for the destination
        dependency.srcAccessMask = 0; // No access mask for the source
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Write access to color attachment
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT; // Dependency is local

        // Create the render pass
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 2; // Number of attachments
        VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
        renderPassInfo.pAttachments = attachments; // Pointer to the attachments
        renderPassInfo.subpassCount = 1; // Number of subpasses
        renderPassInfo.pSubpasses = &subpass; // Pointer to the subpasses
        renderPassInfo.dependencyCount = 1; // Number of dependencies
        renderPassInfo.pDependencies = &dependency; // Pointer to the dependencies

        // Create the render pass
        if (vkCreateRenderPass(device.vk_GetDevice(), &renderPassInfo, nullptr, &renderPasses[RenderPassType::SCENE]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

        CreateFramebuffer(RenderPassType::SCENE);

        std::cout << "Created scene render pass!" << std::endl;
    }

}
#include "Engine/RenderpassManager.h"

namespace Engine {
    void RenderpassManager::InvalidateFramebuffers() {
        // for (auto& [fst, snd] : framebuffers) {
        //     if (snd != VK_NULL_HANDLE) {
        //         // std::cout << "Destroying Framebuffer: " << snd << std::endl;
        //         vkDestroyFramebuffer(device.vk_GetDevice(), snd, nullptr);
        //     }
        // }
        // framebuffers.clear();
        //
        // for (auto& framebuffer : cubemapFaceFramebuffers) {
        //     if (framebuffer != VK_NULL_HANDLE) {
        //         vkDestroyFramebuffer(device.vk_GetDevice(), framebuffer, nullptr);
        //     }
        // }
        //
        // for (auto& framebuffer : irradianceFaceFramebuffers) {
        //     if (framebuffer != VK_NULL_HANDLE) {
        //         vkDestroyFramebuffer(device.vk_GetDevice(), framebuffer, nullptr);
        //     }
        // }
        //
        // for(auto& imageView : renderTargets[RenderPassType::ENV_TO_CUBEMAP]->cubemapFaceViews ) {
        //     vkDestroyImageView(device.vk_GetDevice(), imageView, nullptr);
        // }
        //
        // for(auto& imageView : renderTargets[RenderPassType::IBL]->cubemapFaceViews ) {
        //     vkDestroyImageView(device.vk_GetDevice(), imageView, nullptr);
        // }

        // for (auto& [fst, snd] : renderTargets) {
        //     if (snd) {
        //         if (snd->colorTexture) snd->colorTexture->DestroyAll();
        //         if (snd->depthTexture) snd->depthTexture->DestroyAll();
        //     }
        // }
        // renderTargets.clear();
    }

    void RenderpassManager::DestroyAll() {
        vkDeviceWaitIdle(device.vk_GetDevice());

        // for (auto& [fst, snd] : framebuffers) {
        //     if (snd != VK_NULL_HANDLE) {
        //         // std::cout << "Destroying Framebuffer: " << snd << std::endl;
        //         vkDestroyFramebuffer(device.vk_GetDevice(), snd, nullptr);
        //     }
        // }
        // framebuffers.clear();
        //
        // for (auto& framebuffer : cubemapFaceFramebuffers) {
        //     if (framebuffer != VK_NULL_HANDLE) {
        //         vkDestroyFramebuffer(device.vk_GetDevice(), framebuffer, nullptr);
        //     }
        // }
        //
        // for (auto& framebuffer : irradianceFaceFramebuffers) {
        //     if (framebuffer != VK_NULL_HANDLE) {
        //         vkDestroyFramebuffer(device.vk_GetDevice(), framebuffer, nullptr);
        //     }
        // }
        //
        // for (auto& [fst, snd] : renderPasses) {
        //     if (snd != VK_NULL_HANDLE) {
        //         vkDestroyRenderPass(device.vk_GetDevice(), snd, nullptr);
        //     }
        // }
        // renderPasses.clear();
        //
        // // for(auto& imageView : renderTargets[RenderPassType::ENV_TO_CUBEMAP]->cubemapFaceViews ) {
        // //     vkDestroyImageView(device.vk_GetDevice(), imageView, nullptr);
        // // }
        // //
        // // for(auto& imageView : renderTargets[RenderPassType::IBL]->cubemapFaceViews ) {
        // //     vkDestroyImageView(device.vk_GetDevice(), imageView, nullptr);
        // // }
        //
        // for (auto& [fst, snd] : renderTargets) {
        //     if (snd) {
        //         if (snd->colorTexture) snd->colorTexture->DestroyAll();
        //         if (snd->depthTexture) snd->depthTexture->DestroyAll();
        //     }
        // }
        // renderTargets.clear();
    }

    void RenderpassManager::SetExtent(VkExtent2D extent) {
        this->extent = extent;
    }

    void RenderpassManager::CreateRenderTarget(Device& device, uint32_t width, uint32_t height) {
        // For SCENE, we need both color and depth attachments
        renderTargets[RenderPassType::GEOMETRY] = std::make_shared<RenderTarget>(
            device, width, height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, RenderTarget::AttachmentType::BOTH);

        // SHADOW pass, only depth attachment
        renderTargets[RenderPassType::SHADOW] = std::make_shared<RenderTarget>(
            device, 8192, 8192, VK_FORMAT_UNDEFINED, 0,
            VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, RenderTarget::AttachmentType::DEPTH);

        // SSAO pass, only color attachment
        renderTargets[RenderPassType::SSAO] = std::make_shared<RenderTarget>(
            device, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_FORMAT_UNDEFINED, 0, RenderTarget::AttachmentType::COLOR);

        renderTargets[RenderPassType::GRID] = std::make_shared<RenderTarget>(
            device, width, height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_FORMAT_UNDEFINED, 0, RenderTarget::AttachmentType::COLOR);

        renderTargets[RenderPassType::ENV_TO_CUBEMAP] = std::make_shared<RenderTarget>(
            device, 1024, 1024, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_FORMAT_UNDEFINED, 0, RenderTarget::AttachmentType::COLOR, RenderTarget::TargetType::CUBEMAP);

        renderTargets[RenderPassType::IBL] = std::make_shared<RenderTarget>(
            device, 64, 64, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_FORMAT_UNDEFINED, 0, RenderTarget::AttachmentType::COLOR, RenderTarget::TargetType::CUBEMAP);

        // POST_PROCESSING pass, only color attachment
        renderTargets[RenderPassType::POST_PROCESSING] = std::make_shared<RenderTarget>(
            device, width, height, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_FORMAT_UNDEFINED, 0, RenderTarget::AttachmentType::COLOR);

        renderTargets[RenderPassType::FINAL] = std::make_shared<RenderTarget>(
            device, width, height, VK_FORMAT_B8G8R8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_FORMAT_UNDEFINED, 0, RenderTarget::AttachmentType::COLOR);
    }


    VkDescriptorSet& RenderpassManager::GetDescriptorSet(RenderPassType type) {
        auto it = imageToViewport.find(type);
        if (it != imageToViewport.end()) {
            return it->second;
        } else {
            throw std::runtime_error("Descriptor Set for this RenderPassType not found!");
        }
    }

    Unique<DescriptorPool>& RenderpassManager::GetDescriptorPool(RenderPassType type) {
        auto it = descriptorPools.find(type);
        if (it != descriptorPools.end()) {
            return it->second;
        } else {
            throw std::runtime_error("Descriptor Pool for this RenderPassType not found!");
        }
    }

    Unique<DescriptorSetLayout>& RenderpassManager::GetDescriptorLayout(RenderPassType type) {
        auto it = descriptorSetLayouts.find(type);
        if (it != descriptorSetLayouts.end()) {
            return it->second;
        } else {
            throw std::runtime_error("Descriptor Pool for this RenderPassType not found!");
        }
    }



    VkImage RenderpassManager::GetImage(RenderPassType type) const {
        auto it = renderTargets.find(type);
        if (it != renderTargets.end()) {
            if (it->second->colorTexture->GetTextureImage()) {
                return it->second->colorTexture->GetTextureImage();
            }
            if (it->second->depthTexture->GetTextureImageView()) {
                return it->second->depthTexture->GetTextureImage();
            }
        } else {
            throw std::runtime_error("Image for this RenderPassType not found!");
        }
        return VK_NULL_HANDLE;

    }

    VkImageView RenderpassManager::GetRenderTarget(RenderPassType type) const {
        auto it = renderTargets.find(type);
        if (it != renderTargets.end()) {
            if (it->second->colorTexture->GetTextureImageView()) {
                return it->second->colorTexture->GetTextureImageView();
            }
            if (it->second->depthTexture->GetTextureImageView()) {
                return it->second->depthTexture->GetTextureImageView();
            }
        } else {
            throw std::runtime_error("Render target for this RenderPassType not found!");
        }
        return VK_NULL_HANDLE;
    }

    VkSampler RenderpassManager::GetRenderSampler(RenderPassType type) const {
        auto it = renderTargets.find(type);
        if (it != renderTargets.end()) {
            if (it->second->colorTexture->GetTextureSampler()) {
                return it->second->colorTexture->GetTextureSampler();
            }
            if (it->second->depthTexture->GetTextureSampler()) {
                return it->second->depthTexture->GetTextureSampler();
            }
        } else {
            throw std::runtime_error("Render target for this RenderPassType not found!");
        }
        return VK_NULL_HANDLE;
    }

    void RenderpassManager::WriteImageToDescriptor(RenderPassType type){
        descriptorPools[type] = DescriptorPool::Builder(device)
        .setMaxSets(1)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)
        .build();

       descriptorSetLayouts[type] = DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build();

        VkDescriptorImageInfo descriptorInfo{};
        if (renderTargets[type]->colorTexture->HasImageView()) {
            descriptorInfo.sampler = renderTargets[type]->colorTexture->GetTextureSampler();
            descriptorInfo.imageView = renderTargets[type]->colorTexture->GetTextureImageView();
            descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  // For color
        } else if (renderTargets[type]->depthTexture->HasImageView()) {
            descriptorInfo.sampler = renderTargets[type]->depthTexture->GetTextureSampler();
            descriptorInfo.imageView = renderTargets[type]->depthTexture->GetTextureImageView();
            descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;  // For depth
        }

        DescriptorWriter writer(*descriptorSetLayouts[type], *descriptorPools[type]);
        writer.writeImage(0, &descriptorInfo);
        writer.build(imageToViewport[type]);
    }

    void RenderpassManager::CreateFramebuffer(RenderPassType type) {
        if (framebuffers.find(type) != framebuffers.end()) {
            vkDestroyFramebuffer(device.vk_GetDevice(), framebuffers[type], nullptr);
        }

        VkRenderPass renderPass = renderPasses[type];
        std::vector<VkImageView> attachments;
        VkFramebufferCreateInfo framebufferInfo{};

        if (type == RenderPassType::ENV_TO_CUBEMAP) {
            for (uint32_t face = 0; face < 6; face++) {
                std::vector<VkImageView> faceAttachments = {
                    // renderTargets[RenderPassType::ENV_TO_CUBEMAP]->cubemapFaceViews[face]
                };

                VkFramebufferCreateInfo faceFramebufferInfo{};
                faceFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                faceFramebufferInfo.renderPass = renderPass;
                faceFramebufferInfo.width = 1024;
                faceFramebufferInfo.height = 1024;
                faceFramebufferInfo.layers = 1;
                faceFramebufferInfo.attachmentCount = static_cast<uint32_t>(faceAttachments.size());
                faceFramebufferInfo.pAttachments = faceAttachments.data();

                if (vkCreateFramebuffer(device.vk_GetDevice(), &faceFramebufferInfo, nullptr,
                                      &cubemapFaceFramebuffers[face]) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create framebuffer for cubemap face: " + std::to_string(face));
                }

                // std::cout << "Created framebuffer for cubemap face: " << static_cast<int>(RenderPassType::ENV_TO_CUBEMAP) + face + 1 << std::endl;
            }

            WriteImageToDescriptor(type);
        }
        else if (type == RenderPassType::IBL) {
            for (uint32_t face = 0; face < 6; face++) {
                std::vector<VkImageView> faceAttachments = {
                    // renderTargets[RenderPassType::IBL]->cubemapFaceViews[face]
                };

                VkFramebufferCreateInfo faceFramebufferInfo{};
                faceFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                faceFramebufferInfo.renderPass = renderPass;
                faceFramebufferInfo.width = 64;
                faceFramebufferInfo.height = 64;
                faceFramebufferInfo.layers = 1;
                faceFramebufferInfo.attachmentCount = static_cast<uint32_t>(faceAttachments.size());
                faceFramebufferInfo.pAttachments = faceAttachments.data();

                if (vkCreateFramebuffer(device.vk_GetDevice(), &faceFramebufferInfo, nullptr,
                                      &irradianceFaceFramebuffers[face]) != VK_SUCCESS) {
                    throw std::runtime_error("Failed to create framebuffer for cubemap face: " + std::to_string(face));
                }

                // std::cout << "Created framebuffer for cubemap face ADOAKDOAKDOA: " << static_cast<int>(RenderPassType::IBL) + face + 1 << std::endl;
            }

            WriteImageToDescriptor(type);
        }

        else {
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.width = (type == RenderPassType::SHADOW) ? 8192 : extent.width;
            framebufferInfo.height = (type == RenderPassType::SHADOW) ? 8192 : extent.height;
            framebufferInfo.layers = 1;  // Set layers to 1 for 2D textures, or 6 for cubemaps if needed.

            switch (type) {
                case RenderPassType::SHADOW:
                    attachments = { renderTargets[RenderPassType::SHADOW]->depthTexture->GetTextureImageView() };
                    break;
                case RenderPassType::SSAO:
                    attachments = { renderTargets[RenderPassType::SSAO]->colorTexture->GetTextureImageView() };
                    break;
                case RenderPassType::GRID:
                    attachments = { renderTargets[RenderPassType::GRID]->colorTexture->GetTextureImageView() };
                    break;
                case RenderPassType::POST_PROCESSING:
                    attachments = { renderTargets[RenderPassType::POST_PROCESSING]->colorTexture->GetTextureImageView() };
                    break;
                case RenderPassType::GEOMETRY:
                    attachments = {
                        renderTargets[RenderPassType::GEOMETRY]->colorTexture->GetTextureImageView(),
                        renderTargets[RenderPassType::GEOMETRY]->depthTexture->GetTextureImageView()
                    };
                    break;
                case RenderPassType::FINAL:
                    attachments = { renderTargets[RenderPassType::FINAL]->colorTexture->GetTextureImageView() };
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
    }


    void RenderpassManager::CreateRenderPass(RenderPassType type) {
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
            case RenderPassType::GEOMETRY:
                CreateGeometryPass();
                break;
            case RenderPassType::GRID:
                CreateGridPass();
                break;
            case RenderPassType::FINAL:
                CreateFinalPass();
                break;
            case RenderPassType::ENV_TO_CUBEMAP:
                CreateCubemapPass();
                break;
            case RenderPassType::IBL:
                CreateIBLPass();
                break;
            default:
                throw std::runtime_error("Unsupported RenderPassType!");
        }
    }

    VkRenderPass RenderpassManager::GetRenderPass(RenderPassType type) const {
        auto it = renderPasses.find(type);
        if (it != renderPasses.end()) {
            return it->second;
        }
        throw std::runtime_error("Unsupported RenderPassType!");
    }

    VkFramebuffer RenderpassManager::GetFramebuffer(RenderPassType type) const {
        auto it = framebuffers.find(type);
        if (it != framebuffers.end()) {
            return it->second;
        }
        throw std::runtime_error("Unsupported RenderPassType!");
    }

    VkFramebuffer RenderpassManager::GetCubemapFramebuffer(int faceIndex) const {
        if (faceIndex < 0 || faceIndex >= 6) {
            throw std::runtime_error("Invalid cubemap face index!");
        }
        return cubemapFaceFramebuffers[faceIndex];
    }

    VkFramebuffer RenderpassManager::GetIrradianceFramebuffer(int faceIndex) const {
        if (faceIndex < 0 || faceIndex >= 6) {
            throw std::runtime_error("Invalid cubemap face index!");
        }
        return irradianceFaceFramebuffers[faceIndex];
    }


    // *********************************************
    // ***         RENDER PASSES CREATION        ***
    // *********************************************

    void RenderpassManager::CreateGBufferPass() {
    }

    void RenderpassManager::CreateCubemapPass() {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        std::vector<VkSubpassDependency> dependencies;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        // Create the render pass
        if (vkCreateRenderPass(device.vk_GetDevice(), &renderPassInfo, nullptr, &renderPasses[RenderPassType::ENV_TO_CUBEMAP]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cubemap render pass!");
        }

        CreateFramebuffer(RenderPassType::ENV_TO_CUBEMAP);

        std::cout << "Cubemap Render Pass created successfully!" << std::endl;
    }

    void RenderpassManager::CreateShadowPass() {
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

    void RenderpassManager::CreateSSAOPass() {
    }

    void RenderpassManager::CreatePostProcessingPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No need to keep previous data
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store result for presentation
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        // Subpass dependency to ensure scene output is ready for post-processing
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device.vk_GetDevice(), &renderPassInfo, nullptr, &renderPasses[RenderPassType::POST_PROCESSING]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create post-processing render pass!");
        }

        CreateFramebuffer(RenderPassType::POST_PROCESSING);

        std::cout << "Created post-processing render pass!" << std::endl;

    }

    void RenderpassManager::CreateGridPass() {
        // Color attachment description
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Subpass description
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;


        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // Render pass creation info
        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        // Create the render pass
        if (vkCreateRenderPass(device.vk_GetDevice(), &renderPassInfo, nullptr, &renderPasses[RenderPassType::GRID]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create grid render pass!");
        }

        CreateFramebuffer(RenderPassType::GRID);

        std::cout << "Created grid render pass!" << std::endl;
    }

    void RenderpassManager::CreateIBLPass() {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        std::vector<VkSubpassDependency> dependencies;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        // Create the render pass
        if (vkCreateRenderPass(device.vk_GetDevice(), &renderPassInfo, nullptr, &renderPasses[RenderPassType::IBL]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cubemap render pass!");
        }

        CreateFramebuffer(RenderPassType::IBL);

        std::cout << "Irradiance Render Pass created successfully!" << std::endl;

    }
    void RenderpassManager::CreateGeometryPass()
    {
        //Define the color attachment
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM; // or VK_FORMAT_B8G8R8A8_UNORM for sRGB
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear before rendering
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the result
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // No stencil buffer
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Undefined layout at the start
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout for presentation

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
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

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
        if (vkCreateRenderPass(device.vk_GetDevice(), &renderPassInfo, nullptr, &renderPasses[RenderPassType::GEOMETRY]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

        CreateFramebuffer(RenderPassType::GEOMETRY);

        std::cout << "Created scene render pass!" << std::endl;
    }

void RenderpassManager::CreateFinalPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // ImGui uses it

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Subpass dependency to ensure proper synchronization
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device.vk_GetDevice(), &renderPassInfo, nullptr, &renderPasses[RenderPassType::FINAL]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create final render pass!");
    }

        CreateFramebuffer(RenderPassType::FINAL);

        std::cout << "Created composite/final render pass!" << std::endl;

}

}
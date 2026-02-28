// DynamicRenderingHelper.h - Extended with additional format factories
#ifndef AQUILA_DYNAMIC_RENDERING_HELPER_H
#define AQUILA_DYNAMIC_RENDERING_HELPER_H

#include "Aquila/Core/Defines.h"
#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Graphics/Pipeline/Rendertarget.h"
#include "Aquila/Graphics/Texture/ImageOperations.h"

namespace Aquila::Graphics::Helpers {

/**
 * @brief Simple helper to begin dynamic rendering with a RenderTarget
 * This replaces the old BeginRenderPass pattern
 */
class DynamicRendering {
  public:
	/**
     * @brief Begin rendering to a single color attachment (no depth)
     */
	static void Begin(VkCommandBuffer cmd, const Ref<RenderingPipeline::RenderTarget> &target, bool clear = true,
					  VkClearColorValue clearColor = { { 0.0F, 0.0F, 0.0F, 1.0F } }) {
		auto spec = target->GetSpec();

		VkRenderingAttachmentInfo colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachment.imageView = target->GetColorAttachment()->GetImageView();
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue.color = clearColor;

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.extent = { .width = spec.width, .height = spec.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;

		vkCmdBeginRendering(cmd, &renderingInfo);

		// Set viewport and scissor
		VkViewport viewport{};
		viewport.width = static_cast<float>(spec.width);
		viewport.height = static_cast<float>(spec.height);
		viewport.maxDepth = 1.0F;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.extent = { .width = spec.width, .height = spec.height };
		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	/**
     * @brief Begin rendering to GBuffer (3 color attachments + depth)
     */
	static void BeginGBuffer(VkCommandBuffer cmd, const Ref<RenderingPipeline::RenderTarget> &gbuffer,
							 bool clear = true) {
		auto spec = gbuffer->GetSpec();
		auto colorAttachments = gbuffer->GetColorAttachments();

		std::array<VkRenderingAttachmentInfo, 3> colorInfos{};
		for (size_t i = 0; i < 3; ++i) {
			colorInfos[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			colorInfos[i].imageView = colorAttachments[i]->GetImageView();
			colorInfos[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorInfos[i].loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			colorInfos[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorInfos[i].clearValue.color = { { 0.0F, 0.0F, 0.0F, 1.0F } };
		}

		VkRenderingAttachmentInfo depthInfo{};
		depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthInfo.imageView = gbuffer->GetDepthAttachment()->GetImageView();
		depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		depthInfo.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		depthInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthInfo.clearValue.depthStencil = { .depth = 1.0F, .stencil = 0 };

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.extent = { .width = spec.width, .height = spec.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = static_cast<uint32>(colorInfos.size());
		renderingInfo.pColorAttachments = colorInfos.data();
		renderingInfo.pDepthAttachment = &depthInfo;

		vkCmdBeginRendering(cmd, &renderingInfo);

		VkViewport viewport{};
		viewport.width = static_cast<float>(spec.width);
		viewport.height = static_cast<float>(spec.height);
		viewport.maxDepth = 1.0F;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.extent = { .width = spec.width, .height = spec.height };
		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	static void BeginSwapchain(VkCommandBuffer cmd, VkImageView targetImageView, VkExtent2D extent,
							   bool clear = false) {
		VkRenderingAttachmentInfo colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachment.pNext = nullptr;

		colorAttachment.imageView = targetImageView;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.extent = extent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;

		vkCmdBeginRendering(cmd, &renderingInfo);

		VkViewport viewport{};
		viewport.width = static_cast<float>(extent.width);
		viewport.height = static_cast<float>(extent.height);
		viewport.maxDepth = 1.0F;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.extent = { .width = extent.width, .height = extent.height };
		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	/**
     * @brief Begin rendering depth-only (for shadows)
     */
	static void BeginDepthOnly(VkCommandBuffer cmd, VkImageView depthView, uint32 size, bool clear = true) {
		VkRenderingAttachmentInfo depthInfo{};
		depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthInfo.imageView = depthView;
		depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		depthInfo.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		depthInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthInfo.clearValue.depthStencil = { .depth = 1.0F, .stencil = 0 };

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.extent = { size, size };
		renderingInfo.layerCount = 1;
		renderingInfo.pDepthAttachment = &depthInfo;

		vkCmdBeginRendering(cmd, &renderingInfo);

		VkViewport viewport{};
		viewport.width = static_cast<float>(size);
		viewport.height = static_cast<float>(size);
		viewport.maxDepth = 1.0F;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.extent = { size, size };
		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	/**
     * @brief Begin rendering with custom color attachment using external depth
     * (e.g., Skybox using GBuffer's depth)
     */
	static void BeginWithExternalDepth(VkCommandBuffer cmd, const Ref<RenderingPipeline::RenderTarget> &colorTarget,
									   VkImageView externalDepthView, bool clearColor = true, bool writeDepth = false) {
		auto spec = colorTarget->GetSpec();

		VkRenderingAttachmentInfo colorInfo{};
		colorInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorInfo.imageView = colorTarget->GetColorAttachment()->GetImageView();
		colorInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorInfo.loadOp = clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		colorInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorInfo.clearValue.color = { { 0.0F, 0.0F, 0.0F, 1.0F } };

		VkRenderingAttachmentInfo depthInfo{};
		depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthInfo.imageView = externalDepthView;
		depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		depthInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Keep existing depth
		depthInfo.storeOp = writeDepth ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_NONE;

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.extent = { .width = spec.width, .height = spec.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorInfo;
		renderingInfo.pDepthAttachment = &depthInfo;

		vkCmdBeginRendering(cmd, &renderingInfo);

		VkViewport viewport{};
		viewport.width = static_cast<float>(spec.width);
		viewport.height = static_cast<float>(spec.height);
		viewport.maxDepth = 1.0F;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.extent = { .width = spec.width, .height = spec.height };
		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	/**
     * @brief End rendering (replaces vkCmdEndRenderPass)
     */
	static void End(VkCommandBuffer cmd) { vkCmdEndRendering(cmd); }

	static void TransitionImages(Device &device, VkCommandBuffer cmd, const std::vector<VkImage> &colorImages,
								 VkFormat colorFormat, VkImage depthImage, VkFormat depthFormat,
								 VkImageLayout oldLayout, VkImageLayout newLayout, uint32 mipLevels = 1,
								 uint32 layers = 1) {
		for (auto *image : colorImages) {
			Texture::ImageOperations::TransitionLayout(device, cmd, image, colorFormat, oldLayout, newLayout, mipLevels,
													   layers);
		}

		if (depthImage != VK_NULL_HANDLE) {
			Texture::ImageOperations::TransitionLayout(device, cmd, depthImage, depthFormat, oldLayout, newLayout,
													   mipLevels, layers);
		}
	}
};

/**
 * @brief Helper to create VkPipelineRenderingCreateInfo for pipeline creation
 */
struct PipelineRenderingFormats {
	std::vector<VkFormat> colorFormats;
	VkFormat depthFormat = VK_FORMAT_UNDEFINED;

	static PipelineRenderingFormats GBuffer() {
		return { .colorFormats = { VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT,
								   VK_FORMAT_R8G8B8A8_UNORM },
				 .depthFormat = VK_FORMAT_D32_SFLOAT };
	}

	static PipelineRenderingFormats SkyBox() {
		return { .colorFormats = { VK_FORMAT_R32G32B32A32_SFLOAT }, .depthFormat = VK_FORMAT_D32_SFLOAT };
	}

	static PipelineRenderingFormats SingleColor(VkFormat format = VK_FORMAT_R8G8B8A8_UNORM) {
		return { .colorFormats = { format }, .depthFormat = VK_FORMAT_UNDEFINED };
	}

	static PipelineRenderingFormats DepthOnly(VkFormat depthFormat = VK_FORMAT_D32_SFLOAT) {
		return { .colorFormats = {}, .depthFormat = depthFormat };
	}

	static PipelineRenderingFormats Lighting() {
		return { .colorFormats = { VK_FORMAT_R16G16B16A16_SFLOAT }, .depthFormat = VK_FORMAT_UNDEFINED };
	}

	static PipelineRenderingFormats Composite() {
		return { .colorFormats = { VK_FORMAT_R16G16B16A16_SFLOAT }, .depthFormat = VK_FORMAT_UNDEFINED };
	}
	static PipelineRenderingFormats Shadow() { return DepthOnly(); }

	static PipelineRenderingFormats Gizmo() {
		return { .colorFormats = { VK_FORMAT_R16G16B16A16_SFLOAT }, .depthFormat = VK_FORMAT_D32_SFLOAT };
	}

	static PipelineRenderingFormats Custom(const std::vector<VkFormat> &colorFormats,
										   VkFormat depthFormat = VK_FORMAT_UNDEFINED) {
		return { .colorFormats = colorFormats, .depthFormat = depthFormat };
	}

	[[nodiscard]] VkPipelineRenderingCreateInfo GetCreateInfo() const {
		VkPipelineRenderingCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		info.colorAttachmentCount = static_cast<uint32>(colorFormats.size());
		info.pColorAttachmentFormats = colorFormats.empty() ? nullptr : colorFormats.data();
		info.depthAttachmentFormat = depthFormat;
		return info;
	}
};

} // namespace Aquila::Graphics::Helpers

#endif // AQUILA_DYNAMIC_RENDERING_HELPER_H

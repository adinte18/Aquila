#ifndef AQUILA_VULKAN_DYNAMIC_RENDERING_H
#define AQUILA_VULKAN_DYNAMIC_RENDERING_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Color.h"
#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/RHI/Vulkan/VulkanCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanFormatUtils.h"

namespace Aquila::RHI {

class VulkanDynamicRendering {
  public:
	struct ColorAttachmentDesc {
		VkImageView view = VK_NULL_HANDLE;
		VkImageView resolveView = VK_NULL_HANDLE;
		VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		bool clear = true;
		vec4 clearColor = { Foundation::Color::Black_v, 1.F };
	};

	struct DepthAttachmentDesc {
		VkImageView view = VK_NULL_HANDLE;
		VkImageView resolveView = VK_NULL_HANDLE;
		VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		bool clear = true;
		float clearDepth = 1.F;
		bool readOnly = false;
	};

	struct BeginDesc {
		uint32 width = 0;
		uint32 height = 0;
		std::vector<ColorAttachmentDesc> colorAttachments;
		std::optional<DepthAttachmentDesc> depthAttachment;
	};

	static void Begin(VulkanCommandList &cmd, const BeginDesc &desc) {
		std::vector<VkRenderingAttachmentInfo> colorInfos;
		colorInfos.reserve(desc.colorAttachments.size());

		for (const auto &c : desc.colorAttachments) {
			VkRenderingAttachmentInfo info{};
			info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			info.imageView = c.view;
			info.imageLayout = c.layout;
			info.loadOp = c.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			info.clearValue.color = { { c.clearColor.r, c.clearColor.g, c.clearColor.b, c.clearColor.a } };

			if (c.resolveView != VK_NULL_HANDLE) {
				info.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
				info.resolveImageView = c.resolveView;
				info.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}

			colorInfos.push_back(info);
		}

		VkRenderingAttachmentInfo depthInfo{};
		if (desc.depthAttachment.has_value()) {
			const auto &d = *desc.depthAttachment;
			depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthInfo.imageView = d.view;
			depthInfo.imageLayout = d.readOnly ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : d.layout;
			depthInfo.loadOp = d.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			depthInfo.storeOp = d.readOnly ? VK_ATTACHMENT_STORE_OP_NONE : VK_ATTACHMENT_STORE_OP_STORE;
			depthInfo.clearValue.depthStencil = { .depth = d.clearDepth, .stencil = 0 };
		}

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.extent = { .width = desc.width, .height = desc.height };
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = static_cast<uint32>(colorInfos.size());
		renderingInfo.pColorAttachments = colorInfos.empty() ? nullptr : colorInfos.data();
		renderingInfo.pDepthAttachment = desc.depthAttachment.has_value() ? &depthInfo : nullptr;

		vkCmdBeginRendering(cmd.GetHandle(), &renderingInfo);

		VkViewport viewport{};
		viewport.width = static_cast<float>(desc.width);
		viewport.height = static_cast<float>(desc.height);
		viewport.maxDepth = 1.F;
		vkCmdSetViewport(cmd.GetHandle(), 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.extent = { .width = desc.width, .height = desc.height };
		vkCmdSetScissor(cmd.GetHandle(), 0, 1, &scissor);
	}

	static void End(VulkanCommandList &cmd) { vkCmdEndRendering(cmd.GetHandle()); }
};

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

} // namespace Aquila::RHI
#endif

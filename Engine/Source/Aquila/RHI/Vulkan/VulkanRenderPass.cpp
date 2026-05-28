#include "Aquila/RHI/Vulkan/VulkanRenderPass.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"
#include "Aquila/RHI/Vulkan/VulkanCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanSwapchain.h"
#include "Aquila/RHI/Vulkan/VulkanTexture.h"
#include "Aquila/RHI/Vulkan/VulkanDynamicRendering.h"
#include "Aquila/RHI/Vulkan/VulkanFormatUtils.h"
#include "Aquila/Foundation/Macros.h"

namespace Aquila::RHI {

static bool HasStencil(TextureFormat fmt) {
	return fmt == TextureFormat::Depth24Stencil8 || fmt == TextureFormat::Depth32Stencil8;
}

static VkImageAspectFlags DepthAspect(TextureFormat fmt) {
	return HasStencil(fmt) ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_DEPTH_BIT;
}

static VkImageMemoryBarrier MakeBarrier(VkImage image, VkImageAspectFlags aspect, VkImageLayout oldLayout,
										VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess) {
	VkImageMemoryBarrier b{};
	b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	b.oldLayout = oldLayout;
	b.newLayout = newLayout;
	b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	b.image = image;
	b.subresourceRange = { aspect, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
	b.srcAccessMask = srcAccess;
	b.dstAccessMask = dstAccess;
	return b;
}

VulkanRenderPass::VulkanRenderPass(VulkanDevice &device, const RenderPassDesc &desc) : m_Device(device), m_Desc(desc) {}

void VulkanRenderPass::IssuePreBarriers(VulkanCommandList &cmd, const VulkanSwapchain *swapchain,
										uint32 imageIndex) const {
	std::vector<VkImageMemoryBarrier> barriers;
	barriers.reserve(4);

	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
									VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
									VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

	if (m_Desc.useSwapchain) {
		AQUILA_ASSERT(swapchain, "useSwapchain=true but no swapchain passed to Begin()");
		barriers.push_back(MakeBarrier(swapchain->GetImage(imageIndex), VK_IMAGE_ASPECT_COLOR_BIT,
									   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
									   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));
	} else {
		for (const auto &att : m_Desc.colorAttachments) {
			if (att.texture == nullptr) {
				continue;
			}
			if (att.resolveTexture != nullptr) {
				auto &vkResolve = static_cast<VulkanTexture &>(*att.resolveTexture);
				barriers.push_back(MakeBarrier(vkResolve.GetImage(), VK_IMAGE_ASPECT_COLOR_BIT,
											   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
											   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT));
			}

			auto &vkTex = static_cast<VulkanTexture &>(*att.texture);

			VkImageLayout oldLayout = (att.loadOp == AttachmentLoadOp::Load) ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
																			 : VK_IMAGE_LAYOUT_UNDEFINED;
			VkAccessFlags srcAccess = (att.loadOp == AttachmentLoadOp::Load) ? VK_ACCESS_SHADER_READ_BIT : 0;

			barriers.push_back(MakeBarrier(vkTex.GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, oldLayout,
										   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, srcAccess,
										   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT));
		}
	}

	if (m_Desc.depthAttachment.has_value()) {
		const auto &d = *m_Desc.depthAttachment;

		VkImage depthImage = VK_NULL_HANDLE;
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (d.texture) {
			auto &vkDepth = static_cast<VulkanTexture &>(*d.texture);
			depthImage = vkDepth.GetImage();
			aspect = DepthAspect(vkDepth.GetFormat());
		} else if (m_Desc.useSwapchain && swapchain) {
			depthImage = swapchain->GetDepthImage(imageIndex);
		}

		if (depthImage != VK_NULL_HANDLE) {
			VkImageLayout oldLayout = (d.depthLoadOp == AttachmentLoadOp::Load)
										  ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
										  : VK_IMAGE_LAYOUT_UNDEFINED;
			VkAccessFlags srcAccess = (d.depthLoadOp == AttachmentLoadOp::Load) ? VK_ACCESS_SHADER_READ_BIT : 0;
			VkImageLayout newLayout =
				d.readOnly ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			VkAccessFlags dstAccess =
				d.readOnly
					? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
					: (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

			barriers.push_back(MakeBarrier(depthImage, aspect, oldLayout, newLayout, srcAccess, dstAccess));
		}
	}

	if (!barriers.empty()) {
		vkCmdPipelineBarrier(cmd.GetHandle(), srcStage, dstStage, 0, 0, nullptr, 0, nullptr,
							 static_cast<uint32>(barriers.size()), barriers.data());
	}
}

void VulkanRenderPass::IssuePostBarriers(VulkanCommandList &cmd) const {
	std::vector<VkImageMemoryBarrier> barriers;
	barriers.reserve(4);

	VkPipelineStageFlags srcStage =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	if (m_Desc.useSwapchain) {
		AQUILA_ASSERT(m_ActiveSwapchain, "useSwapchain=true but swapchain is null during End()");
		barriers.push_back(MakeBarrier(m_ActiveSwapchain->GetImage(m_SwapchainImageIndex), VK_IMAGE_ASPECT_COLOR_BIT,
									   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
									   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0));
	} else {
		for (const auto &att : m_Desc.colorAttachments) {
			if ((att.texture == nullptr) || att.storeOp == AttachmentStoreOp::DontCare) {
				continue;
			}
			if (att.resolveTexture != nullptr) {
				auto &vkResolve = static_cast<VulkanTexture &>(*att.resolveTexture);
				barriers.push_back(MakeBarrier(vkResolve.GetImage(), VK_IMAGE_ASPECT_COLOR_BIT,
											   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
											   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
											   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT));
			}

			auto &vkTex = static_cast<VulkanTexture &>(*att.texture);

			if ((ToVkImageUsage(vkTex.GetDesc().usage) & VK_IMAGE_USAGE_SAMPLED_BIT) == 0u) {
				continue;
			}

			barriers.push_back(MakeBarrier(vkTex.GetImage(), VK_IMAGE_ASPECT_COLOR_BIT,
										   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
										   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
										   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT));
		}
	}

	if (m_Desc.depthAttachment.has_value()) {
		const auto &d = *m_Desc.depthAttachment;
		if (d.texture && d.depthStoreOp == AttachmentStoreOp::Store) {
			auto &vkDepth = static_cast<VulkanTexture &>(*d.texture);
			VkImageAspectFlags aspect = DepthAspect(vkDepth.GetFormat());
			VkImageLayout oldLayout =
				d.readOnly ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			barriers.push_back(
				MakeBarrier(vkDepth.GetImage(), aspect, oldLayout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
							d.readOnly ? 0 : VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT));
		}
	}

	if (!barriers.empty()) {
		vkCmdPipelineBarrier(cmd.GetHandle(), srcStage, dstStage, 0, 0, nullptr, 0, nullptr,
							 static_cast<uint32>(barriers.size()), barriers.data());
	}
}

void VulkanRenderPass::Begin(IRHICommandList &cmd, IRHISwapchain *swapchain, uint32 imageIndex) {
	AQUILA_ASSERT(!m_Recording, "RenderPass already recording");
	m_Recording = true;

	auto &vkCmd = static_cast<VulkanCommandList &>(cmd);
	m_ActiveSwapchain = (swapchain != nullptr) ? &static_cast<VulkanSwapchain &>(*swapchain) : nullptr;
	m_SwapchainImageIndex = imageIndex;

	if (!m_Desc.externalBarriers) {
		IssuePreBarriers(vkCmd, m_ActiveSwapchain, imageIndex);
	}

	uint32 width = m_Desc.width;
	uint32 height = m_Desc.height;

	std::vector<VulkanDynamicRendering::ColorAttachmentDesc> colorDescs;

	if (m_Desc.useSwapchain) {
		AQUILA_ASSERT(m_ActiveSwapchain, "useSwapchain=true but no swapchain passed");
		width = m_ActiveSwapchain->GetExtent().width;
		height = m_ActiveSwapchain->GetExtent().height;

		const RenderPassColorAttachmentDesc &src =
			m_Desc.colorAttachments.empty() ? RenderPassColorAttachmentDesc{} : m_Desc.colorAttachments[0];
		colorDescs.push_back({
			.view = m_ActiveSwapchain->GetImageView(imageIndex),
			.resolveView = VK_NULL_HANDLE,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.clear = (src.loadOp == AttachmentLoadOp::Clear),
			.clearColor = src.clearColor,
		});
		m_ColorFormat = TextureFormat::BGRA8;
	} else {
		for (const auto &att : m_Desc.colorAttachments) {
			AQUILA_ASSERT(att.texture, "Offscreen color attachment has null texture");
			auto &vkTex = static_cast<VulkanTexture &>(*att.texture);

			VkImageView resolveView = VK_NULL_HANDLE;
			if (att.resolveTexture != nullptr) {
				resolveView = static_cast<VulkanTexture &>(*att.resolveTexture).GetImageView();
			}

			colorDescs.push_back({
				.view = vkTex.GetImageView(),
				.resolveView = resolveView,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.clear = (att.loadOp == AttachmentLoadOp::Clear),
				.clearColor = att.clearColor,
			});
			m_ColorFormat = vkTex.GetFormat();
			m_SampleCount = vkTex.GetSampleCount();
		}
	}

	std::optional<VulkanDynamicRendering::DepthAttachmentDesc> depthDesc;
	if (m_Desc.depthAttachment.has_value()) {
		const auto &d = *m_Desc.depthAttachment;

		VkImageView depthView = VK_NULL_HANDLE;
		if (d.texture) {
			depthView = static_cast<VulkanTexture &>(*d.texture).GetImageView();
		} else if (m_Desc.useSwapchain && m_ActiveSwapchain) {
			depthView = m_ActiveSwapchain->GetDepthImageView(imageIndex);
		}

		if (depthView != VK_NULL_HANDLE) {
			depthDesc = VulkanDynamicRendering::DepthAttachmentDesc{
				.view = depthView,
				.layout = d.readOnly ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
									 : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
				.clear = (d.depthLoadOp == AttachmentLoadOp::Clear),
				.clearDepth = d.clearDepth,
				.readOnly = d.readOnly,
			};
		}
	}

	VulkanDynamicRendering::Begin(vkCmd, {
											 .width = width,
											 .height = height,
											 .colorAttachments = colorDescs,
											 .depthAttachment = depthDesc,
										 });

	m_Width = width;
	m_Height = height;

	// Default full-attachment viewport and scissor — caller can override via
	// cmd.SetViewport / cmd.SetScissor after Begin() returns.
	VkViewport viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
	vkCmdSetViewport(vkCmd.GetHandle(), 0, 1, &viewport);

	VkRect2D scissor{ { 0, 0 }, { width, height } };
	vkCmdSetScissor(vkCmd.GetHandle(), 0, 1, &scissor);
}

void VulkanRenderPass::End(IRHICommandList &cmd) {
	AQUILA_ASSERT(m_Recording, "RenderPass not recording");
	auto &vkCmd = static_cast<VulkanCommandList &>(cmd);

	VulkanDynamicRendering::End(vkCmd);
	if (!m_Desc.externalBarriers) {
		IssuePostBarriers(vkCmd);
	}

	m_Recording = false;
	m_ActiveSwapchain = nullptr;
}

} // namespace Aquila::RHI

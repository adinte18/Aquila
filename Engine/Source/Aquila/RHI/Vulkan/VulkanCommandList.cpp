#include "Aquila/RHI/Vulkan/VulkanCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanDevice.h"
#include "Aquila/RHI/Vulkan/VulkanTexture.h"
#include "Aquila/RHI/Vulkan/VulkanBuffer.h"
#include "Aquila/RHI/Vulkan/VulkanPipeline.h"
#include "Aquila/RHI/Vulkan/VulkanComputePipeline.h"
#include "Aquila/RHI/Vulkan/VulkanDescriptorSet.h"
#include "Aquila/RHI/Vulkan/VulkanFormatUtils.h"

namespace Aquila::RHI {

// ResourceState -> Vulkan barrier helpers

namespace {

struct VkTexBarrierInfo {
	VkPipelineStageFlags stage = 0;
	VkAccessFlags access = 0;
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

struct VkBufBarrierInfo {
	VkPipelineStageFlags stage = 0;
	VkAccessFlags access = 0;
};

static VkTexBarrierInfo TexBarrierInfo(ResourceState state, bool isDepth) {
	VkTexBarrierInfo out{};

	if (state == ResourceState::Undefined) {
		out.stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		out.access = 0;
		out.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		return out;
	}

	const auto s = static_cast<uint16>(state);
	const auto has = [s](ResourceState bit) { return (s & static_cast<uint16>(bit)) != 0; };

	if (has(ResourceState::ColorAttachmentWrite)) {
		out.stage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		out.access |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		out.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	if (has(ResourceState::ColorAttachmentRead)) {
		out.stage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		out.access |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		if (out.layout == VK_IMAGE_LAYOUT_UNDEFINED) {
			out.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
	}

	// Depth write takes precedence over depth read for layout selection
	if (has(ResourceState::DepthStencilWrite)) {
		out.stage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		out.access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		out.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	} else if (has(ResourceState::DepthStencilRead)) {
		out.stage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		out.access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		if (out.layout == VK_IMAGE_LAYOUT_UNDEFINED) {
			out.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		}
	}

	if (has(ResourceState::ShaderRead)) {
		out.stage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
					 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		out.access |= VK_ACCESS_SHADER_READ_BIT;
		if (out.layout == VK_IMAGE_LAYOUT_UNDEFINED) {
			out.layout =
				isDepth ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
	}

	// Storage overrides layout to GENERAL regardless of other flags
	if (has(ResourceState::StorageRead) || has(ResourceState::StorageWrite)) {
		out.stage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		if (has(ResourceState::StorageRead)) {
			out.access |= VK_ACCESS_SHADER_READ_BIT;
		}
		if (has(ResourceState::StorageWrite)) {
			out.access |= VK_ACCESS_SHADER_WRITE_BIT;
		}
		out.layout = VK_IMAGE_LAYOUT_GENERAL;
	}

	if (has(ResourceState::TransferSrc)) {
		out.stage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		out.access |= VK_ACCESS_TRANSFER_READ_BIT;
		if (out.layout == VK_IMAGE_LAYOUT_UNDEFINED) {
			out.layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		}
	}
	if (has(ResourceState::TransferDst)) {
		out.stage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		out.access |= VK_ACCESS_TRANSFER_WRITE_BIT;
		if (out.layout == VK_IMAGE_LAYOUT_UNDEFINED) {
			out.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		}
	}

	if (has(ResourceState::Present)) {
		out.stage |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		out.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	if (out.stage == 0) {
		out.stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
	return out;
}

static VkBufBarrierInfo BufBarrierInfo(ResourceState state) {
	VkBufBarrierInfo out{};

	if (state == ResourceState::Undefined) {
		out.stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		return out;
	}

	const auto s = static_cast<uint16>(state);
	const auto has = [s](ResourceState bit) { return (s & static_cast<uint16>(bit)) != 0; };

	if (has(ResourceState::UniformRead)) {
		out.stage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
					 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		out.access |= VK_ACCESS_UNIFORM_READ_BIT;
	}
	if (has(ResourceState::ShaderRead)) {
		out.stage |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
					 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		out.access |= VK_ACCESS_SHADER_READ_BIT;
	}
	if (has(ResourceState::StorageRead)) {
		out.stage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		out.access |= VK_ACCESS_SHADER_READ_BIT;
	}
	if (has(ResourceState::StorageWrite)) {
		out.stage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		out.access |= VK_ACCESS_SHADER_WRITE_BIT;
	}
	if (has(ResourceState::TransferSrc)) {
		out.stage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		out.access |= VK_ACCESS_TRANSFER_READ_BIT;
	}
	if (has(ResourceState::TransferDst)) {
		out.stage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		out.access |= VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	if (has(ResourceState::IndirectArgument)) {
		out.stage |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
		out.access |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
	}
	if (has(ResourceState::IndexBuffer)) {
		out.stage |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		out.access |= VK_ACCESS_INDEX_READ_BIT;
	}
	if (has(ResourceState::VertexBuffer)) {
		out.stage |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
		out.access |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	}

	if (out.stage == 0) {
		out.stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
	return out;
}

static VkImageAspectFlags AspectFor(TextureFormat fmt) {
	if (fmt == TextureFormat::Depth24Stencil8 || fmt == TextureFormat::Depth32Stencil8) {
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	if (IsDepthFormat(fmt)) {
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	return VK_IMAGE_ASPECT_COLOR_BIT;
}

} // anonymous namespace

// Lifecycle

VulkanCommandList::VulkanCommandList(VulkanDevice &device, VkCommandPool commandPool, CommandListType type,
									 const std::string &name)
	: m_CommandPool(commandPool), m_Type(type), m_Name(name), m_Device(device) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	AQUILA_VULKAN_CHECK(vkAllocateCommandBuffers(m_Device.GetDevice(), &allocInfo, &m_CommandBuffer));

	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64>(m_CommandBuffer), name.c_str());
}

VulkanCommandList::VulkanCommandList(VulkanDevice &device, VkCommandPool commandPool, VkCommandBuffer existingCmd,
									 CommandListType type, const std::string &name)
	: m_CommandBuffer(existingCmd), m_CommandPool(commandPool), m_Type(type), m_Name(name), m_Device(device) {
	m_Device.SetObjectDebugName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64>(m_CommandBuffer), name.c_str());
}

VulkanCommandList::~VulkanCommandList() {
	// Freed automatically when the command pool is destroyed
}

void VulkanCommandList::Begin() {
	if (m_IsRecording) {
		throw std::runtime_error("Command list '" + m_Name + "' is already recording");
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	AQUILA_VULKAN_CHECK(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));
	m_IsRecording = true;
}

void VulkanCommandList::End() {
	if (!m_IsRecording) {
		throw std::runtime_error("Command list '" + m_Name + "' is not recording");
	}

	AQUILA_VULKAN_CHECK(vkEndCommandBuffer(m_CommandBuffer));
	m_IsRecording = false;
}

void VulkanCommandList::Reset() {
	vkResetCommandBuffer(m_CommandBuffer, 0);
	m_IsRecording = false;
	m_BoundPipelineLayout = VK_NULL_HANDLE;
}

// Resource transitions

void VulkanCommandList::TransitionTexture(IRHITexture &texture, ResourceState oldState, ResourceState newState) {
	if (oldState == newState) {
		return;
	}

	auto &vkTex = static_cast<VulkanTexture &>(texture);
	const bool depth = IsDepthFormat(vkTex.GetFormat());

	VkTexBarrierInfo src = TexBarrierInfo(oldState, depth);
	VkTexBarrierInfo dst = TexBarrierInfo(newState, depth);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = src.access;
	barrier.dstAccessMask = dst.access;
	barrier.oldLayout = src.layout;
	barrier.newLayout = dst.layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkTex.GetImage();
	barrier.subresourceRange = { AspectFor(vkTex.GetFormat()), 0, VK_REMAINING_MIP_LEVELS, 0,
								 VK_REMAINING_ARRAY_LAYERS };

	vkCmdPipelineBarrier(m_CommandBuffer, src.stage, dst.stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanCommandList::TransitionBuffer(IRHIBuffer &buffer, ResourceState oldState, ResourceState newState) {
	if (oldState == newState) {
		return;
	}

	auto &vkBuf = static_cast<VulkanBuffer &>(buffer);

	VkBufBarrierInfo src = BufBarrierInfo(oldState);
	VkBufBarrierInfo dst = BufBarrierInfo(newState);

	VkBufferMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = src.access;
	barrier.dstAccessMask = dst.access;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = vkBuf.GetBuffer();
	barrier.offset = 0;
	barrier.size = VK_WHOLE_SIZE;

	vkCmdPipelineBarrier(m_CommandBuffer, src.stage, dst.stage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

// Pipeline and state

void VulkanCommandList::BindPipeline(IRHIPipeline &pipeline) {
	if (pipeline.GetBindPoint() == PipelineBindPoint::Compute) {
		auto &vkPipeline = static_cast<VulkanComputePipeline &>(pipeline);
		m_BoundPipelineLayout = vkPipeline.GetLayout();
		m_BoundBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline.GetPipeline());
	} else {
		auto &vkPipeline = static_cast<VulkanPipeline &>(pipeline);
		m_BoundPipelineLayout = vkPipeline.GetLayout();
		m_BoundBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline.GetPipeline());
	}
}

void VulkanCommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
	VkViewport vp{ x, y, width, height, minDepth, maxDepth };
	vkCmdSetViewport(m_CommandBuffer, 0, 1, &vp);
}

void VulkanCommandList::SetScissor(int32 x, int32 y, uint32 width, uint32 height) {
	VkRect2D s{ { x, y }, { width, height } };
	vkCmdSetScissor(m_CommandBuffer, 0, 1, &s);
}

// Resource binding

void VulkanCommandList::BindDescriptorSet(uint32 set, IRHIDescriptorSet &descriptorSet) {
	AQUILA_ASSERT(m_BoundPipelineLayout != VK_NULL_HANDLE, "BindDescriptorSet called before BindPipeline");
	auto &vkSet = static_cast<VulkanDescriptorSet &>(descriptorSet);
	VkDescriptorSet raw = vkSet.GetDescriptorSet();
	vkCmdBindDescriptorSets(m_CommandBuffer, m_BoundBindPoint, m_BoundPipelineLayout, set, 1, &raw, 0, nullptr);
}

void VulkanCommandList::PushConstants(const void *data, uint32 size, ShaderStageFlags stages, uint32 offset) {
	AQUILA_ASSERT(m_BoundPipelineLayout != VK_NULL_HANDLE, "PushConstants called before BindPipeline");
	vkCmdPushConstants(m_CommandBuffer, m_BoundPipelineLayout, ToVkShaderStage(stages), offset, size, data);
}

void VulkanCommandList::BindVertexBuffer(IRHIBuffer &buffer, uint32 binding, uint64 offset) {
	auto &vkBuf = static_cast<VulkanBuffer &>(buffer);
	VkBuffer raw = vkBuf.GetBuffer();
	VkDeviceSize off = static_cast<VkDeviceSize>(offset);
	vkCmdBindVertexBuffers(m_CommandBuffer, binding, 1, &raw, &off);
}

void VulkanCommandList::BindIndexBuffer(IRHIBuffer &buffer, IndexFormat format, uint64 offset) {
	auto &vkBuf = static_cast<VulkanBuffer &>(buffer);
	VkIndexType indexType = (format == IndexFormat::UInt32) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
	vkCmdBindIndexBuffer(m_CommandBuffer, vkBuf.GetBuffer(), static_cast<VkDeviceSize>(offset), indexType);
}

// Draw commands

void VulkanCommandList::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) {
	vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandList::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset,
									uint32 firstInstance) {
	vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandList::DrawIndirect(IRHIBuffer &buffer, uint64 offset, uint32 drawCount, uint32 stride) {
	auto &vkBuf = static_cast<VulkanBuffer &>(buffer);
	vkCmdDrawIndirect(m_CommandBuffer, vkBuf.GetBuffer(), static_cast<VkDeviceSize>(offset), drawCount, stride);
}

void VulkanCommandList::DrawIndexedIndirect(IRHIBuffer &buffer, uint64 offset, uint32 drawCount, uint32 stride) {
	auto &vkBuf = static_cast<VulkanBuffer &>(buffer);
	vkCmdDrawIndexedIndirect(m_CommandBuffer, vkBuf.GetBuffer(), static_cast<VkDeviceSize>(offset), drawCount, stride);
}

void VulkanCommandList::CopyBufferToTexture(IRHIBuffer &src, IRHITexture &dst, uint32 width, uint32 height,
											uint32 dstArrayLayer, uint32 dstMipLevel) {
	auto &vkBuf = static_cast<VulkanBuffer &>(src);
	auto &vkTex = static_cast<VulkanTexture &>(dst);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = dstMipLevel;
	region.imageSubresource.baseArrayLayer = dstArrayLayer;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(m_CommandBuffer, vkBuf.GetBuffer(), vkTex.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   1, &region);
}

void VulkanCommandList::FillBuffer(IRHIBuffer &buffer, uint64 offset, uint64 size, uint32 value) {
	auto &vkBuf = static_cast<VulkanBuffer &>(buffer);
	vkCmdFillBuffer(m_CommandBuffer, vkBuf.GetBuffer(), static_cast<VkDeviceSize>(offset),
					static_cast<VkDeviceSize>(size), value);
}

void VulkanCommandList::Dispatch(uint32 x, uint32 y, uint32 z) {
	vkCmdDispatch(m_CommandBuffer, x, y, z);
}

// Debug markers

void VulkanCommandList::PushDebugGroup(const char *name) {
	if (auto fn = m_Device.GetDebugBeginLabel()) {
		VkDebugUtilsLabelEXT label{};
		label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		label.pLabelName = name;
		label.color[0] = 0.2f;
		label.color[1] = 0.6f;
		label.color[2] = 1.0f;
		label.color[3] = 1.0f;
		fn(m_CommandBuffer, &label);
	}
}

void VulkanCommandList::PopDebugGroup() {
	if (auto fn = m_Device.GetDebugEndLabel()) {
		fn(m_CommandBuffer);
	}
}

} // namespace Aquila::RHI

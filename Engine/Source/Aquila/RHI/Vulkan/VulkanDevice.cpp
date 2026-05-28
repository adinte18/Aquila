#include "Aquila/RHI/Vulkan/VulkanDevice.h"
#include "Aquila/RHI/Vulkan/VulkanDeletionQueue.h"
#include "Aquila/Foundation/Profiler.h"

#include "Aquila/RHI/Vulkan/VulkanBuffer.h"
#include "Aquila/RHI/Vulkan/VulkanCommandList.h"
#include "Aquila/RHI/Vulkan/VulkanComputePipeline.h"
#include "Aquila/RHI/Vulkan/VulkanDescriptorSet.h"
#include "Aquila/RHI/Vulkan/VulkanDescriptors.h"
#include "Aquila/RHI/Vulkan/VulkanFormatUtils.h"
#include "Aquila/RHI/Vulkan/VulkanPipeline.h"
#include "Aquila/RHI/Vulkan/VulkanRenderPass.h"
#include "Aquila/RHI/Vulkan/VulkanShader.h"
#include "Aquila/RHI/Vulkan/VulkanSwapchain.h"
#include "Aquila/RHI/Vulkan/VulkanTexture.h"
#include "Aquila/RHI/Vulkan/VulkanVertex.h"

namespace Aquila::RHI {

VulkanDevice::VulkanDevice(GLFWwindow &nativeWindow) : m_WindowHandle(nativeWindow) {
	CreateInstance();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();

	InitializeVMA();

	m_DeletionQueue = CreateUnique<RHI::DeletionQueue>(*this);

	CreateGraphicsCommandPool();
	CreateComputeCommandPool();
	CreateTransferCommandPool();
	CreateFrameCommandPools();
	CreateGlobalDescriptorPool();

	SetupDebugMessenger();
	LogDeviceInfo();

	VkFenceCreateInfo offscreenFenceInfo{};
	offscreenFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	offscreenFenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		AQUILA_VULKAN_CHECK(vkCreateFence(m_Device, &offscreenFenceInfo, nullptr, &m_OffscreenFences[i]));
	}
}

VulkanDevice::~VulkanDevice() {
	Wait(); // before killing device wait for it, be gentle

	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		for (auto &p : m_OffscreenPendingCmdBufs[i]) {
			vkFreeCommandBuffers(m_Device, p.pool, 1, &p.cmd);
		}
		vkDestroyFence(m_Device, m_OffscreenFences[i], nullptr);
	}

	m_DeletionQueue.reset();

	DestroySamplerCache();
	DestroyGlobalDescriptorPool();

	vmaDestroyAllocator(m_Allocator);

	for (auto &[id, threadPool] : m_ThreadPools) {
		if (threadPool.pool != VK_NULL_HANDLE) {
			vkDestroyCommandPool(m_Device, threadPool.pool, nullptr);
		}
	}

	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroyCommandPool(m_Device, m_FrameSlots[i].pool, nullptr);
	}

	vkDestroyCommandPool(m_Device, m_GraphicsCommandPool, nullptr);
	vkDestroyCommandPool(m_Device, m_ComputeCommandPool, nullptr);
	vkDestroyCommandPool(m_Device, m_TransferCommandPool, nullptr);

	vkDestroyDevice(m_Device, nullptr);

	if (enableValidationLayers) {
		DestroyDebugMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);
	vkDestroyInstance(m_VulkanInstance, nullptr);

	AQUILA_LOG_DEBUG("VulkanDevice destroyed!");
}

Unique<IRHIBuffer> VulkanDevice::CreateBuffer(const BufferDesc &desc) {
	VkBufferUsageFlags vkUsage = ToVkBufferUsage(desc.usage);
	if (desc.domain == MemoryDomain::GPU_ONLY) {
		vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	return CreateUnique<VulkanBuffer>(*this, desc.debugName, static_cast<VkDeviceSize>(desc.size), desc.instanceCount,
									  vkUsage, desc.domain, static_cast<VkDeviceSize>(desc.minAlignment));
}

Unique<IRHITexture> VulkanDevice::CreateTexture(const TextureDesc &desc) {
	return CreateUnique<VulkanTexture>(*this, desc);
}

Unique<IRHICommandList> VulkanDevice::CreateCommandList(CommandListType type, const std::string &name) {
	VkCommandPool pool = VK_NULL_HANDLE;
	switch (type) {
	case CommandListType::Compute:
		pool = m_ComputeCommandPool;
		break;
	case CommandListType::Transfer:
		pool = m_TransferCommandPool;
		break;
	default:
		pool = m_GraphicsCommandPool;
		break;
	}
	return CreateUnique<VulkanCommandList>(*this, pool, type, name);
}

Unique<IRHISwapchain> VulkanDevice::CreateSwapchain(const SwapchainDesc &desc) {
	VkExtent2D extent{ desc.width, desc.height };
	return CreateUnique<VulkanSwapchain>(*this, extent, desc.vsync);
}

Unique<IRHIRenderPass> VulkanDevice::CreateRenderPass(const RHI::RenderPassDesc &desc) {
	return CreateUnique<VulkanRenderPass>(*this, desc);
}

void VulkanDevice::Submit(IRHICommandList &cmd) {
	auto &vkCmd = static_cast<VulkanCommandList &>(cmd);
	VkCommandBuffer cmdBuf = vkCmd.GetHandle();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;

	SubmitToGraphicsQueue(&submitInfo, VK_NULL_HANDLE);
}

void VulkanDevice::SubmitAndWait(IRHICommandList &cmd) {
	auto &vkCmd = static_cast<VulkanCommandList &>(cmd);
	VkCommandBuffer cmdBuf = vkCmd.GetHandle();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;

	VkFence fence = CreateFence(false);
	switch (cmd.GetType()) {
	case CommandListType::Compute:
		SubmitToComputeQueue(&submitInfo, fence);
		break;
	case CommandListType::Transfer:
		SubmitToTransferQueue(&submitInfo, fence);
		break;
	default:
		SubmitToGraphicsQueue(&submitInfo, fence);
		break;
	}
	WaitForFence(fence);
	DestroyFence(fence);
}

void VulkanDevice::PresentFrame(IRHISwapchain &swapchain, uint32 imageIndex, vec4 clearColor) {
	auto &vkSwapchain = static_cast<VulkanSwapchain &>(swapchain);
	uint32 lastFrame = vkSwapchain.GetCurrentFrameSlot();
	VkSemaphore imageAvailable = vkSwapchain.GetImageAvailableSemaphore(lastFrame);
	VkSemaphore renderFinished = vkSwapchain.GetRenderFinishedSemaphore(lastFrame);
	VkImage image = vkSwapchain.GetImage(imageIndex);

	VkCommandPool pool = GetOrCreateThreadLocalGraphicsPool();
	VkCommandBuffer cmd = VK_NULL_HANDLE;
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = pool;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(m_Device, &allocInfo, &cmd);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &beginInfo);

	VkImageSubresourceRange range{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VkImageMemoryBarrier toClear{};
	toClear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	toClear.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	toClear.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	toClear.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	toClear.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	toClear.image = image;
	toClear.subresourceRange = range;
	toClear.srcAccessMask = 0;
	toClear.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
						 nullptr, 1, &toClear);

	VkClearColorValue vkClear{ .float32 = { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
	vkCmdClearColorImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &vkClear, 1, &range);

	VkImageMemoryBarrier toPresent{};
	toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	toPresent.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	toPresent.image = image;
	toPresent.subresourceRange = range;
	toPresent.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	toPresent.dstAccessMask = 0;
	vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
						 nullptr, 1, &toPresent);

	vkEndCommandBuffer(cmd);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailable;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmd;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinished;

	VkFence fence = vkSwapchain.GetInFlightFence(lastFrame);
	SubmitToGraphicsQueue(&submitInfo, fence);
	vkSwapchain.DeferCmdBufFree(lastFrame, cmd, pool);

	vkSwapchain.PresentImageRaw(&imageIndex, renderFinished);
}

void VulkanDevice::SubmitFrame(IRHICommandList &cmd, IRHISwapchain *swapchain, uint32 imageIndex) {
	auto &vkCmd = static_cast<VulkanCommandList &>(cmd);
	VkCommandBuffer cmdBuf = vkCmd.GetHandle();

	{
		PROFILE_SCOPE("EndCommandBuffer");
		cmd.End();
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuf;

	if (swapchain != nullptr) {
		auto &vkSwapchain = static_cast<VulkanSwapchain &>(*swapchain);
		uint32 lastFrame = vkSwapchain.GetCurrentFrameSlot();
		VkSemaphore imageAvailable = vkSwapchain.GetImageAvailableSemaphore(lastFrame);
		VkSemaphore renderFinished = vkSwapchain.GetRenderFinishedSemaphore(lastFrame);

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailable;
		submitInfo.pWaitDstStageMask = &waitStage;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinished;

		VkFence fence = vkSwapchain.GetInFlightFence(lastFrame);
		{
			PROFILE_SCOPE("QueueSubmit");
			SubmitToGraphicsQueue(&submitInfo, fence);
		}
		bool isFrameManaged = false;
		for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
			if (vkCmd.GetPool() == m_FrameSlots[i].pool) {
				isFrameManaged = true;
				break;
			}
		}
		if (!isFrameManaged) {
			vkSwapchain.DeferCmdBufFree(lastFrame, cmdBuf, vkCmd.GetPool());
		}

		{
			PROFILE_SCOPE("QueuePresent");
			vkSwapchain.PresentImageRaw(&imageIndex, renderFinished);
		}
	} else {
		uint32 slot = m_OffscreenFrameIndex;
		vkWaitForFences(m_Device, 1, &m_OffscreenFences[slot], VK_TRUE, UINT64_MAX);
		for (auto &p : m_OffscreenPendingCmdBufs[slot]) {
			vkFreeCommandBuffers(m_Device, p.pool, 1, &p.cmd);
		}
		m_OffscreenPendingCmdBufs[slot].clear();

		m_DeletionQueue->Flush(slot);

		vkResetFences(m_Device, 1, &m_OffscreenFences[slot]);
		SubmitToGraphicsQueue(&submitInfo, m_OffscreenFences[slot]);
		m_OffscreenPendingCmdBufs[slot].push_back({ cmdBuf, vkCmd.GetPool() });

		m_OffscreenFrameIndex = (slot + 1) % SharedConstants::MAX_FRAMES_IN_FLIGHT;
		m_DeletionQueue->SetCurrentSlot(m_OffscreenFrameIndex);
	}
}

RHI::DeletionQueue &VulkanDevice::GetDeletionQueue() const {
	return *m_DeletionQueue;
}

void VulkanDevice::CopyBuffer(IRHICommandList &cmd, IRHIBuffer &src, IRHIBuffer &dst, uint64 size, uint64 srcOffset,
							  uint64 dstOffset) {
	auto &vkCmd = static_cast<VulkanCommandList &>(cmd);
	auto &vkSrc = static_cast<VulkanBuffer &>(src);
	auto &vkDst = static_cast<VulkanBuffer &>(dst);

	VkBufferCopy region{};
	region.srcOffset = srcOffset;
	region.dstOffset = dstOffset;
	region.size = size;
	vkCmdCopyBuffer(vkCmd.GetHandle(), vkSrc.GetBuffer(), vkDst.GetBuffer(), 1, &region);
}

Unique<IRHIPipeline> VulkanDevice::CreateGraphicsPipeline(const GraphicsPipelineDesc &desc) {
	std::vector<VkDescriptorSetLayout> vkLayouts;
	vkLayouts.reserve(desc.setLayouts.size());
	for (auto *layout : desc.setLayouts) {
		vkLayouts.push_back(static_cast<VulkanDescriptorSetLayout *>(layout)->GetDescriptorSetLayout());
	}

	std::vector<VkPushConstantRange> pushRanges;
	pushRanges.reserve(desc.pushConstants.size());
	for (const auto &pc : desc.pushConstants) {
		pushRanges.push_back({ ToVkShaderStage(pc.stages), pc.offset, pc.size });
	}

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = static_cast<uint32>(vkLayouts.size());
	layoutInfo.pSetLayouts = vkLayouts.data();
	layoutInfo.pushConstantRangeCount = static_cast<uint32>(pushRanges.size());
	layoutInfo.pPushConstantRanges = pushRanges.data();

	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	AQUILA_VULKAN_CHECK(vkCreatePipelineLayout(m_Device, &layoutInfo, nullptr, &pipelineLayout));

	// Build shader stages (fragment is optional so omit for depth-only pipelines)
	auto makeModule = [&](const ShaderStageDesc &s) {
		return VulkanShader::CreateShaderModule(s.spirv, *this, s.entryPoint);
	};

	VkShaderModule vertModule = makeModule(desc.vertexShader);
	VkShaderModule fragModule = desc.fragmentShader.spirv.empty() ? VK_NULL_HANDLE : makeModule(desc.fragmentShader);

	std::vector<VkPipelineShaderStageCreateInfo> stages;
	stages.push_back({ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT,
					   vertModule, desc.vertexShader.entryPoint.c_str(), nullptr });
	if (fragModule != VK_NULL_HANDLE) {
		stages.push_back({ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
						   VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, desc.fragmentShader.entryPoint.c_str(), nullptr });
	}

	VulkanPipelineConfig config{};
	VulkanPipeline::DefaultPipelineConfig(config);

	config.pipelineLayout = pipelineLayout;

	config.multisampleInfo.rasterizationSamples = ToVkSampleCount(desc.sampleCount);

	config.inputAssemblyInfo.topology = ToVkPrimitiveTopology(desc.topology);

	config.rasterizationInfo.cullMode = ToVkCullMode(desc.raster.cullMode);
	config.rasterizationInfo.polygonMode = ToVkPolygonMode(desc.raster.fillMode);
	config.rasterizationInfo.frontFace = ToVkFrontFace(desc.raster.frontFace);
	config.rasterizationInfo.depthClampEnable = desc.raster.depthClamp ? VK_TRUE : VK_FALSE;
	config.rasterizationInfo.lineWidth = desc.raster.lineWidth;

	config.depthStencilInfo.depthTestEnable = desc.depthStencil.depthTest ? VK_TRUE : VK_FALSE;
	config.depthStencilInfo.depthWriteEnable = desc.depthStencil.depthWrite ? VK_TRUE : VK_FALSE;
	config.depthStencilInfo.depthCompareOp = ToVkCompareOp(desc.depthStencil.depthCompare);
	config.depthStencilInfo.stencilTestEnable = desc.depthStencil.stencilTest ? VK_TRUE : VK_FALSE;

	config.colorBlendAttachments.clear();
	for (const auto &att : desc.blendAttachments) {
		VkPipelineColorBlendAttachmentState vkAtt{};
		vkAtt.blendEnable = att.enable ? VK_TRUE : VK_FALSE;
		vkAtt.srcColorBlendFactor = ToVkBlendFactor(att.srcColor);
		vkAtt.dstColorBlendFactor = ToVkBlendFactor(att.dstColor);
		vkAtt.colorBlendOp = ToVkBlendOp(att.colorOp);
		vkAtt.srcAlphaBlendFactor = ToVkBlendFactor(att.srcAlpha);
		vkAtt.dstAlphaBlendFactor = ToVkBlendFactor(att.dstAlpha);
		vkAtt.alphaBlendOp = ToVkBlendOp(att.alphaOp);
		vkAtt.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		config.colorBlendAttachments.push_back(vkAtt);
	}
	config.colorBlendInfo.attachmentCount = static_cast<uint32>(config.colorBlendAttachments.size());
	config.colorBlendInfo.pAttachments = config.colorBlendAttachments.data();

	for (auto fmt : desc.colorFormats) {
		config.colorFormats.push_back(ToVkFormat(fmt));
	}
	if (desc.depthFormat != TextureFormat::None) {
		config.depthFormat = ToVkFormat(desc.depthFormat);
	}

	// Depth-only pipelines have no color attachments so blend state must match.
	if (config.colorFormats.empty()) {
		config.colorBlendAttachments.clear();
	}

	if (desc.noVertexInput) {
		config.bindingDescriptions.clear();
		config.attributeDescriptions.clear();
	} else if (desc.customVertexLayout.has_value()) {
		auto &layout = desc.customVertexLayout.value();
		config.bindingDescriptions = { { 0, layout.stride, VK_VERTEX_INPUT_RATE_VERTEX } };
		config.attributeDescriptions.clear();
		for (auto &a : layout.attributes) {
			config.attributeDescriptions.push_back({ a.location, a.binding, ToVkFormat(a.format), a.offset });
		}
	} else {
		config.bindingDescriptions = Vertex::GetBindingDescriptions();
		config.attributeDescriptions = Vertex::GetAttributeDescriptions();
	}
	auto pipeline = CreateUnique<VulkanPipeline>(*this, stages, config);

	vkDestroyShaderModule(m_Device, vertModule, nullptr);
	if (fragModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(m_Device, fragModule, nullptr);
	}

	return pipeline;
}

Unique<IRHIPipeline> VulkanDevice::CreateComputePipeline(const ComputePipelineDesc &desc) {
	std::vector<VkDescriptorSetLayout> vkLayouts;
	vkLayouts.reserve(desc.setLayouts.size());
	for (auto *layout : desc.setLayouts) {
		vkLayouts.push_back(static_cast<VulkanDescriptorSetLayout *>(layout)->GetDescriptorSetLayout());
	}

	std::vector<VkPushConstantRange> pushRanges;
	pushRanges.reserve(desc.pushConstants.size());
	for (const auto &pc : desc.pushConstants) {
		pushRanges.push_back({ ToVkShaderStage(pc.stages), pc.offset, pc.size });
	}

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = static_cast<uint32>(vkLayouts.size());
	layoutInfo.pSetLayouts = vkLayouts.data();
	layoutInfo.pushConstantRangeCount = static_cast<uint32>(pushRanges.size());
	layoutInfo.pPushConstantRanges = pushRanges.data();

	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	AQUILA_VULKAN_CHECK(vkCreatePipelineLayout(m_Device, &layoutInfo, nullptr, &pipelineLayout));

	VkShaderModule compModule =
		VulkanShader::CreateShaderModule(desc.computeShader.spirv, *this, desc.computeShader.entryPoint);

	auto pipeline =
		CreateUnique<VulkanComputePipeline>(*this, compModule, desc.computeShader.entryPoint, pipelineLayout);

	// Layout is owned by VulkanComputePipeline — do not destroy here.
	vkDestroyShaderModule(m_Device, compModule, nullptr);

	return pipeline;
}

Unique<IRHIDescriptorSetLayout> VulkanDevice::CreateDescriptorSetLayout(const DescriptorSetLayoutDesc &desc) {
	VulkanDescriptorSetLayout::Builder builder(*this);
	for (const auto &b : desc.bindings) {
		builder.AddBinding(b.binding, ToVkDescriptorType(b.type), ToVkShaderStage(b.stages), b.count);
	}
	return builder.Build();
}

Unique<IRHIDescriptorSet> VulkanDevice::AllocateDescriptorSet(IRHIDescriptorSetLayout &layout) {
	auto &vkLayout = static_cast<VulkanDescriptorSetLayout &>(layout);
	VkDescriptorSet set = VK_NULL_HANDLE;
	if (!m_GlobalPool->AllocateDescriptor(vkLayout.GetDescriptorSetLayout(), set)) {
		throw std::runtime_error("Global descriptor pool exhausted");
	}
	return Unique<IRHIDescriptorSet>(new VulkanDescriptorSet(*this, set, vkLayout, *m_GlobalPool));
}

void VulkanDevice::CreateGlobalDescriptorPool() {
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },		 { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 }, { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 256 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 64 },
	};
	m_GlobalPool =
		CreateUnique<VulkanDescriptorPool>(*this, 4000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, poolSizes);
}

void VulkanDevice::DestroyGlobalDescriptorPool() {
	m_GlobalPool.reset();
}

void VulkanDevice::CreateGraphicsCommandPool() {
	const VkQueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.m_GraphicsFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	AQUILA_VULKAN_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_GraphicsCommandPool));
}

void VulkanDevice::CreateComputeCommandPool() {
	const VkQueueFamilyIndices queueFamiliesIndices = FindQueueFamilies(m_PhysicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamiliesIndices.m_ComputeFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	AQUILA_VULKAN_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_ComputeCommandPool));
}

void VulkanDevice::CreateTransferCommandPool() {
	const VkQueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = indices.m_TransferFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	AQUILA_VULKAN_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_TransferCommandPool));
}

void VulkanDevice::CreateFrameCommandPools() {
	const VkQueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = indices.m_GraphicsFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	for (uint32 i = 0; i < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++i) {
		AQUILA_VULKAN_CHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_FrameSlots[i].pool));
		allocInfo.commandPool = m_FrameSlots[i].pool;
		AQUILA_VULKAN_CHECK(vkAllocateCommandBuffers(m_Device, &allocInfo, &m_FrameSlots[i].cmd));

		std::string name = "FrameCmd_" + std::to_string(i);
		SetObjectDebugName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64>(m_FrameSlots[i].cmd), name.c_str());
	}
}

void VulkanDevice::ResetFrameCommandPool(uint32 slot) {
	AQUILA_ASSERT(slot < SharedConstants::MAX_FRAMES_IN_FLIGHT, "Frame slot out of range");
	vkResetCommandPool(m_Device, m_FrameSlots[slot].pool, 0);
}

Unique<IRHICommandList> VulkanDevice::CreateFrameCommandList(uint32 slot) {
	AQUILA_ASSERT(slot < SharedConstants::MAX_FRAMES_IN_FLIGHT, "Frame slot out of range");
	return CreateUnique<VulkanCommandList>(*this, m_FrameSlots[slot].pool, m_FrameSlots[slot].cmd,
										   CommandListType::Graphics, "FrameCmd_" + std::to_string(slot));
}

void VulkanDevice::SubmitToComputeQueue(const VkSubmitInfo *submitInfo, VkFence fence) {
	std::lock_guard<std::mutex> lock(m_ComputeQueueMutex);
	AQUILA_VULKAN_CHECK(vkQueueSubmit(m_ComputeQueue, 1, submitInfo, fence));
}

void VulkanDevice::SubmitToGraphicsQueue(const VkSubmitInfo *submitInfo, VkFence fence) {
	std::lock_guard<std::mutex> lock(m_GraphicsQueueMutex);
	AQUILA_VULKAN_CHECK(vkQueueSubmit(m_GraphicsQueue, 1, submitInfo, fence));
}

void VulkanDevice::SubmitToTransferQueue(const VkSubmitInfo *submitInfo, VkFence fence) {
	std::lock_guard<std::mutex> lock(m_TransferQueueMutex);
	AQUILA_VULKAN_CHECK(vkQueueSubmit(m_TransferQueue, 1, submitInfo, fence));
}

void VulkanDevice::WaitGraphicsQueueIdle() {
	std::lock_guard<std::mutex> lock(m_GraphicsQueueMutex);
	AQUILA_VULKAN_CHECK(vkQueueWaitIdle(m_GraphicsQueue));
}

void VulkanDevice::WaitTransferQueueIdle() {
	std::lock_guard<std::mutex> lock(m_TransferQueueMutex);
	AQUILA_VULKAN_CHECK(vkQueueWaitIdle(m_TransferQueue));
}

VkSampler VulkanDevice::GetOrCreateSampler(const SamplerDesc &desc) {
	auto it = m_SamplerCache.find(desc);
	if (it != m_SamplerCache.end()) {
		return it->second;
	}

	VkPhysicalDeviceProperties props{};
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);

	VkSamplerCreateInfo info = ToVkSamplerCreateInfo(desc, props.limits.maxSamplerAnisotropy);

	VkSampler sampler = nullptr;
	AQUILA_VULKAN_CHECK(vkCreateSampler(m_Device, &info, nullptr, &sampler));
	m_SamplerCache[desc] = sampler;
	return sampler;
}

void VulkanDevice::DestroySamplerCache() {
	for (auto &[desc, sampler] : m_SamplerCache) {
		vkDestroySampler(m_Device, sampler, nullptr);
	}
	m_SamplerCache.clear();
}

VkCommandPool VulkanDevice::GetOrCreateThreadLocalGraphicsPool() {
	auto queueFamiliesIndices = FindQueueFamilies(m_PhysicalDevice);
	auto id = std::this_thread::get_id();

	{
		std::lock_guard<std::mutex> lock(m_ThreadPoolMapMutex);
		auto it = m_ThreadPools.find(id);
		if (it != m_ThreadPools.end()) {
			return it->second.pool;
		}
	}

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamiliesIndices.m_GraphicsFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkCommandPool pool = nullptr;
	vkCreateCommandPool(m_Device, &poolInfo, nullptr, &pool);

	std::lock_guard<std::mutex> lock(m_ThreadPoolMapMutex);
	m_ThreadPools[id].pool = pool;
	return pool;
}

VkFence VulkanDevice::CreateFence(bool signaled) {
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	VkFence fence = nullptr;
	AQUILA_VULKAN_CHECK(vkCreateFence(m_Device, &fenceInfo, nullptr, &fence));
	return fence;
}

void VulkanDevice::WaitForFence(VkFence fence) {
	AQUILA_VULKAN_CHECK(vkWaitForFences(m_Device, 1, &fence, VK_TRUE, UINT64_MAX));
}

void VulkanDevice::DestroyFence(VkFence fence) {
	vkDestroyFence(m_Device, fence, nullptr);
}

VkFormat VulkanDevice::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
										   VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}

void VulkanDevice::SetObjectDebugName(VkObjectType objectType, uint64 handle, const char *name) const {
	if ((m_vkSetDebugUtilsObjectNameEXT == nullptr) || (name == nullptr)) {
		return;
	}
	VkDebugUtilsObjectNameInfoEXT nameInfo{};
	nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	nameInfo.objectType = objectType;
	nameInfo.objectHandle = handle;
	nameInfo.pObjectName = name;
	m_vkSetDebugUtilsObjectNameEXT(m_Device, &nameInfo);
}

void VulkanDevice::LogDeviceInfo() const {
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

	AQUILA_LOG_INFO("Graphics Device Information");
	AQUILA_LOG_INFO("Device Name: {}", properties.deviceName);

	std::string deviceType;
	switch (properties.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		deviceType = "Discrete GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		deviceType = "Integrated GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		deviceType = "Virtual GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		deviceType = "CPU";
		break;
	default:
		deviceType = "Unknown";
	}

	AQUILA_LOG_INFO("Device Type: " + deviceType);

	const uint32 driverVersion = properties.driverVersion;
	const uint32 apiVersion = properties.apiVersion;

	AQUILA_LOG_INFO("Driver Version: " + std::to_string(VK_VERSION_MAJOR(driverVersion)) + "." +
					std::to_string(VK_VERSION_MINOR(driverVersion)) + "." +
					std::to_string(VK_VERSION_PATCH(driverVersion)));

	AQUILA_LOG_INFO("Vulkan Version: " + std::to_string(VK_VERSION_MAJOR(apiVersion)) + "." +
					std::to_string(VK_VERSION_MINOR(apiVersion)) + "." + std::to_string(VK_VERSION_PATCH(apiVersion)));

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

	uint64 totalMemory = 0;
	for (uint32 i = 0; i < memProperties.memoryHeapCount; i++) {
		if ((memProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0U) {
			totalMemory += memProperties.memoryHeaps[i].size;
		}
	}
	AQUILA_LOG_INFO("Device Memory: {}", std::to_string(totalMemory / (1024 * 1024)) + " MB");

	VkQueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
	AQUILA_LOG_INFO("Graphics Queue Family: " + std::to_string(indices.m_GraphicsFamily.value()));
	AQUILA_LOG_INFO("Present Queue Family: " + std::to_string(indices.m_PresentFamily.value()));
	AQUILA_LOG_INFO("Compute Queue Family: " + std::to_string(indices.m_ComputeFamily.value()));
	AQUILA_LOG_INFO("Transfer Queue Family: " + std::to_string(indices.m_TransferFamily.value()));
}

void VulkanDevice::CreateInstance() {
	if (enableValidationLayers && !CheckValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Aquila Engine";
	appInfo.applicationVersion =
		VK_MAKE_API_VERSION(0, AQUILA_VERSION_MAJOR, AQUILA_VERSION_MINOR, AQUILA_VERSION_PATCH);
	appInfo.pEngineName = "Aquila";
	appInfo.engineVersion = VK_MAKE_API_VERSION(0, AQUILA_VERSION_MAJOR, AQUILA_VERSION_MINOR, AQUILA_VERSION_PATCH);
	appInfo.apiVersion = VK_API_VERSION_1_4;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	const auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = &debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	AQUILA_VULKAN_CHECK(vkCreateInstance(&createInfo, nullptr, &m_VulkanInstance));
}

void VulkanDevice::CreateSurface() {
	if (glfwCreateWindowSurface(m_VulkanInstance, &m_WindowHandle, nullptr, &m_Surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void VulkanDevice::InitializeVMA() {
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_4;
	allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
	allocatorCreateInfo.device = m_Device;
	allocatorCreateInfo.instance = m_VulkanInstance;
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

	vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator);
}

bool VulkanDevice::CheckValidationLayerSupport() const {
	uint32 layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char *layerName : validationLayers) {
		bool layerFound = false;
		for (const auto &layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			return false;
		}
	}
	return true;
}

std::vector<const char *> VulkanDevice::GetRequiredExtensions() const {
	uint32 glfwExtensionCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

void VulkanDevice::CreateLogicalDevice() {
	VkQueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32> uniqueQueueFamilies = { indices.m_GraphicsFamily.value(), indices.m_PresentFamily.value(),
											 indices.m_ComputeFamily.value(), indices.m_TransferFamily.value() };

	std::vector<f32> queuePriorities = { 1.0F, 0.5F };

	for (uint32 queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;

		if (queueFamily == indices.m_GraphicsFamily.value()) {
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());
			uint32 availableQueues = queueFamilies[queueFamily].queueCount;
			queueCreateInfo.queueCount = std::min(2U, availableQueues);
			queueCreateInfo.pQueuePriorities = queuePriorities.data();
		} else {
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = queuePriorities.data();
		}

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
	dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.wideLines = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.independentBlend = VK_TRUE;

	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.pNext = &dynamicRenderingFeatures;
	deviceFeatures2.features = deviceFeatures;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pNext = &deviceFeatures2;
	createInfo.pEnabledFeatures = nullptr;
	createInfo.enabledExtensionCount = static_cast<uint32>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	bool computeSharesGraphics = indices.m_ComputeFamily.value() == indices.m_GraphicsFamily.value();

	vkGetDeviceQueue(m_Device, indices.m_GraphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Device, indices.m_PresentFamily.value(), 0, &m_PresentQueue);
	vkGetDeviceQueue(m_Device, indices.m_TransferFamily.value(), 0, &m_TransferQueue);

	if (computeSharesGraphics) {
		uint32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

		uint32 availableQueues = queueFamilies[indices.m_GraphicsFamily.value()].queueCount;
		uint32 computeQueueIndex = (availableQueues > 1) ? 1 : 0;
		vkGetDeviceQueue(m_Device, indices.m_ComputeFamily.value(), computeQueueIndex, &m_ComputeQueue);
	} else {
		vkGetDeviceQueue(m_Device, indices.m_ComputeFamily.value(), 0, &m_ComputeQueue);
	}
}

void VulkanDevice::PickPhysicalDevice() {
	uint32 deviceCount = 0;
	vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		AQUILA_LOG_CRITICAL("Failed to find GPUs with Vulkan support!");
		abort();
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_VulkanInstance, &deviceCount, devices.data());

	for (const auto &device : devices) {
		if (IsSuitable(device)) {
			m_PhysicalDevice = device;
			break;
		}
	}

	if (m_PhysicalDevice == VK_NULL_HANDLE) {
		AQUILA_LOG_CRITICAL("Failed to find a suitable GPU!");
		abort();
	}
}

bool VulkanDevice::IsSuitable(const VkPhysicalDevice vkPhysicalDevice) {
	VkQueueFamilyIndices indices = FindQueueFamilies(vkPhysicalDevice);
	const bool extensionSupported = CheckDeviceExtensionSupport(vkPhysicalDevice);

	bool swapChainAdequate = false;
	if (extensionSupported) {
		const VkSwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(vkPhysicalDevice);
		swapChainAdequate = !swapChainSupport.m_Formats.empty() && !swapChainSupport.m_PresentModes.empty();
	}

	return indices.IsComplete() && extensionSupported && swapChainAdequate;
}

bool VulkanDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice vkPhysicalDevice) const {
	uint32 extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto &extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

VkQueueFamilyIndices VulkanDevice::FindQueueFamilies(const VkPhysicalDevice vkPhysicalDevice) const {
	VkQueueFamilyIndices indices;

	uint32 queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamilyCount, queueFamilies.data());

	int index = 0;
	for (const auto &queueFamily : queueFamilies) {
		if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
			indices.m_GraphicsFamily = index;
		}

		if (((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0U) &&
			((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0U) &&
			((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0U)) {
			indices.m_TransferFamily = index;
		}

		if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0U) {
			bool isDedicated = (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0U;
			VkBool32 presentSupport = 0U;
			vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, index, m_Surface, &presentSupport);
			bool isPresent = presentSupport != 0U;

			if (!isPresent) {
				if (isDedicated || !indices.m_ComputeFamily.has_value()) {
					indices.m_ComputeFamily = index;
				}
			}
		}

		VkBool32 presentSupport = 0U;
		vkGetPhysicalDeviceSurfaceSupportKHR(vkPhysicalDevice, index, m_Surface, &presentSupport);
		if (presentSupport != 0U) {
			indices.m_PresentFamily = index;
		}

		if (indices.IsComplete()) {
			break;
		}

		index++;
	}

	if (!indices.m_ComputeFamily.has_value() && indices.m_GraphicsFamily.has_value()) {
		indices.m_ComputeFamily = indices.m_GraphicsFamily;
	}
	if (!indices.m_TransferFamily.has_value()) {
		indices.m_TransferFamily = indices.m_ComputeFamily;
	}

	return indices;
}

VkSwapChainSupportDetails VulkanDevice::QuerySwapChainSupport(VkPhysicalDevice vkPhysicalDevice) const {
	VkSwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, m_Surface, &details.m_SurfaceCapabilities);

	uint32 formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, m_Surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.m_Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, m_Surface, &formatCount, details.m_Formats.data());
	}

	uint32 presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, m_Surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.m_PresentModes.resize(presentModeCount);
		AQUILA_VULKAN_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, m_Surface, &presentModeCount,
																	  details.m_PresentModes.data()));
	}

	return details;
}

void VulkanDevice::SetupDebugMessenger() {
	if (!enableValidationLayers) {
		return;
	}
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);
	AQUILA_VULKAN_CHECK(CreateDebugMessengerEXT(m_VulkanInstance, &createInfo, nullptr, &m_DebugMessenger));

	m_vkSetDebugUtilsObjectNameEXT =
		(PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(m_Device, "vkSetDebugUtilsObjectNameEXT");
	m_vkCmdBeginDebugUtilsLabelEXT =
		(PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(m_Device, "vkCmdBeginDebugUtilsLabelEXT");
	m_vkCmdEndDebugUtilsLabelEXT =
		(PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(m_Device, "vkCmdEndDebugUtilsLabelEXT");
}

VkResult VulkanDevice::CreateDebugMessengerEXT(const VkInstance instance,
											   const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
											   const VkAllocationCallbacks *pAllocator,
											   VkDebugUtilsMessengerEXT *pDebugMessenger) {
	if (const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
		func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VulkanDevice::DestroyDebugMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger,
											const VkAllocationCallbacks *pAllocator) {
	const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void VulkanDevice::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDevice::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
														   VkDebugUtilsMessageTypeFlagsEXT messageType,
														   const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
														   void *pUserData) {
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		AQUILA_LOG_ERROR("Vulkan: {}", pCallbackData->pMessage);
	} else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		AQUILA_LOG_WARNING("Vulkan: {}", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

} // namespace Aquila::RHI

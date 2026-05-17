#ifndef AQUILA_VULKAN_COMMAND_LIST_H
#define AQUILA_VULKAN_COMMAND_LIST_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHICommandList.h"

namespace Aquila::RHI {

class VulkanDevice;

class VulkanCommandList final : public IRHICommandList {
  public:
	VulkanCommandList(VulkanDevice &device, VkCommandPool commandPool, CommandListType type, const std::string &name);
	~VulkanCommandList() override;

	AQUILA_NONCOPYABLE(VulkanCommandList);
	AQUILA_NONMOVEABLE(VulkanCommandList);

	// IRHICommandList — lifecycle
	void Begin() override;
	void Reset() override;
	void End() override;

	[[nodiscard]] bool IsRecording() const override { return m_IsRecording; }
	[[nodiscard]] CommandListType GetType() const override { return m_Type; }
	[[nodiscard]] const std::string &GetName() const override { return m_Name; }

	// IRHICommandList
	void TransitionTexture(IRHITexture &texture, ResourceState oldState, ResourceState newState) override;
	void TransitionBuffer(IRHIBuffer &buffer, ResourceState oldState, ResourceState newState) override;

	// IRHICommandList
	void BindPipeline(IRHIPipeline &pipeline) override;
	void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) override;
	void SetScissor(int32 x, int32 y, uint32 width, uint32 height) override;

	// IRHICommandList
	void BindDescriptorSet(uint32 set, IRHIDescriptorSet &descriptorSet) override;
	void PushConstants(const void *data, uint32 size, ShaderStageFlags stages, uint32 offset) override;
	void BindVertexBuffer(IRHIBuffer &buffer, uint32 binding, uint64 offset) override;
	void BindIndexBuffer(IRHIBuffer &buffer, IndexFormat format, uint64 offset) override;

	// IRHICommandList
	void Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) override;
	void DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset,
					 uint32 firstInstance) override;
	void DrawIndirect(IRHIBuffer &buffer, uint64 offset, uint32 drawCount, uint32 stride) override;
	void DrawIndexedIndirect(IRHIBuffer &buffer, uint64 offset, uint32 drawCount, uint32 stride) override;

	void CopyBufferToTexture(IRHIBuffer &src, IRHITexture &dst, uint32 width, uint32 height,
							 uint32 dstArrayLayer = 0, uint32 dstMipLevel = 0) override;

	void FillBuffer(IRHIBuffer &buffer, uint64 offset, uint64 size, uint32 value) override;

	void Dispatch(uint32 x, uint32 y, uint32 z) override;

	// IRHICommandList
	void PushDebugGroup(const char *name) override;
	void PopDebugGroup() override;

	// Vulkan-specific accessors for internal use (RenderPass, Device, etc.)
	[[nodiscard]] VkCommandBuffer GetHandle() const { return m_CommandBuffer; }
	[[nodiscard]] VkCommandPool GetPool() const { return m_CommandPool; }

  private:
	VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
	VkCommandPool m_CommandPool = VK_NULL_HANDLE;
	CommandListType m_Type;
	std::string m_Name;
	bool m_IsRecording = false;
	VulkanDevice &m_Device;

	// Captured by BindPipeline; required for BindDescriptorSet, PushConstants, and Dispatch.
	VkPipelineLayout m_BoundPipelineLayout = VK_NULL_HANDLE;
	VkPipelineBindPoint m_BoundBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

} // namespace Aquila::RHI
#endif

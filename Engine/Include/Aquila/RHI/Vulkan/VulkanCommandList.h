#ifndef AQUILA_VULKAN_COMMAND_LIST_H
#define AQUILA_VULKAN_COMMAND_LIST_H

#include "GraphicsPCH.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHICommandList.h"

namespace Aquila::RHI {

class VulkanDevice;

class VulkanCommandList final : public IRHICommandList {
  public:
	VulkanCommandList(VulkanDevice &device, VkCommandPool command_pool, CommandListType type, const std::string &name);
	VulkanCommandList(VulkanDevice &device, VkCommandPool command_pool, VkCommandBuffer existing_cmd,
					  CommandListType type, const std::string &name);
	~VulkanCommandList() override;

	AQUILA_NONCOPYABLE(VulkanCommandList);
	AQUILA_NONMOVEABLE(VulkanCommandList);

	// IRHICommandList — lifecycle
	void begin() override;
	void reset() override;
	void end() override;

	[[nodiscard]] bool is_recording() const override { return m_is_recording; }
	[[nodiscard]] CommandListType get_type() const override { return m_type; }
	[[nodiscard]] const std::string &get_name() const override { return m_name; }

	// IRHICommandList
	void transition_texture(IRHITexture &texture, ResourceState old_state, ResourceState new_state) override;
	void transition_buffer(IRHIBuffer &buffer, ResourceState old_state, ResourceState new_state) override;

	// IRHICommandList
	void bind_pipeline(IRHIPipeline &pipeline) override;
	void set_viewport(float x, float y, float width, float height, float min_depth, float max_depth) override;
	void set_scissor(Int32 x, Int32 y, Uint32 width, Uint32 height) override;

	// IRHICommandList
	void bind_descriptor_set(Uint32 set, IRHIDescriptorSet &descriptor_set) override;
	void push_constants(const void *data, Uint32 size, ShaderStageFlags stages, Uint32 offset) override;
	void bind_vertex_buffer(IRHIBuffer &buffer, Uint32 binding, Uint64 offset) override;
	void bind_index_buffer(IRHIBuffer &buffer, IndexFormat format, Uint64 offset) override;

	// IRHICommandList
	void draw(Uint32 vertex_count, Uint32 instance_count, Uint32 first_vertex, Uint32 first_instance) override;
	void draw_indexed(Uint32 index_count, Uint32 instance_count, Uint32 first_index, Int32 vertex_offset,
					 Uint32 first_instance) override;
	void draw_indirect(IRHIBuffer &buffer, Uint64 offset, Uint32 draw_count, Uint32 stride) override;
	void draw_indexed_indirect(IRHIBuffer &buffer, Uint64 offset, Uint32 draw_count, Uint32 stride) override;

	void copy_buffer_to_texture(IRHIBuffer &src, IRHITexture &dst, Uint32 width, Uint32 height, Uint32 dst_array_layer = 0,
							 Uint32 dst_mip_level = 0) override;

	void fill_buffer(IRHIBuffer &buffer, Uint64 offset, Uint64 size, Uint32 value) override;

	void dispatch(Uint32 x, Uint32 y, Uint32 z) override;

	// IRHICommandList
	void push_debug_group(const char *name) override;
	void pop_debug_group() override;

	// Vulkan-specific accessors for internal use (RenderPass, Device, etc.)
	[[nodiscard]] VkCommandBuffer get_handle() const { return m_command_buffer; }
	[[nodiscard]] VkCommandPool get_pool() const { return m_command_pool; }

  private:
	VkCommandBuffer m_command_buffer = VK_NULL_HANDLE;
	VkCommandPool m_command_pool = VK_NULL_HANDLE;
	CommandListType m_type;
	std::string m_name;
	bool m_is_recording = false;
	VulkanDevice &m_device;

	// Captured by BindPipeline; required for BindDescriptorSet, PushConstants, and Dispatch.
	VkPipelineLayout m_bound_pipeline_layout = VK_NULL_HANDLE;
	VkPipelineBindPoint m_bound_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

} // namespace Aquila::RHI
#endif

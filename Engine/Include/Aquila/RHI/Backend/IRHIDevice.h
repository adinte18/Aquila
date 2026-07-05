#ifndef AQUILA_IRHI_DEVICE_H
#define AQUILA_IRHI_DEVICE_H

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHIBuffer.h"
#include "Aquila/RHI/Backend/IRHICommandList.h"
#include "Aquila/RHI/Backend/IRHIDescriptors.h"
#include "Aquila/RHI/Backend/IRHIRenderpass.h"
#include "Aquila/RHI/Backend/IRHIPipeline.h"
#include "Aquila/RHI/Backend/IRHISwapchain.h"
#include "Aquila/RHI/Backend/IRHITexture.h"
#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::RHI {

class IRHIDevice {
  public:
	virtual ~IRHIDevice() = default;

	IRHIDevice(const IRHIDevice &) = delete;
	IRHIDevice &operator=(const IRHIDevice &) = delete;
	IRHIDevice(IRHIDevice &&) = delete;
	IRHIDevice &operator=(IRHIDevice &&) = delete;

	[[nodiscard]] virtual Unique<IRHIBuffer> create_buffer(const BufferDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHITexture> create_texture(const TextureDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHICommandList> create_command_list(CommandListType type,
																	const std::string &name = "") = 0;
	[[nodiscard]] virtual Unique<IRHICommandList> create_frame_command_list(Uint32 slot) = 0;
	[[nodiscard]] virtual Unique<IRHISwapchain> create_swapchain(const SwapchainDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHIRenderPass> create_render_pass(const RHI::RenderPassDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHIPipeline> create_graphics_pipeline(const GraphicsPipelineDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHIPipeline> create_compute_pipeline(const ComputePipelineDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHIDescriptorSetLayout>
	create_descriptor_set_layout(const DescriptorSetLayoutDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHIDescriptorSet> allocate_descriptor_set(IRHIDescriptorSetLayout &layout) = 0;
	virtual void copy_buffer(IRHICommandList &cmd, IRHIBuffer &src, IRHIBuffer &dst, Uint64 size, Uint64 src_offset = 0,
							Uint64 dst_offset = 0) = 0;
	virtual void submit(IRHICommandList &cmd) = 0;
	virtual void submit_frame(IRHICommandList &cmd, IRHISwapchain *swapchain = nullptr, Uint32 image_index = 0) = 0;
	virtual void submit_and_wait(IRHICommandList &cmd) = 0;
	virtual void present_frame(IRHISwapchain &swapchain, Uint32 image_index,
							  Vec4 clear_color = { 0.0f, 0.0f, 0.0f, 1.0f }) = 0;
	virtual void wait_idle() = 0;

	template <typename Func> void execute_immediate(CommandListType type, Func &&func) {
		auto cmd = create_command_list(type, "ImmediateCmd");
		cmd->begin();
		func(*cmd);
		cmd->end();
		submit_and_wait(*cmd);
	}

  protected:
	IRHIDevice() = default;
};

} // namespace Aquila::RHI
#endif

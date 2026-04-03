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

	[[nodiscard]] virtual Unique<IRHIBuffer> CreateBuffer(const BufferDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHITexture> CreateTexture(const TextureDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHICommandList> CreateCommandList(CommandListType type,
																	const std::string &name = "") = 0;
	[[nodiscard]] virtual Unique<IRHISwapchain> CreateSwapchain(const SwapchainDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHIRenderPass> CreateRenderPass(const RHI::RenderPassDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHIPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHIPipeline> CreateComputePipeline(const ComputePipelineDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHIDescriptorSetLayout>
	CreateDescriptorSetLayout(const DescriptorSetLayoutDesc &desc) = 0;
	[[nodiscard]] virtual Unique<IRHIDescriptorSet> AllocateDescriptorSet(IRHIDescriptorSetLayout &layout) = 0;
	virtual void CopyBuffer(IRHICommandList &cmd, IRHIBuffer &src, IRHIBuffer &dst, uint64 size, uint64 srcOffset = 0,
							uint64 dstOffset = 0) = 0;
	virtual void Submit(IRHICommandList &cmd) = 0;
	virtual void SubmitFrame(IRHICommandList &cmd, IRHISwapchain *swapchain = nullptr, uint32 imageIndex = 0) = 0;
	virtual void SubmitAndWait(IRHICommandList &cmd) = 0;
	virtual void PresentFrame(IRHISwapchain &swapchain, uint32 imageIndex,
							  vec4 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f }) = 0;
	virtual void WaitIdle() = 0;

	template <typename Func> void ExecuteImmediate(CommandListType type, Func &&func) {
		auto cmd = CreateCommandList(type, "ImmediateCmd");
		cmd->Begin();
		func(*cmd);
		cmd->End();
		SubmitAndWait(*cmd);
	}

  protected:
	IRHIDevice() = default;
};

} // namespace Aquila::RHI
#endif

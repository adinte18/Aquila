#pragma once
#include "Aquila/RHI/Backend/IRHIDevice.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/GFX/GfxSwapchain.h"
#include "Aquila/GFX/GfxPipeline.h"
#include "Aquila/GFX/GfxRenderpass.h"
#include "Aquila/GFX/GfxDescriptorSet.h"

struct GLFWwindow;

namespace Aquila::GFX {

class GfxContext {
  public:
	static Unique<GfxContext> Create(GLFWwindow &window);
	~GfxContext();
	AQUILA_NONCOPYABLE(GfxContext);
	AQUILA_NONMOVEABLE(GfxContext);

	[[nodiscard]] Ref<GfxBuffer> CreateBuffer(const RHI::BufferDesc &desc);
	[[nodiscard]] Ref<GfxTexture> CreateTexture(const RHI::TextureDesc &desc);
	[[nodiscard]] Ref<GfxSwapchain> CreateSwapchain(const RHI::SwapchainDesc &desc);
	[[nodiscard]] Ref<GfxPipeline> CreateGraphicsPipeline(const RHI::GraphicsPipelineDesc &desc);
	[[nodiscard]] Ref<GfxPipeline> CreateComputePipeline(const RHI::ComputePipelineDesc &desc);
	[[nodiscard]] Ref<GfxDescriptorSetLayout> CreateDescriptorSetLayout(const RHI::DescriptorSetLayoutDesc &desc);
	[[nodiscard]] Ref<GfxDescriptorSet> AllocateDescriptorSet(GfxDescriptorSetLayout &layout);
	[[nodiscard]] Ref<GfxRenderPass> CreateRenderPass(const RHI::RenderPassDesc &desc);

	[[nodiscard]] Ref<GfxCommandList> CreateCommandList(RHI::CommandListType type, const std::string &name = {});

	void CopyBuffer(GfxBuffer &src, GfxBuffer &dst, uint64 size, uint64 srcOffset = 0, uint64 dstOffset = 0);

	// Records end-of-frame barriers, submits, and presents.
	// All Vulkan sync (semaphores, fences) is handled inside IRHIDevice.
	void SubmitFrame(GfxCommandList &cmd, GfxSwapchain *swapchain = nullptr, uint32 imageIndex = 0);
	void SubmitAndWait(GfxCommandList &cmd);

	template <typename Func> void ExecuteImmediate(RHI::CommandListType type, Func &&func) {
		auto cmd = CreateCommandList(type, "ImmediateCmd");
		cmd->Begin();
		func(*cmd);
		cmd->End();
		m_Device->SubmitAndWait(cmd->GetRHI());
	}
	void WaitIdle();
	void ProcessPendingDeletions();

	[[nodiscard]] RHI::IRHIDevice &GetDevice() { return *m_Device; }

  private:
	explicit GfxContext(Unique<RHI::IRHIDevice> device);
	Unique<RHI::IRHIDevice> m_Device;
};

} // namespace Aquila::GFX

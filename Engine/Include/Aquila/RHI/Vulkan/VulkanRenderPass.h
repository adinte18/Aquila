#pragma once
#include "GraphicsPCH.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHIRenderpass.h"
#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::RHI {

class VulkanDevice;
class VulkanCommandList;
class VulkanSwapchain;

class VulkanRenderPass final : public IRHIRenderPass {
  public:
	VulkanRenderPass(VulkanDevice &device, const RenderPassDesc &desc);
	~VulkanRenderPass() override = default;

	AQUILA_NONCOPYABLE(VulkanRenderPass);
	AQUILA_NONMOVEABLE(VulkanRenderPass);

	// IRHIRenderPass — begin/end dynamic rendering + attachment barriers
	void Begin(IRHICommandList &cmd, IRHISwapchain *swapchain, uint32 imageIndex) override;
	void End(IRHICommandList &cmd) override;

	// IRHIRenderPass — format queries for pipeline key derivation
	[[nodiscard]] uint32 GetWidth() const override { return m_Width; }
	[[nodiscard]] uint32 GetHeight() const override { return m_Height; }
	[[nodiscard]] RHI::TextureFormat GetColorFormat() const override { return m_ColorFormat; }
	[[nodiscard]] RHI::SampleCount GetSampleCount() const override { return m_SampleCount; }

  private:
	void IssuePreBarriers(VulkanCommandList &cmd, const VulkanSwapchain *swapchain, uint32 imageIndex) const;
	void IssuePostBarriers(VulkanCommandList &cmd) const;

	VulkanDevice &m_Device;
	RenderPassDesc m_Desc;

	// Per-frame recording state — swapchain reference held between Begin/End for
	// post-barrier emission when useSwapchain=true and externalBarriers=false.
	const VulkanSwapchain *m_ActiveSwapchain = nullptr;
	uint32 m_SwapchainImageIndex = 0;
	bool m_Recording = false;
	uint32 m_Width = 0;
	uint32 m_Height = 0;
	TextureFormat m_ColorFormat = TextureFormat::None;
	SampleCount m_SampleCount = SampleCount::x1;
};

} // namespace Aquila::RHI

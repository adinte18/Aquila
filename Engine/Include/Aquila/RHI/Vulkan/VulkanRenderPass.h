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
	void begin(IRHICommandList &cmd, IRHISwapchain *swapchain, Uint32 image_index) override;
	void end(IRHICommandList &cmd) override;

	// IRHIRenderPass — format queries for pipeline key derivation
	[[nodiscard]] Uint32 get_width() const override { return m_width; }
	[[nodiscard]] Uint32 get_height() const override { return m_height; }
	[[nodiscard]] RHI::TextureFormat get_color_format() const override { return m_color_format; }
	[[nodiscard]] RHI::SampleCount get_sample_count() const override { return m_sample_count; }

  private:
	void issue_pre_barriers(VulkanCommandList &cmd, const VulkanSwapchain *swapchain, Uint32 image_index) const;
	void issue_post_barriers(VulkanCommandList &cmd) const;

	VulkanDevice &m_device;
	RenderPassDesc m_desc;

	// Per-frame recording state — swapchain reference held between Begin/End for
	// post-barrier emission when useSwapchain=true and externalBarriers=false.
	const VulkanSwapchain *m_active_swapchain = nullptr;
	Uint32 m_swapchain_image_index = 0;
	bool m_recording = false;
	Uint32 m_width = 0;
	Uint32 m_height = 0;
	TextureFormat m_color_format = TextureFormat::None;
	SampleCount m_sample_count = SampleCount::X1;
};

} // namespace Aquila::RHI

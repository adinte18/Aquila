#include "Aquila/GFX/GfxContext.h"
#include "Aquila/RHI/RHIBackend.h"

namespace Aquila::GFX {

GfxContext::GfxContext(Unique<RHI::IRHIDevice> device) : m_Device(std::move(device)) {}
GfxContext::~GfxContext() = default;

Unique<GfxContext> GfxContext::Create(GLFWwindow &window) {
	// NOTE : THEORETICALLY THIS IS WHERE WE WOULD SPLIT BY RHIBACKENDTYPE
	// TODO: when time comes -> define a RHIBackend.h file and set the Enums
	// for now we'll select API by hand, but in the future it would be nice to have an engine config
	//
	// in the RHIBackend.h we would have something like htis  :
	// Unique<IRHIDevice> CreateDevice(RHIBackendType backend, GLFWwindow &window) {
	//     switch (backend) {
	//      case RHIBackendType::Vulkan: return CreateVulkanBackend(window);
	//      case RHIBackendType::DirectX12: return CreateDX12Backend(window);
	//      case RHIBackendType::Metal: return CreateMetalBackend(window);
	//     }
	// }
	//
	//
	// and this should become :
	// Unique<GfxContext> GfxContext::Create(GLFWwindow &window, RHI::RHIBackendType backend) {
	//      return Unique<GfxContext>(new GfxContext(RHI::CreateDevice(backend, window)));
	// }
	return Unique<GfxContext>(new GfxContext(RHI::CreateVulkanBackend(window)));
}

Ref<GfxBuffer> GfxContext::CreateBuffer(const RHI::BufferDesc &desc) {
	return Ref<GfxBuffer>(new GfxBuffer(m_Device->CreateBuffer(desc)));
}
Ref<GfxTexture> GfxContext::CreateTexture(const RHI::TextureDesc &desc) {
	return Ref<GfxTexture>(new GfxTexture(m_Device->CreateTexture(desc)));
}
Ref<GfxSwapchain> GfxContext::CreateSwapchain(const RHI::SwapchainDesc &desc) {
	return Ref<GfxSwapchain>(new GfxSwapchain(m_Device->CreateSwapchain(desc)));
}
Ref<GfxPipeline> GfxContext::CreateGraphicsPipeline(const RHI::GraphicsPipelineDesc &desc) {
	return Ref<GfxPipeline>(new GfxPipeline(m_Device->CreateGraphicsPipeline(desc)));
}
Ref<GfxPipeline> GfxContext::CreateComputePipeline(const RHI::ComputePipelineDesc &desc) {
	return Ref<GfxPipeline>(new GfxPipeline(m_Device->CreateComputePipeline(desc)));
}
Ref<GfxDescriptorSetLayout> GfxContext::CreateDescriptorSetLayout(const RHI::DescriptorSetLayoutDesc &desc) {
	return Ref<GfxDescriptorSetLayout>(new GfxDescriptorSetLayout(m_Device->CreateDescriptorSetLayout(desc)));
}
Ref<GfxDescriptorSet> GfxContext::AllocateDescriptorSet(GfxDescriptorSetLayout &layout) {
	return Ref<GfxDescriptorSet>(new GfxDescriptorSet(m_Device->AllocateDescriptorSet(layout.GetRHI())));
}
Ref<GfxRenderPass> GfxContext::CreateRenderPass(const RHI::RenderPassDesc &desc) {
	return Ref<GfxRenderPass>(new GfxRenderPass(m_Device->CreateRenderPass(desc)));
}
Ref<GfxCommandList> GfxContext::CreateCommandList(RHI::CommandListType type, const std::string &name) {
	return Ref<GfxCommandList>(new GfxCommandList(m_Device->CreateCommandList(type, name)));
}

void GfxContext::SubmitFrame(GfxCommandList &cmd, GfxSwapchain *swapchain, uint32 imageIndex) {
	m_Device->SubmitFrame(cmd.GetRHI(), (swapchain != nullptr) ? &swapchain->GetRHI() : nullptr, imageIndex);
}

void GfxContext::SubmitAndWait(GfxCommandList &cmd) {
	if (!cmd.IsRecording()) {
		return;
	}
	m_Device->SubmitAndWait(cmd.GetRHI());
}

void GfxContext::CopyBuffer(GfxBuffer &src, GfxBuffer &dst, uint64 size, uint64 srcOffset, uint64 dstOffset) {
	ExecuteImmediate(RHI::CommandListType::Transfer, [&](GfxCommandList &cmd) {
		m_Device->CopyBuffer(cmd.GetRHI(), src.GetRHI(), dst.GetRHI(), size, srcOffset, dstOffset);
	});
}

void GfxContext::WaitIdle() {
	m_Device->WaitIdle();
}

} // namespace Aquila::GFX

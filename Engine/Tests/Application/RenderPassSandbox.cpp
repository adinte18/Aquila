#include "Aquila/Application/ApplicationNew.h"
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Foundation/Color.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxMesh.h"
#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Graphics/RenderGraph/RGPassBuilder.h"
#include "Aquila/Graphics/Resources/Mesh.h"
#include "Aquila/RHI/Vulkan/VulkanShaderCompiler.h"
#include "Aquila/Foundation/SharedConstants.h"

using namespace Aquila;
using namespace Aquila::Graphics;
using Aquila::SharedConstants::kShadersDir;

struct CubePushConstants {
	mat4 mvp;
	vec4 color;
};

static Ref<GFX::GfxPipeline>
CompilePipeline(GFX::GfxContext &ctx, const std::string &shaderPath,
				const std::vector<RHI::TextureFormat> &colorFormats, RHI::TextureFormat depthFormat,
				const std::vector<RHI::IRHIDescriptorSetLayout *> &setLayouts = {}, uint32 pushConstantSize = 0,
				RHI::ShaderStageFlags pushStages = RHI::ShaderStageFlags::Vertex,
				RHI::CullMode cull = RHI::CullMode::None, bool depthTest = false, bool depthWrite = false,
				bool blendEnable = false, bool noVertexInput = false) {
	std::vector<RHI::VulkanCompiledStage> stages;
	std::string err;
	if (!RHI::VulkanShaderCompiler::CompileFile(shaderPath, stages, err)) {
		AQUILA_LOG_ERROR("Shader compile failed [{}]: {}", shaderPath, err);
	}

	RHI::GraphicsPipelineDesc desc{};
	for (auto &s : stages) {
		RHI::ShaderStageDesc sd{ .spirv = s.spirv, .entryPoint = s.entryPointName };
		if (s.stage == VK_SHADER_STAGE_VERTEX_BIT) {
			sd.stage = RHI::ShaderStageFlags::Vertex;
			desc.vertexShader = sd;
		} else {
			sd.stage = RHI::ShaderStageFlags::Fragment;
			desc.fragmentShader = sd;
		}
	}
	desc.colorFormats = colorFormats;
	desc.depthFormat = depthFormat;
	desc.topology = RHI::PrimitiveTopology::TriangleList;
	desc.raster.cullMode = cull;
	desc.raster.frontFace = RHI::FrontFace::Clockwise;
	desc.depthStencil.depthTest = depthTest;
	desc.depthStencil.depthWrite = depthWrite;
	desc.setLayouts = setLayouts;
	desc.noVertexInput = noVertexInput;
	if (pushConstantSize > 0) {
		desc.pushConstants = { { pushStages, 0, pushConstantSize } };
	}
	if (blendEnable) {
		desc.blendAttachments = { { true } };
	}
	return ctx.CreateGraphicsPipeline(desc);
}

class RenderPassSandbox : public Application::Application {
  public:
	RenderPassSandbox() : Application({ .Name = "RenderPassSandbox", .Width = 1280, .Height = 720 }) {}

	void OnStart() override {
		RHI::VulkanShaderCompiler::Initialize();
		m_Ctx = GFX::GfxContext::Create(*GetWindow().GetNativeWindow());

		m_Graph = CreateUnique<RG::RenderGraph>();

		m_Swapchain = m_Ctx->CreateSwapchain({
			.width = GetWindow().GetWidth(),
			.height = GetWindow().GetHeight(),
			.format = RHI::TextureFormat::BGRA8,
			.imageCount = 2,
			.vsync = true,
		});

		RebuildSizedResources();

		Graphics::Resources::Mesh cubeCPU("Cube");
		cubeCPU.LoadFromData(Graphics::Resources::Mesh::GenerateCube(0.5f));
		m_CubeMesh = GFX::GfxMesh::Create(*m_Ctx, cubeCPU);

		// 3D cube pipeline — depth test+write, RGBA16F target, Depth32 attachment
		m_CubePipeline = CompilePipeline(*m_Ctx, kShadersDir + "Basic.slang", { RHI::TextureFormat::RGBA16F },
										 RHI::TextureFormat::Depth32, {}, sizeof(CubePushConstants),
										 RHI::ShaderStageFlags::Vertex | RHI::ShaderStageFlags::Fragment,
										 RHI::CullMode::Back, /*depthTest=*/true, /*depthWrite=*/true);

		m_QuadBatcher = CreateUnique<QuadBatcher>(*m_Ctx);

		AQUILA_LOG_INFO("RenderPassSandbox ready. Esc = quit.");
	}

	void OnUpdate(f32 dt) override {
		if (Platform::Input::IsKeyPressed(Aquila::Application::Events::KeyCode::Escape)) {
			Close();
			return;
		}

		uint32 imageIndex = 0;
		if (!m_Swapchain->AcquireNextImage(imageIndex)) {
			HandleResize();
			return;
		}

		auto cmd = m_Ctx->CreateCommandList(RHI::CommandListType::Graphics, "SandboxFrame");
		cmd->Begin();

		BuildAndExecuteGraph(*cmd);
		BlitToSwapchain(*cmd, imageIndex);

		m_Ctx->SubmitFrame(*cmd, m_Swapchain.get(), imageIndex);

		m_Time += dt;

		if (m_Swapchain->NeedsResize()) {
			HandleResize();
		}
	}

	void OnEvent(Aquila::Application::Events::Event &event) override {
		Application::OnEvent(event);
		Aquila::Application::Events::EventDispatcher d(event);
		d.Dispatch<Aquila::Application::Events::KeyPressedEvent>(
			[this](Aquila::Application::Events::KeyPressedEvent &e) {
				if (e.GetKeyCode() == Aquila::Application::Events::KeyCode::Tab) {
					AQUILA_LOG_INFO("Tab key pressed");
				}
				return false;
			});
	}

	void OnShutdown() override {
		m_Ctx->WaitIdle();
		m_QuadBatcher.reset();
		m_Ctx.reset();
		RHI::VulkanShaderCompiler::Shutdown();
	}

  private:
	mat4 BuildCubeMVP() const {
		f32 aspect = static_cast<f32>(m_Swapchain->GetWidth()) / static_cast<f32>(m_Swapchain->GetHeight());
		mat4 proj = glm::perspective(glm::radians(60.f), aspect, 0.1f, 100.f);
		mat4 view = glm::lookAt(vec3(0.f, 1.5f, 4.f), vec3(0.f), vec3(0.f, 1.f, 0.f));
		mat4 model = glm::rotate(mat4(1.f), m_Time, vec3(0.f, 1.f, 0.f));
		return proj * view * model;
	}

	mat4 BuildOrtho() const {
		f32 W = static_cast<f32>(m_Swapchain->GetWidth());
		f32 H = static_cast<f32>(m_Swapchain->GetHeight());
		return glm::ortho(0.f, W, H, 0.f, -1.f, 1.f);
	}

	void BuildAndExecuteGraph(GFX::GfxCommandList &cmd) {
		// kill the state of previous frame, we are deliberatelly saying to the graph to forget everything it knows about last frame
		m_Graph->Reset();

		// we proceed with describing how the next frame should look like

		RG::RGTextureHandle hScene = m_Graph->ImportTexture(m_SceneTex.get(), "Scene");
		RG::RGTextureHandle hUI = m_Graph->ImportTexture(m_UITex.get(), "UI");
		RG::RGTextureHandle hComposite = m_Graph->ImportTexture(m_CompositeTex.get(), "Composite");
		RG::RGTextureHandle hDepth = m_Graph->ImportTexture(m_DepthTex.get(), "Depth");

		const mat4 cubeMVP = BuildCubeMVP();
		const mat4 orthoVP = BuildOrtho();

		// Writes: hScene (color, clear), hDepth (depth, clear)
		// Reads:  nothing  ->  no dependency on any prior pass

		// The RG compiler will infer:
		//   hScene: Undefined -> ColorAttachment   (layout transition)
		//   hDepth: Undefined -> DepthStencilWrite (layout transition)

		m_Graph->AddPass(
			"CubePass",
			[&hScene, &hDepth](RG::RGPassBuilder &b) {
				hScene = b.SetColorAttachment(0, hScene, RG::AttachmentLoadOp::Clear, RG::AttachmentStoreOp::Store,
											  { vec4(0.05f, 0.05f, 0.08f, 1.0f) });
				b.SetDepthAttachment(hDepth, RG::AttachmentLoadOp::Clear, RG::AttachmentStoreOp::DontCare,
									 RG::AttachmentLoadOp::DontCare, RG::AttachmentStoreOp::DontCare,
									 /*readOnly=*/false, RG::ClearDepth{ .depth = 1.f });
			},

			[this, cubeMVP](GFX::GfxCommandList &cmd, RG::RGRegistry & /*reg*/) {
				cmd.BindPipeline(*m_CubePipeline);

				CubePushConstants pc{ .mvp = cubeMVP, .color = vec4(0.8f, 0.4f, 0.2f, 1.0f) };
				cmd.PushConstants(pc, RHI::ShaderStageFlags::Vertex | RHI::ShaderStageFlags::Fragment);

				cmd.BindVertexBuffer(m_CubeMesh->GetVertexBuffer());
				cmd.BindIndexBuffer(m_CubeMesh->GetIndexBuffer());
				cmd.DrawIndexed(m_CubeMesh->GetIndexCount());
			});

		// Writes: hUI (color, clear)
		// Reads:  nothing  ->  independent of CubePass
		//
		// The RG sees no shared resource between CubePass and UIPass, so it is
		// free to schedule them in any order.  On a GPU with a dedicated async
		// compute / transfer queue they could even overlap.

		m_Graph->AddPass(
			"UIPass",
			[&hUI](RG::RGPassBuilder &b) {
				hUI = b.SetColorAttachment(0, hUI, RG::AttachmentLoadOp::Clear, RG::AttachmentStoreOp::Store,
										   { vec4(0.08f, 0.08f, 0.10f, 1.0f) });
			},
			[this, orthoVP](GFX::GfxCommandList &cmd, RG::RGRegistry & /*reg*/) {
				// UIPass has no depth attachment — pass depthFormat=None to QuadBatcher
				// so the pipeline it compiles matches this render pass exactly.
				m_QuadBatcher->Begin(cmd, RHI::TextureFormat::RGBA16F, RHI::SampleCount::x1, orthoVP,
									RHI::TextureFormat::None);

				f32 W = static_cast<f32>(m_Swapchain->GetWidth());
				f32 H = static_cast<f32>(m_Swapchain->GetHeight());

				// Header bar
				m_QuadBatcher->DrawRect({
					.position = { 0.f, 0.f },
					.size = { W, 40.f },
					.color = vec4(0.15f, 0.15f, 0.20f, 1.0f),
				});

				// Animated fill inside the status bar
				float fill = (std::sin(m_Time * 0.8f) * 0.5f + 0.5f) * (W - 20.f);
				m_QuadBatcher->DrawRect({
					.position = { 10.f, H - 26.f },
					.size = { fill, 22.f },
					.color = vec4(0.3f, 0.7f, 1.0f, 0.8f),
				});

				m_QuadBatcher->End();
			});

		// Reads:  hScene (ShaderRead) <- RAW dependency on CubePass
		//         hUI    (ShaderRead) <- RAW dependency on UIPass
		// Writes: hComposite (color)
		// Before CompositePass begins, two pipeline barriers are emitted:
		//   hScene: COLOR_ATTACHMENT_WRITE -> SHADER_READ
		//   hUI:    COLOR_ATTACHMENT_WRITE -> SHADER_READ

		m_Graph->AddPass(
			"CompositePass",
			[&hScene, &hUI, &hComposite](RG::RGPassBuilder &b) {
				// Declare reads, these create the dependency edges described above.
				// ReadTexture does not bump the version; both handles remain valid.
				b.ReadTexture(hScene, RG::ResourceState::ShaderRead);
				b.ReadTexture(hUI, RG::ResourceState::ShaderRead);
				// Declare write, normally that means version bump so new handle will be returned.
				hComposite =
					b.SetColorAttachment(0, hComposite, RG::AttachmentLoadOp::DontCare, RG::AttachmentStoreOp::Store);
			},
			[this, hScene, hUI](GFX::GfxCommandList &cmd, RG::RGRegistry &reg) {
				auto W = static_cast<float>(m_Swapchain->GetWidth());
				auto H = static_cast<float>(m_Swapchain->GetHeight());
				auto Wh = static_cast<uint32>(W * 0.5f);

				// Both halves share the same blit pipeline (RGBA16F -> RGBA16F composite).
				cmd.BindPipeline(*m_BlitPipeline16F);

				// Left half (hScene)
				// SetViewport maps the fullscreen triangle NDC space into this region,
				// so the full source texture is stretched into the left half.
				cmd.SetViewport(0.f, 0.f, W * 0.5f, H);
				cmd.SetScissor(0, 0, Wh, static_cast<uint32>(H));
				m_SceneSampleSet->SetTexture(0, reg.GetTexture(hScene));
				m_SceneSampleSet->Flush();
				cmd.BindDescriptorSet(0, *m_SceneSampleSet);
				cmd.Draw(3);

				// Right half (hUI)
				cmd.SetViewport(W * 0.5f, 0.f, W * 0.5f, H);
				cmd.SetScissor(static_cast<int32>(Wh), 0, Wh, static_cast<uint32>(H));
				m_UISampleSet->SetTexture(0, reg.GetTexture(hUI));
				m_UISampleSet->Flush();
				cmd.BindDescriptorSet(0, *m_UISampleSet);
				cmd.Draw(3);
			});

		// at this point the graph should completely analyse if the graph is valid or not
		// is parallel (well not really because its sequential lol),
		// but it will allocate necessary resources based on what the mighty dev describes
		m_Graph->Compile(*m_Ctx);

		// if something goes wrong, compile will let you know at runtime through loud ass asserts
		// but if everything is fine, execute will record the GPU commands you asked earlier
		// so everything you did before compile, will execute right here
		m_Graph->Execute(cmd);

		// this is optional, but better be safe than sorry
		m_Graph->Reset();
	}

	// Blits hComposite (RGBA16F) -> swapchain (BGRA8).
	// Lives outside the graph because swapchain images are not GfxTextures.

	void BlitToSwapchain(GFX::GfxCommandList &cmd, uint32 imageIndex) {
		// RG left hComposite in ColorAttachment state; transition to ShaderRead.
		cmd.TransitionTexture(*m_CompositeTex, RHI::ResourceState::ColorAttachment, RHI::ResourceState::ShaderRead);

		m_SwapchainPass->Begin(cmd, m_Swapchain.get(), imageIndex);

		cmd.BindPipeline(*m_BlitPipelineBGRA8);

		m_BlitSet->SetTexture(0, *m_CompositeTex);
		m_BlitSet->Flush();
		cmd.BindDescriptorSet(0, *m_BlitSet);

		cmd.Draw(3);

		m_SwapchainPass->End(cmd);
	}

	void RebuildSizedResources() {
		uint32 W = m_Swapchain->GetWidth();
		uint32 H = m_Swapchain->GetHeight();

		// Three offscreen render targets — all RGBA16F so the blit pipeline is the
		// same format for all intermediate passes.
		auto makeRT = [&](const char *name) {
			return m_Ctx->CreateTexture({
				.width = W,
				.height = H,
				.format = RHI::TextureFormat::RGBA16F,
				.usage = RHI::TextureUsage::ColorAttachment | RHI::TextureUsage::Sampled,
				.sampler = RHI::SamplerDesc::RenderTarget(),
				.debugName = name,
			});
		};
		m_SceneTex = makeRT("Scene");
		m_UITex = makeRT("UI");
		m_CompositeTex = makeRT("Composite");

		m_DepthTex = m_Ctx->CreateTexture({
			.width = W,
			.height = H,
			.format = RHI::TextureFormat::Depth32,
			.usage = RHI::TextureUsage::DepthAttachment,
			.debugName = "Depth",
		});

		// Blit resources are created once; descriptor sets are updated each frame.
		if (!m_BlitLayout) {
			m_BlitLayout = m_Ctx->CreateDescriptorSetLayout({
				.bindings = { { .binding = 0,
								.type = RHI::DescriptorType::CombinedImageSampler,
								.stages = RHI::ShaderStageFlags::Fragment,
								.count = 1 } },
			});

			// RGBA16F -> RGBA16F  (used by CompositePass to blit scene/UI into composite)
			m_BlitPipeline16F = CompilePipeline(*m_Ctx, kShadersDir + "Blit.slang", { RHI::TextureFormat::RGBA16F },
												RHI::TextureFormat::None, { &m_BlitLayout->GetRHI() }, 0,
												RHI::ShaderStageFlags::Vertex, RHI::CullMode::None,
												/*depthTest=*/false, /*depthWrite=*/false, /*blendEnable=*/false,
												/*noVertexInput=*/true);

			// RGBA16F -> BGRA8  (used by the manual swapchain blit)
			m_BlitPipelineBGRA8 = CompilePipeline(*m_Ctx, kShadersDir + "Blit.slang", { RHI::TextureFormat::BGRA8 },
												  RHI::TextureFormat::None, { &m_BlitLayout->GetRHI() }, 0,
												  RHI::ShaderStageFlags::Vertex, RHI::CullMode::None,
												  /*depthTest=*/false, /*depthWrite=*/false, /*blendEnable=*/false,
												  /*noVertexInput=*/true);

			// One descriptor set per sampled input in CompositePass, plus one for the
			// swapchain blit.  All share the same layout (single sampler binding).
			m_SceneSampleSet = m_Ctx->AllocateDescriptorSet(*m_BlitLayout);
			m_UISampleSet = m_Ctx->AllocateDescriptorSet(*m_BlitLayout);
			m_BlitSet = m_Ctx->AllocateDescriptorSet(*m_BlitLayout);

			m_SwapchainPass = m_Ctx->CreateRenderPass({
				.colorAttachments = { { .loadOp = RHI::AttachmentLoadOp::DontCare,
										.storeOp = RHI::AttachmentStoreOp::Store } },
				.useSwapchain = true,
				.debugName = "SwapchainBlit",
			});
		}
	}

	void HandleResize() {
		uint32 W = GetWindow().GetWidth();
		uint32 H = GetWindow().GetHeight();
		if (W == 0 || H == 0) {
			return;
		}
		m_Ctx->WaitIdle();
		m_Swapchain->Resize(W, H);
		RebuildSizedResources();
	}

	Unique<GFX::GfxContext> m_Ctx;
	Unique<RG::RenderGraph> m_Graph;
	Unique<QuadBatcher> m_QuadBatcher;

	Ref<GFX::GfxSwapchain> m_Swapchain;

	// Render targets (recreated on resize)
	Ref<GFX::GfxTexture> m_SceneTex;	 // CubePass output
	Ref<GFX::GfxTexture> m_UITex;		 // UIPass output
	Ref<GFX::GfxTexture> m_CompositeTex; // CompositePass output -> swapchain blit input
	Ref<GFX::GfxTexture> m_DepthTex;	 // CubePass depth

	// Geometry + pipeline
	Ref<GFX::GfxMesh> m_CubeMesh;
	Ref<GFX::GfxPipeline> m_CubePipeline;

	// Shared descriptor set layout (one sampler binding — reused for all blit sets)
	Ref<GFX::GfxDescriptorSetLayout> m_BlitLayout;

	// Blit pipelines — same Blit.slang shader, different color format
	Ref<GFX::GfxPipeline> m_BlitPipeline16F;   // RGBA16F  (CompositePass)
	Ref<GFX::GfxPipeline> m_BlitPipelineBGRA8; // BGRA8    (swapchain blit)

	// Descriptor sets — one per sampled texture (updated each frame before draw)
	Ref<GFX::GfxDescriptorSet> m_SceneSampleSet; // hScene  in CompositePass
	Ref<GFX::GfxDescriptorSet> m_UISampleSet;	 // hUI     in CompositePass
	Ref<GFX::GfxDescriptorSet> m_BlitSet;		 // hComposite in swapchain blit

	// Swapchain render pass — Begin/End only, no draw state
	Ref<GFX::GfxRenderPass> m_SwapchainPass;

	f32 m_Time = 0.f;
};

int main() {
	RenderPassSandbox app;
	app.OnStart();
	app.Run();
	return 0;
}

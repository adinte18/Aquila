#pragma once
#include "Aquila/Graphics/RenderGraph/RGRegistry.h"
#include "Aquila/Graphics/RenderGraph/RGPassBuilder.h"
#include "Aquila/Graphics/RenderGraph/RGCompiler.h"

namespace Aquila::GFX {
class GfxContext;
class GfxCommandList;
} // namespace Aquila::GFX

namespace Aquila::Graphics::RG {

class RenderGraph {
  public:
	RenderGraph() = default;
	~RenderGraph() = default;

	RenderGraph(const RenderGraph &) = delete;
	RenderGraph &operator=(const RenderGraph &) = delete;
	RenderGraph(RenderGraph &&) = default;
	RenderGraph &operator=(RenderGraph &&) = default;

	RGTextureHandle DeclareTexture(const RGTextureDesc &desc) { return m_Registry.DeclareTexture(desc); }

	RGBufferHandle DeclareBuffer(const RGBufferDesc &desc) { return m_Registry.DeclareBuffer(desc); }

	RGTextureHandle ImportTexture(GFX::GfxTexture *tex, std::string_view name = {}) {
		return m_Registry.ImportTexture(tex, name);
	}

	RGBufferHandle ImportBuffer(GFX::GfxBuffer *buf, std::string_view name = {}) {
		return m_Registry.ImportBuffer(buf, name);
	}

	/// Register a pass with a setup lambda and an execute lambda.
	///
	/// @param name        Debug label shown in GPU profilers / validation.
	/// @param setupFn     Called immediately (sync) to declare resource accesses.
	///                    Receives an RGPassBuilder by reference.
	/// @param executeFn   Captured and called later during Execute().
	///                    Receives a resolved command list and the registry.
	template <typename SetupFn, typename ExecuteFn>
	void AddPass(std::string_view name, SetupFn &&setupFn, ExecuteFn &&executeFn) {
		RGPassBuilder builder(name, m_Registry);

		// Run setup immediately so resource versioning stays in-order.
		std::forward<SetupFn>(setupFn)(builder);

		RGPassData data = std::move(builder).TakeData();
		data.RenderPassExecute = std::forward<ExecuteFn>(executeFn);

		m_Passes.push_back(std::move(data));
	}

	/// Must be called once per frame after all AddPass calls and before Execute().
	void Compile(GFX::GfxContext &ctx);

	/// Replay the compiled schedule.
	/// Compile() must have been called first.
	void Execute(GFX::GfxCommandList &cmd);

	/// Reset all state for the next frame (releases transient resources).
	void Reset();

	[[nodiscard]] const RGRegistry &GetRegistry() const { return m_Registry; }
	[[nodiscard]] const std::vector<RGPassData> &GetPasses() const { return m_Passes; }
	[[nodiscard]] const RGCompiledGraph &GetCompiled() const { return m_Compiled; }

  private:
	RGRegistry m_Registry;
	std::vector<RGPassData> m_Passes;
	RGCompiledGraph m_Compiled;
};

} // namespace Aquila::Graphics::RG

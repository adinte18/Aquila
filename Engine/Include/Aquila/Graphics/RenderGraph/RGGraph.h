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

	RGTextureHandle declare_texture(const RGTextureDesc &desc) { return m_registry.declare_texture(desc); }

	RGBufferHandle declare_buffer(const RGBufferDesc &desc) { return m_registry.declare_buffer(desc); }

	RGTextureHandle import_texture(GFX::GfxTexture *tex, std::string_view name = {},
								  RG::ResourceState initial_state = RG::ResourceState::Undefined) {
		return m_registry.import_texture(tex, name, initial_state);
	}

	RGBufferHandle import_buffer(GFX::GfxBuffer *buf, std::string_view name = {},
								RG::ResourceState initial_state = RG::ResourceState::Undefined) {
		return m_registry.import_buffer(buf, name, initial_state);
	}

	/// Register a pass with a setup lambda and an execute lambda.
	///
	/// @param name        Debug label shown in GPU profilers / validation.
	/// @param setupFn     Called immediately (sync) to declare resource accesses.
	///                    Receives an RGPassBuilder by reference.
	/// @param executeFn   Captured and called later during Execute().
	///                    Receives a resolved command list and the registry.
	template <typename SetupFn, typename ExecuteFn>
	void add_pass(std::string_view name, SetupFn &&setup_fn, ExecuteFn &&execute_fn) {
		RGPassBuilder builder(name, m_registry);

		// Run setup immediately so resource versioning stays in-order.
		std::forward<SetupFn>(setup_fn)(builder);

		RGPassData data = std::move(builder).take_data();
		data.render_pass_execute = std::forward<ExecuteFn>(execute_fn);

		m_passes.push_back(std::move(data));
	}

	/// Must be called once per frame after all AddPass calls and before Execute().
	void compile(GFX::GfxContext &ctx);

	/// Replay the compiled schedule.
	/// Compile() must have been called first.
	void execute(GFX::GfxCommandList &cmd);

	/// Reset all state for the next frame (releases transient resources).
	void reset();

	[[nodiscard]] const RGRegistry &get_registry() const { return m_registry; }
	[[nodiscard]] const std::vector<RGPassData> &get_passes() const { return m_passes; }
	[[nodiscard]] const RGCompiledGraph &get_compiled() const { return m_compiled; }

  private:
	RGRegistry m_registry;
	std::vector<RGPassData> m_passes;
	RGCompiledGraph m_compiled;
};

} // namespace Aquila::Graphics::RG

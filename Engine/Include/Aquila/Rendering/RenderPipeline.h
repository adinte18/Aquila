#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Rendering/Renderers/IRenderer.h"
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::GFX {
class GfxContext;
class GfxCommandList;
} // namespace Aquila::GFX

namespace Aquila::SceneManagement {
class Scene;
}

namespace Aquila::Rendering {

class RenderPipeline {
  public:
	RenderPipeline(GFX::GfxContext &ctx, uint32 width, uint32 height);
	~RenderPipeline();

	AQUILA_NONCOPYABLE(RenderPipeline);
	AQUILA_NONMOVEABLE(RenderPipeline);

	template <typename T, typename... Args> T &Add(Args &&...args) {
		static_assert(std::is_base_of_v<IRenderer, T>, "T must derive from IRenderer");
		auto renderer = CreateUnique<T>(std::forward<Args>(args)...);
		T &ref = *renderer;
		renderer->OnInit(m_Ctx);
		m_Renderers.push_back(std::move(renderer));
		return ref;
	}

	void Render(GFX::GfxCommandList &cmd, SceneManagement::Scene &scene, f32 deltaTime);
	void Render(GFX::GfxCommandList &cmd, SceneManagement::Scene &scene, f32 deltaTime, uint32 width, uint32 height);
	void Resize(uint32 width, uint32 height);

	[[nodiscard]] GFX::GfxTexture &GetOutput() const { return *m_SceneColor; }
	[[nodiscard]] uint32 GetWidth() const { return m_Width; }
	[[nodiscard]] uint32 GetHeight() const { return m_Height; }

  private:
	void BuildFrameContext(SceneManagement::Scene &scene, f32 deltaTime, FrameContext &out);
	void RebuildTargets();

	GFX::GfxContext &m_Ctx;
	Graphics::RG::RenderGraph m_Graph;
	std::vector<Unique<IRenderer>> m_Renderers;

	Ref<GFX::GfxTexture> m_SceneColor;
	Ref<GFX::GfxTexture> m_DepthTex;

	uint32 m_Width = 0;
	uint32 m_Height = 0;
};

} // namespace Aquila::Rendering

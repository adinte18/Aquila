#ifndef AQUILA_APPLICATION_H
#define AQUILA_APPLICATION_H

#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/Timer.h"
#include "Aquila/Application/Window.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxSwapchain.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Rendering/RenderPipeline.h"
#include "Aquila/Rendering/Renderers/Renderer.h"
#include "Aquila/Rendering/Renderers/Renderer2D.h"

struct ApplicationSpec {
	std::string Name = "Aquila";
	uint32 Width = 1920;
	uint32 Height = 1080;
};

namespace Aquila::Application {

class Application {
  public:
	explicit Application(const ApplicationSpec &spec);
	virtual ~Application();

	AQUILA_NONCOPYABLE(Application);
	AQUILA_NONMOVEABLE(Application);

	void Run();
	void Close();

	Window &GetWindow() { return *m_Window; }

  protected:
	virtual void OnInit() {}
	virtual void OnShutdown() {}
	virtual void OnPreRender(f32 deltaTime) {}
	virtual void OnEvent(Events::Event &event) {}
	virtual void OnResize(uint32 width, uint32 height) {}

	GFX::GfxContext &GetContext() { return *m_Ctx; }
	GFX::GfxTexture &GetRenderOutput() { return m_RenderPipeline->GetOutput(); }
	SceneManagement::Scene &GetScene() { return *m_Scene; }
	Rendering::RenderPipeline &GetRenderPipeline() { return *m_RenderPipeline; }
	Rendering::Renderer &GetRenderer() { return *m_Renderer; }
	Rendering::Renderer2D &GetRenderer2D() { return *m_Renderer2D; }

  private:
	void InternalUpdate(f32 deltaTime);
	void InternalOnEvent(Events::Event &event);
	void HandleResize();
	void InitRendering(uint32 width, uint32 height);

	ApplicationSpec m_Spec;
	Unique<Window> m_Window;
	Unique<Foundation::Stopwatch> m_Timer;
	bool m_Running = true;
	bool m_PendingResize = false;

	Unique<GFX::GfxContext> m_Ctx;
	Ref<GFX::GfxSwapchain> m_Swapchain;
	Unique<SceneManagement::Scene> m_Scene;
	Unique<Rendering::RenderPipeline> m_RenderPipeline;
	Rendering::Renderer *m_Renderer = nullptr;
	Rendering::Renderer2D *m_Renderer2D = nullptr;
};

} // namespace Aquila::Application

#endif

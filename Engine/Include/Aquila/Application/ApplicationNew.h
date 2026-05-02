#ifndef AQUILA_APPLICATION_H
#define AQUILA_APPLICATION_H

#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/Timer.h"
#include "Aquila/Platform/Input.h"
#include "Aquila/Application/Window.h"

// GFX
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxSwapchain.h"

// Scene
#include "Aquila/Scene/Scene.h"

// Rendering
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

	virtual void OnStart();
	virtual void OnShutdown();
	virtual void OnUpdate(f32 deltaTime);
	virtual void OnEvent(Events::Event &event);

	Window &GetWindow() { return *m_Window; }
	SceneManagement::Scene &GetScene() { return *m_Scene; }
	Rendering::RenderPipeline &GetPipeline() { return *m_RenderPipeline; }

  protected:
	virtual void SetupScene();

  private:
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

#ifndef AQUILA_APPLICATION_H
#define AQUILA_APPLICATION_H

#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/Timer.h"

#include "Aquila/Platform/Input.h"

#include "Aquila/Application/Window.h"

struct ApplicationSpec {
	std::string Name = "Aquila";
	uint32 Width = 1280;
	uint32 Height = 720;
};

namespace Aquila::Application {
class Application {
  public:
	Application(const ApplicationSpec &spec);
	virtual ~Application();

	AQUILA_NONCOPYABLE(Application);
	AQUILA_NONMOVEABLE(Application);

	void Run();
	void Close();

	virtual void OnStart();
	virtual void OnShutdown();
	virtual void OnUpdate(float deltaTime);
	virtual void OnEvent(Events::Event &event);

	Window &GetWindow() { return *m_Window; }

  private:
	void Init();
	void Shutdown();

	ApplicationSpec m_Spec;
	Unique<Window> m_Window;
	Unique<Foundation::Stopwatch> m_Timer;
	bool m_Running = true;
};

} // namespace Aquila::Application

#endif

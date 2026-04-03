#include "Aquila/Application/ApplicationNew.h"
#include "Aquila/Platform/Input.h"

namespace Aquila::Application {

Application::Application(const ApplicationSpec &spec) {
	m_Timer = CreateUnique<Foundation::Stopwatch>();
	m_Window = CreateUnique<Window>(spec.Width, spec.Height, spec.Name);

	m_Window->SetEventCallback([this](Events::Event &event) {
		Aquila::Platform::Input::OnEvent(event);
		OnEvent(event);
	});

	m_Timer->Start();
}

Application::~Application() = default;

void Application::OnStart() {}

void Application::Run() {
	while (m_Running && !m_Window->ShouldClose()) {
		m_Timer->Tick();
		const f32 deltaTime = m_Timer->GetDeltaTime();

		m_Window->PollEvents();
		OnUpdate(deltaTime);
		// m_World->Tick(deltaTime);
	}
}

void Application::Close() {
	m_Running = false;
	OnShutdown();
}

void Application::OnUpdate(f32 deltaTime) {}

void Application::OnEvent(Events::Event &event) {
	Events::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Events::WindowCloseEvent>([this](Events::WindowCloseEvent &event) {
		m_Running = false;
		return true;
	});

	dispatcher.Dispatch<Events::WindowResizeEvent>([this](Events::WindowResizeEvent &e) { return true; });
}

void Application::OnShutdown() {
	m_Window.reset();
}
} // namespace Aquila::Application

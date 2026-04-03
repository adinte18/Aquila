#include "Aquila/Application/ApplicationNew.h"
#include "Aquila/Application/Events/InputEvent.h"

class InputTesting : public Aquila::Application::Application {
  public:
	InputTesting() : Application({ .Name = "InputTesting", .Width = 1280, .Height = 720 }) {}
	~InputTesting() override = default;

	void OnStart() override { AQUILA_LOG_INFO("InputTesting initialized"); }

	void OnUpdate(float deltaTime) override {
		if (GetInput().IsKeyPressed(Aquila::Application::Events::KeyCode::Escape)) {
			Close();
		}
	}

	void OnEvent(Aquila::Application::Events::Event &event) override {
		Application::OnEvent(event);

		Aquila::Application::Events::EventDispatcher dispatcher(event);
		dispatcher.Dispatch<Aquila::Application::Events::KeyPressedEvent>(
			[](Aquila::Application::Events::KeyPressedEvent &event) {
				AQUILA_LOG_INFO("Key pressed: {}", static_cast<int>(event.GetKeyCode()));
				return false;
			});

		dispatcher.Dispatch<Aquila::Application::Events::KeyReleasedEvent>(
			[](Aquila::Application::Events::KeyReleasedEvent &event) {
				AQUILA_LOG_INFO("Key released: {}", static_cast<int>(event.GetKeyCode()));
				return false;
			});

		dispatcher.Dispatch<Aquila::Application::Events::MouseButtonPressedEvent>(
			[](Aquila::Application::Events::MouseButtonPressedEvent &event) {
				AQUILA_LOG_INFO("Mouse button pressed: {}", static_cast<int>(event.GetMouseButton()));
				return false;
			});

		dispatcher.Dispatch<Aquila::Application::Events::MouseButtonReleasedEvent>(
			[](Aquila::Application::Events::MouseButtonReleasedEvent &event) {
				AQUILA_LOG_INFO("Mouse button released: {}", static_cast<int>(event.GetMouseButton()));
				return false;
			});

		dispatcher.Dispatch<Aquila::Application::Events::MouseMovedEvent>(
			[](Aquila::Application::Events::MouseMovedEvent &event) {
				AQUILA_LOG_INFO("Mouse moved to position : [ {} {} ]", static_cast<int>(event.GetX()),
								static_cast<int>(event.GetY()));
				return false;
			});
	}
	void OnShutdown() override { AQUILA_LOG_INFO("InputTesting shutdown"); }
};

int main() {
	InputTesting app;
	app.OnStart();
	app.Run();
	return 0;
}

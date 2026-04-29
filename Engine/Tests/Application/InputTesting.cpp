#include "Aquila/Application/ApplicationNew.h"
#include "Aquila/Application/Events/InputEvent.h"

class InputTesting : public Aquila::Application::Application {
  public:
	InputTesting() : Application({ .Name = "InputTesting", .Width = 1280, .Height = 720 }) {}
	~InputTesting() override = default;

	void OnStart() override { AQUILA_LOG_INFO("InputTesting initialized"); }

	void OnUpdate(float deltaTime) override {
		if (Aquila::Platform::Input::IsKeyPressed(Aquila::Application::Events::KeyCode::Escape)) {
			Close();
		}
	}

	void OnEvent(Aquila::Application::Events::Event &event) override {}
	void OnShutdown() override { AQUILA_LOG_INFO("InputTesting shutdown"); }
};

int main() {
	InputTesting app;
	app.OnStart();
	app.Run();
	return 0;
}

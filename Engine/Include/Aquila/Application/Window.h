#ifndef AQUILA_WINDOW_H
#define AQUILA_WINDOW_H

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/Math/MathTypes.h"

#include "Aquila/Platform/Platform.h"

#include "Aquila/Application/Events/Event.h"
#include "Aquila/Application/Events/WindowEvent.h"
#include "Aquila/Application/Events/InputEvent.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Aquila::Application {

class Window {
  public:
	using EventCallbackFn = std::function<void(Events::Event &)>;

	Window(uint32 width, uint32 height, const std::string &title);
	~Window();

	void PollEvents();
	void WaitEvents();
	bool ShouldClose() const;

	uint32 GetWidth() const { return m_Data.Width; }
	uint32 GetHeight() const { return m_Data.Height; }
	void SetTitle(const std::string &text) const;

	void SetEventCallback(const EventCallbackFn &callback) { m_Data.EventCallback = callback; }

	// Called from inside the Win32 modal resize loop (WM_PAINT/refresh).
	// Hook this to render a frame during live resize so the window doesn't go black.
	void SetRefreshCallback(std::function<void()> callback) { m_Data.RefreshCallback = std::move(callback); }

	bool IsWindowResized() const { return m_Data.Resized; }
	void ResetResizedFlag() { m_Data.Resized = false; }

	GLFWwindow *GetNativeWindow() const { return m_Window; }
	void CreateWindowSurface(VkInstance instance, VkSurfaceKHR *surface) const;

  private:
	void Initialize();
	void Shutdown() const;
	void SetupCallbacks() const;
	GLFWwindow *m_Window;

	struct WindowData {
		std::string Title;
		uint32 Width, Height;
		bool Resized = false;
		EventCallbackFn EventCallback;
		std::function<void()> RefreshCallback;
		f32 LastMouseX = 0.f, LastMouseY = 0.f;
		bool HasPendingMouseMove = false;
	};

	WindowData m_Data;
};

} // namespace Aquila::Application

#endif // AQUILA_WINDOW_H

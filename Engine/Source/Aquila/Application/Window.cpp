#include "Aquila/Application/Window.h"
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Application/Events/WindowEvent.h"

namespace Aquila::Application {

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char *description) {
	AQUILA_LOG_ERROR("GLFW Error ({}): {}", error, description);
}

Window::Window(const uint32 width, const uint32 height, const std::string &title) {
	m_Data.Title = title;
	m_Data.Width = width;
	m_Data.Height = height;

	Initialize();
}

Window::~Window() {
	Shutdown();
}

void Window::Initialize() {
	if (!s_GLFWInitialized) {
		const int success = glfwInit();
		AQUILA_ASSERT(success, "Could not initialize GLFW!");
		glfwSetErrorCallback(GLFWErrorCallback);
		s_GLFWInitialized = true;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);

	glfwSetWindowUserPointer(m_Window, &m_Data);
	glfwMaximizeWindow(m_Window);
	SetupCallbacks();

	// Sync dimensions after maximize — the WM_SIZE fires before SetupCallbacks so
	// the size callback never ran for it; query the actual size directly.
	int actualW, actualH;
	glfwGetWindowSize(m_Window, &actualW, &actualH);
	m_Data.Width = static_cast<uint32>(actualW);
	m_Data.Height = static_cast<uint32>(actualH);

	AQUILA_LOG_INFO("Window created: {}x{}", m_Data.Width, m_Data.Height);
}

void Window::SetTitle(const std::string &text) const {
	const auto title = std::string(m_Data.Title + " | " + text);
	glfwSetWindowTitle(m_Window, title.c_str());
}
void Window::SetupCallbacks() const {
	glfwSetWindowSizeCallback(m_Window, [](GLFWwindow *window, int width, int height) {
		WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		data.Width = width;
		data.Height = height;
		data.Resized = true;

		Events::WindowResizeEvent event(width, height);
		data.EventCallback(event);
	});

	glfwSetWindowCloseCallback(m_Window, [](GLFWwindow *window) {
		const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		Events::WindowCloseEvent event(static_cast<bool>(glfwWindowShouldClose(window)));
		data.EventCallback(event);
	});

	glfwSetKeyCallback(m_Window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
		const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));

		switch (action) {
		case GLFW_PRESS: {
			Events::KeyPressedEvent event(static_cast<Events::KeyCode>(key), 0, mods);
			data.EventCallback(event);
			break;
		}
		case GLFW_RELEASE: {
			Events::KeyReleasedEvent event(static_cast<Events::KeyCode>(key));
			data.EventCallback(event);
			break;
		}
		case GLFW_REPEAT: {
			Events::KeyPressedEvent event(static_cast<Events::KeyCode>(key), 1, mods);
			data.EventCallback(event);
			break;
		}
		default:;
		}
	});

	glfwSetCharCallback(m_Window, [](GLFWwindow *window, unsigned int keycode) {
		const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		Events::KeyTypedEvent event(static_cast<Events::KeyCode>(keycode));
		data.EventCallback(event);
	});

	glfwSetMouseButtonCallback(m_Window, [](GLFWwindow *window, int button, int action, int mods) {
		const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));

		switch (action) {
		case GLFW_PRESS: {
			Events::MouseButtonPressedEvent event(static_cast<Events::MouseButton>(button));
			data.EventCallback(event);
			break;
		}
		case GLFW_RELEASE: {
			Events::MouseButtonReleasedEvent event(static_cast<Events::MouseButton>(button));
			data.EventCallback(event);
			break;
		}
		default:;
		}
	});

	glfwSetScrollCallback(m_Window, [](GLFWwindow *window, const double xOffset, const double yOffset) {
		const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		Events::MouseScrolledEvent event(static_cast<f32>(xOffset), static_cast<f32>(yOffset));
		data.EventCallback(event);
	});

	// Store the latest position; PollEvents fires one synthesized event after draining the queue.
	glfwSetCursorPosCallback(m_Window, [](GLFWwindow *window, const double xPos, const double yPos) {
		WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		data.LastMouseX = static_cast<f32>(xPos);
		data.LastMouseY = static_cast<f32>(yPos);
		data.HasPendingMouseMove = true;
	});

	glfwSetWindowFocusCallback(m_Window, [](GLFWwindow *window, const int focused) {
		const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		Events::WindowFocusEvent event(focused == GLFW_TRUE);
		data.EventCallback(event);
	});

	// On Windows, glfwPollEvents() is blocked inside Win32's modal resize loop while
	// the user drags a window border.  The refresh callback fires from inside that loop
	// whenever the window needs repainting (every mouse move during the drag), giving
	// us a chance to render a live frame.
	glfwSetWindowRefreshCallback(m_Window, [](GLFWwindow *window) {
		WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		if (data.RefreshCallback) {
			data.RefreshCallback();
		}
	});
}

void Window::PollEvents() {
	glfwPollEvents();
	if (m_Data.HasPendingMouseMove) {
		Events::MouseMovedEvent event(m_Data.LastMouseX, m_Data.LastMouseY);
		m_Data.EventCallback(event);
		m_Data.HasPendingMouseMove = false;
	}
}

void Window::WaitEvents() {
	glfwWaitEvents();
	if (m_Data.HasPendingMouseMove) {
		Events::MouseMovedEvent event(m_Data.LastMouseX, m_Data.LastMouseY);
		m_Data.EventCallback(event);
		m_Data.HasPendingMouseMove = false;
	}
}

bool Window::ShouldClose() const {
	return glfwWindowShouldClose(m_Window) != 0;
}

void Window::Shutdown() const {
	glfwDestroyWindow(m_Window);
}

} // namespace Aquila::Application

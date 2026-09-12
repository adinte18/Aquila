#include "Aquila/Application/Window.h"
#include "Aquila/Application/Events/InputEvent.h"
#include "Aquila/Application/Events/WindowEvent.h"
#include "GLFW/glfw3.h"

namespace Aquila::Application {

static bool s_GLFWInitialized = false;

static void glfw_error_callback(int error, const char *description) {
	AQUILA_LOG_ERROR("GLFW Error ({}): {}", error, description);
}

Window::Window(const Uint32 width, const Uint32 height, const std::string &title, bool maximized, bool start_hidden)
	: m_start_maximized(maximized), m_start_hidden(start_hidden) {
	m_data.title = title;
	m_data.width = width;
	m_data.height = height;
	m_data.owner = this;

	initialize();
}

Window::~Window() {
	shutdown();
}

void Window::initialize() {
	if (!s_GLFWInitialized) {
		glfwSetErrorCallback(glfw_error_callback);

		#ifdef AQUILA_PLATFORM_LINUX
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
		#endif

		const int success = glfwInit();
		AQUILA_ASSERT(success, "Could not initialize GLFW!");
		s_GLFWInitialized = true;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, m_start_hidden ? GLFW_FALSE : GLFW_TRUE);

	m_window = glfwCreateWindow(m_data.width, m_data.height, m_data.title.c_str(), nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, &m_data);
	if (m_start_maximized) {
		glfwMaximizeWindow(m_window);
	}
	setup_callbacks();

	// Sync dimensions after maximize — the WM_SIZE fires before SetupCallbacks so
	// the size callback never ran for it; query the actual size directly.
	int actual_w = 0;
	int actual_h = 0;
	glfwGetWindowSize(m_window, &actual_w, &actual_h);
	m_data.width = static_cast<Uint32>(actual_w);
	m_data.height = static_cast<Uint32>(actual_h);

	AQUILA_LOG_INFO("Window created: {}x{}", m_data.width, m_data.height);
}

void Window::set_title(const std::string &text) const {
	const auto title = std::string(m_data.title + " | " + text);
	glfwSetWindowTitle(m_window, title.c_str());
}
void Window::setup_callbacks() {
	glfwSetWindowSizeCallback(m_window, [](GLFWwindow *window, int width, int height) {
		WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		data.width = width;
		data.height = height;
		data.resized = true;

		Events::WindowResizeEvent event(width, height);
		event.set_source(data.owner);
		data.event_callback(event);
	});

	glfwSetWindowCloseCallback(m_window, [](GLFWwindow *window) {
		const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		Events::WindowCloseEvent event(static_cast<bool>(glfwWindowShouldClose(window)));
		event.set_source(data.owner);
		data.event_callback(event);
	});

	glfwSetKeyCallback(m_window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
		const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		switch (action) {
		case GLFW_PRESS: {
			Events::KeyPressedEvent event(static_cast<Events::KeyCode>(key), 0, mods);
			event.set_source(data.owner);

			data.event_callback(event);
			break;
		}
		case GLFW_RELEASE: {
			Events::KeyReleasedEvent event(static_cast<Events::KeyCode>(key));
			event.set_source(data.owner);

			data.event_callback(event);
			break;
		}
		case GLFW_REPEAT: {
			Events::KeyPressedEvent event(static_cast<Events::KeyCode>(key), 1, mods);
			event.set_source(data.owner);

			data.event_callback(event);
			break;
		}
		default:;
		}
	});

	glfwSetCharCallback(m_window, [](GLFWwindow *window, unsigned int keycode) {
		const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		Events::KeyTypedEvent event(static_cast<Events::KeyCode>(keycode));
		event.set_source(data.owner);

		data.event_callback(event);
	});

	glfwSetMouseButtonCallback(m_window, [](GLFWwindow *window, int button, int action, int mods) {
		const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));

		switch (action) {
		case GLFW_PRESS: {
			Events::MouseButtonPressedEvent event(static_cast<Events::MouseButton>(button));
			event.set_source(data.owner);

			data.event_callback(event);
			break;
		}
		case GLFW_RELEASE: {
			Events::MouseButtonReleasedEvent event(static_cast<Events::MouseButton>(button));
			event.set_source(data.owner);

			data.event_callback(event);
			break;
		}
		default:;
		}
	});

	glfwSetScrollCallback(m_window, [](GLFWwindow *window, const F64 x_offset, const F64 y_offset) {
		const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		Events::MouseScrolledEvent event(static_cast<F32>(x_offset), static_cast<F32>(y_offset));
		event.set_source(data.owner);

		data.event_callback(event);
	});

	// Store the latest position; PollEvents fires one synthesized event after draining the queue.
	glfwSetCursorPosCallback(m_window, [](GLFWwindow *window, const F64 x_pos, const F64 y_pos) {
		WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		data.last_mouse_x = x_pos;
		data.last_mouse_y = y_pos;
		data.has_pending_mouse_move = true;
	});

	glfwSetWindowFocusCallback(m_window, [](GLFWwindow *window, const int focused) {
		const WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		Events::WindowFocusEvent event(focused == GLFW_TRUE);
		event.set_source(data.owner);

		data.event_callback(event);
	});

	// On Windows, glfwPollEvents() is blocked inside Win32's modal resize loop while
	// the user drags a window border.  The refresh callback fires from inside that loop
	// whenever the window needs repainting (every mouse move during the drag), giving
	// us a chance to render a live frame.
	glfwSetWindowRefreshCallback(m_window, [](GLFWwindow *window) {
		WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
		if (data.refresh_callback) {
			data.refresh_callback();
		}
	});
}

void Window::flush_pending_events() {
	if (m_data.has_pending_mouse_move) {
		Events::MouseMovedEvent event(m_data.last_mouse_x, m_data.last_mouse_y);
		event.set_source(m_data.owner);

		m_data.event_callback(event);
		m_data.has_pending_mouse_move = false;
	}
}

void Window::poll_events() {
	glfwPollEvents();
}

void Window::wait_events() {
	glfwWaitEvents();
}

bool Window::should_close() const {
	return glfwWindowShouldClose(m_window) != 0;
}

static int to_glfw_cursor(Platform::CursorType type) {
	switch (type) {
	case Platform::CursorType::Text:
		return GLFW_IBEAM_CURSOR;
	case Platform::CursorType::Hand:
		return GLFW_POINTING_HAND_CURSOR;
	case Platform::CursorType::Crosshair:
		return GLFW_CROSSHAIR_CURSOR;
	case Platform::CursorType::ResizeHorizontal:
		return GLFW_RESIZE_EW_CURSOR;
	case Platform::CursorType::ResizeVertical:
		return GLFW_RESIZE_NS_CURSOR;
	case Platform::CursorType::ResizeDiagonalTLBR:
		return GLFW_RESIZE_NWSE_CURSOR;
	case Platform::CursorType::ResizeDiagonalBLTR:
		return GLFW_RESIZE_NESW_CURSOR;
	case Platform::CursorType::ResizeAll:
		return GLFW_RESIZE_ALL_CURSOR;
	case Platform::CursorType::NotAllowed:
		return GLFW_NOT_ALLOWED_CURSOR;
	default:
		return GLFW_ARROW_CURSOR;
	}
}

void Window::set_cursor(Platform::CursorType type) {
	if (type == m_current_cursor) {
		return;
	}
	m_current_cursor = type;

	const size_t index = static_cast<size_t>(type);
	if (m_cursors[index] == nullptr) {
		m_cursors[index] = glfwCreateStandardCursor(to_glfw_cursor(type));
	}
	glfwSetCursor(m_window, m_cursors[index]);
}

void Window::shutdown() const {
	for (GLFWcursor *cursor : m_cursors) {
		if (cursor != nullptr) {
			glfwDestroyCursor(cursor);
		}
	}
	glfwDestroyWindow(m_window);
}

} // namespace Aquila::Application

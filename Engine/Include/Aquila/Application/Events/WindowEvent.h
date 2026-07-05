#ifndef AQUILA_APPLICATION_EVENTS_H
#define AQUILA_APPLICATION_EVENTS_H

#include "Aquila/Application/Events/Event.h"

namespace Aquila::Application::Events {

class WindowResizeEvent final : public Event {
  public:
	WindowResizeEvent(const Uint32 width, const Uint32 height) : m_width(width), m_height(height) {}

	[[nodiscard]] Uint32 get_width() const { return m_width; }
	[[nodiscard]] Uint32 get_height() const { return m_height; }

	[[nodiscard]] std::string to_string() const override {
		return "WindowResizeEvent: " + std::to_string(m_width) + ", " + std::to_string(m_height);
	}

	EVENT_CLASS_TYPE(WindowResizeEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)

  private:
	Uint32 m_width, m_height;
};

class ViewportResizeEvent final : public Event {
  public:
	ViewportResizeEvent(const Uint32 width, const Uint32 height) : m_width(width), m_height(height) {}

	[[nodiscard]] Uint32 get_width() const { return m_width; }
	[[nodiscard]] Uint32 get_height() const { return m_height; }

	[[nodiscard]] std::string to_string() const override {
		return "ViewportResizeEvent: " + std::to_string(m_width) + ", " + std::to_string(m_width);
	}

	EVENT_CLASS_TYPE(ViewportResizeEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)

  private:
	Uint32 m_width, m_height;
};

class WindowCloseEvent final : public Event {
  public:
	WindowCloseEvent(const bool is_closed) : m_closed(is_closed) {}

	[[nodiscard]] bool is_closed() const { return m_closed; }

	EVENT_CLASS_TYPE(WindowCloseEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)

  private:
	bool m_closed;
};

class WindowFocusEvent final : public Event {
  public:
	explicit WindowFocusEvent(const bool focused) : m_focused(focused) {}

	[[nodiscard]] bool is_focused() const { return m_focused; }

	EVENT_CLASS_TYPE(WindowFocusEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)

  private:
	bool m_focused;
};

class AppTickEvent final : public Event {
  public:
	AppTickEvent() = default;

	EVENT_CLASS_TYPE(AppTickEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)
};

class AppUpdateEvent final : public Event {
  public:
	explicit AppUpdateEvent(const F32 delta_time) : m_delta_time(delta_time) {}

	[[nodiscard]] F32 get_delta_time() const { return m_delta_time; }

	EVENT_CLASS_TYPE(AppUpdateEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)

  private:
	F32 m_delta_time;
};

class AppRenderEvent final : public Event {
  public:
	AppRenderEvent() = default;

	EVENT_CLASS_TYPE(AppRenderEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)
};

} // namespace Aquila::Application::Events

#endif // AQUILA_APPLICATION_EVENTS_H

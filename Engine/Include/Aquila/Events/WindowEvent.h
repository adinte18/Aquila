#ifndef AQUILA_APPLICATION_EVENTS_H
#define AQUILA_APPLICATION_EVENTS_H

#include "Aquila/Events/Event.h"

namespace Aquila::Events {

class WindowResizeEvent final : public Event {
  public:
	WindowResizeEvent(const uint32 width, const uint32 height) : m_Width(width), m_Height(height) {}

	[[nodiscard]] uint32 GetWidth() const { return m_Width; }
	[[nodiscard]] uint32 GetHeight() const { return m_Height; }

	[[nodiscard]] std::string ToString() const override {
		return "WindowResizeEvent: " + std::to_string(m_Width) + ", " + std::to_string(m_Height);
	}

	EVENT_CLASS_TYPE(WindowResizeEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)

  private:
	uint32 m_Width, m_Height;
};

class ViewportResizeEvent final : public Event {
  public:
	ViewportResizeEvent(const uint32 width, const uint32 height) : m_Width(width), m_Height(height) {}

	[[nodiscard]] uint32 GetWidth() const { return m_Width; }
	[[nodiscard]] uint32 GetHeight() const { return m_Height; }

	[[nodiscard]] std::string ToString() const override {
		return "ViewportResizeEvent: " + std::to_string(m_Width) + ", " + std::to_string(m_Width);
	}

	EVENT_CLASS_TYPE(ViewportResizeEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)

  private:
	uint32 m_Width, m_Height;
};

class WindowCloseEvent final : public Event {
  public:
	WindowCloseEvent(const bool isClosed) : m_Closed(isClosed) {}

	[[nodiscard]] bool IsClosed() const { return m_Closed; }

	EVENT_CLASS_TYPE(WindowCloseEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)

  private:
	bool m_Closed;
};

class WindowFocusEvent final : public Event {
  public:
	explicit WindowFocusEvent(const bool focused) : m_Focused(focused) {}

	[[nodiscard]] bool IsFocused() const { return m_Focused; }

	EVENT_CLASS_TYPE(WindowFocusEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)

  private:
	bool m_Focused;
};

class AppTickEvent final : public Event {
  public:
	AppTickEvent() = default;

	EVENT_CLASS_TYPE(AppTickEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)
};

class AppUpdateEvent final : public Event {
  public:
	explicit AppUpdateEvent(const f32 deltaTime) : m_DeltaTime(deltaTime) {}

	[[nodiscard]] f32 GetDeltaTime() const { return m_DeltaTime; }

	EVENT_CLASS_TYPE(AppUpdateEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)

  private:
	f32 m_DeltaTime;
};

class AppRenderEvent final : public Event {
  public:
	AppRenderEvent() = default;

	EVENT_CLASS_TYPE(AppRenderEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Application)
};

} // namespace Aquila::Events

#endif // AQUILA_APPLICATION_EVENTS_H

#ifndef AQUILA_EVENT_H
#define AQUILA_EVENT_H
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Profiler.h"

namespace Aquila::Application {
class Window;
}

namespace Aquila::Application::Events {

enum class EventCategory : Uint8 {
	None = 0,
	Application = BIT(0),
	Input = BIT(1),
	Keyboard = BIT(2),
	Mouse = BIT(3),
	MouseButton = BIT(4),
	Scene = BIT(5),
	Renderer = BIT(6),
	AssetSelected = BIT(7)
};

inline EventCategory operator|(EventCategory a, EventCategory b) {
	return static_cast<EventCategory>(static_cast<int>(a) | static_cast<int>(b));
}
inline bool operator&(EventCategory a, EventCategory b) {
	return static_cast<int>(a) & static_cast<int>(b);
}

class Event {
  public:
	virtual ~Event() = default;
	[[nodiscard]] virtual const char *get_name() const = 0;
	[[nodiscard]] virtual EventCategory get_category() const = 0;
	[[nodiscard]] virtual std::type_index get_type_index() const = 0;
	[[nodiscard]] virtual std::string to_string() const { return get_name(); }
	[[nodiscard]] Window *get_source() const { return m_source; }

	void set_source(Window *window) { m_source = window; };
	bool handled = false;

  private:
	Window *m_source = nullptr;
};

class EventDispatcher {
  public:
	explicit EventDispatcher(Event &event) : m_event(event) {}

	template <typename T, typename F> bool dispatch(const F &func) {
		if (m_event.get_type_index() == std::type_index(typeid(T))) {
			m_event.handled = func(static_cast<T &>(m_event));
			return true;
		}
		return false;
	}

  private:
	Event &m_event;
};

} // namespace Aquila::Application::Events
#endif

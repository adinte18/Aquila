#ifndef AQUILA_EVENT_H
#define AQUILA_EVENT_H
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Profiler.h"

namespace Aquila::Application::Events {

enum class EventCategory : uint8 {
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
	[[nodiscard]] virtual const char *GetName() const = 0;
	[[nodiscard]] virtual EventCategory GetCategory() const = 0;
	[[nodiscard]] virtual std::type_index GetTypeIndex() const = 0;
	[[nodiscard]] virtual std::string ToString() const { return GetName(); }
	bool handled = false;
};

class EventDispatcher {
  public:
	explicit EventDispatcher(Event &event) : m_Event(event) {}

	template <typename T, typename F> bool Dispatch(const F &func) {
		if (m_Event.GetTypeIndex() == std::type_index(typeid(T))) {
			m_Event.handled = func(static_cast<T &>(m_Event));
			return true;
		}
		return false;
	}

  private:
	Event &m_Event;
};

} // namespace Aquila::Application::Events
#endif

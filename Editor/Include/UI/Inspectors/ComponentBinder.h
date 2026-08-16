#pragma once

#include "Aquila/Foundation/Signal.h"
#include "Aquila/Scene/Entity.h"
#include <functional>

namespace Editor {

template <typename SignalT> class BindHandle {
  public:
	explicit BindHandle(SignalT *signal) : m_signal(signal) {}

	template <typename Reaction> void on_change(Reaction reaction) { m_signal->connect(std::move(reaction)); }

  private:
	SignalT *m_signal;
};

template <typename Component> class ComponentBinder {
  public:
	explicit ComponentBinder(Aquila::SceneManagement::Entity entity, Signal<void()> Component::*changed = nullptr)
		: m_entity(entity), m_changed(changed) {}

	template <typename Widget, typename Getter, typename Setter>
	auto bind(Widget *widget, Getter getter, Setter setter) {
		widget->on_changed.set([entity = m_entity, setter, changed = m_changed](auto value) mutable {
			std::invoke(setter, entity.get_component<Component>(), value);
			if (changed != nullptr) {
				(entity.get_component<Component>().*changed)();
			}
		});
		widget->set_value(std::invoke(getter, m_entity.get_component<Component>()));
		return BindHandle(&widget->on_changed);
	}

	template <typename Widget, typename Projection>
	auto bind(Widget *widget, Projection project) {
		widget->on_changed.set([entity = m_entity, project, changed = m_changed](auto value) mutable {
			project(entity.get_component<Component>()) = value;
			if (changed != nullptr) {
				(entity.get_component<Component>().*changed)();
			}
		});
		widget->set_value(project(m_entity.get_component<Component>()));
		return BindHandle(&widget->on_changed);
	}

  private:
	Aquila::SceneManagement::Entity m_entity;
	Signal<void()> Component::*m_changed;
};

} // namespace Editor

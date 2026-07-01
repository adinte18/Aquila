#pragma once

#include "Aquila/Scene/Entity.h"
#include <functional>

namespace Editor {

template <typename Component> class ComponentBinder {
  public:
	explicit ComponentBinder(Aquila::SceneManagement::Entity entity) : m_Entity(entity) {}

	template <typename Widget, typename Getter, typename Setter>
	void Bind(Widget *widget, Getter getter, Setter setter) {
		widget->SetValue(std::invoke(getter, m_Entity.GetComponent<Component>()));
		widget->onChanged.Set([entity = m_Entity, setter](auto value) mutable {
			std::invoke(setter, entity.GetComponent<Component>(), value);
		});
	}

	template <typename Widget, typename Projection>
	void Bind(Widget *widget, Projection project) {
		widget->SetValue(project(m_Entity.GetComponent<Component>()));
		widget->onChanged.Set([entity = m_Entity, project](auto value) mutable {
			project(entity.GetComponent<Component>()) = value;
		});
	}

  private:
	Aquila::SceneManagement::Entity m_Entity;
};

} // namespace Editor

#pragma once

#include "Aquila/Scene/Entity.h"
#include <functional>

namespace Editor {

template <typename Component> class ComponentBinder {
  public:
	explicit ComponentBinder(Aquila::SceneManagement::Entity entity) : m_Entity(entity) {}

	template <typename Widget, typename Getter, typename Setter>
	void Bind(Widget *widget, Getter getter, Setter setter) {
		widget->onChanged.Set([entity = m_Entity, setter](auto value) mutable {
			std::invoke(setter, entity.GetComponent<Component>(), value);
		});
		widget->SetValue(std::invoke(getter, m_Entity.GetComponent<Component>()));
	}

	template <typename Widget, typename Projection>
	void Bind(Widget *widget, Projection project) {
		widget->onChanged.Set([entity = m_Entity, project](auto value) mutable {
			project(entity.GetComponent<Component>()) = value;
		});
		widget->SetValue(project(m_Entity.GetComponent<Component>()));
	}

  private:
	Aquila::SceneManagement::Entity m_Entity;
};

} // namespace Editor

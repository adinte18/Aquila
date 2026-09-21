#ifndef AQUILA_SCENE_COMPONENT_REGISTRY_H
#define AQUILA_SCENE_COMPONENT_REGISTRY_H

#include "Aquila/Foundation/Reflection/TypeInfo.h"
#include "Aquila/Foundation/Signal.h"
#include "Aquila/Scene/Entity.h"

#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Aquila::SceneManagement {

class ComponentDescriptor {
  public:
	explicit ComponentDescriptor(std::string name) : m_type(std::move(name)) {}

	[[nodiscard]] const std::string &get_name() const { return m_type.get_name(); }
	[[nodiscard]] const Reflection::TypeInfo &get_type() const { return m_type; }

	[[nodiscard]] bool has(Entity entity) const { return m_has(entity); }
	[[nodiscard]] void *get(Entity entity) const { return m_get(entity); }
	[[nodiscard]] Signal<void()> *get_changed_signal(Entity entity) const { return m_get_changed(entity); }

  private:
	friend class ComponentRegistry;

	Reflection::TypeInfo m_type;
	Delegate<bool(Entity)> m_has;
	Delegate<void *(Entity)> m_get;
	Delegate<Signal<void()> *(Entity)> m_get_changed;
};

class ComponentRegistry {
  public:
	[[nodiscard]] static ComponentRegistry &instance();

	template <typename T> Reflection::TypeBuilder<T> register_component(std::string name) {
		auto descriptor = std::make_unique<ComponentDescriptor>(std::move(name));
		descriptor->m_has = [](Entity entity) { return entity.has_component<T>(); };
		descriptor->m_get = [](Entity entity) -> void * { return entity.try_get_component<T>(); };
		descriptor->m_get_changed = [](Entity entity) -> Signal<void()> * {
			if constexpr (requires(T &component) {
							  { component.on_changed } -> std::convertible_to<Signal<void()> &>;
						  }) {
				T *component = entity.try_get_component<T>();
				return component != nullptr ? &component->on_changed : nullptr;
			} else {
				return nullptr;
			}
		};

		Reflection::TypeInfo &type = descriptor->m_type;
		m_descriptors.push_back(std::move(descriptor));
		return Reflection::TypeBuilder<T>(type);
	}

	[[nodiscard]] const ComponentDescriptor *find(std::string_view name) const;
	[[nodiscard]] const std::vector<Unique<ComponentDescriptor>> &get_all() const { return m_descriptors; }

  private:
	ComponentRegistry();

	std::vector<Unique<ComponentDescriptor>> m_descriptors;
};

}

#endif

#pragma once

#include "Aquila/Scene/Entity.h"

namespace Aquila::UI::Core {
class Collapsible;
class PropertyGrid;
}

namespace Editor {

class IComponentUI {
  public:
	virtual ~IComponentUI() = default;
	virtual bool matches(Aquila::SceneManagement::Entity entity) const = 0;
	virtual void build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) = 0;
	virtual void show(Aquila::SceneManagement::Entity entity) = 0;
	virtual void refresh() {}
	virtual bool is_removable() const { return false; }
	virtual void remove(Aquila::SceneManagement::Entity entity) {}

	Delegate<void(Aquila::SceneManagement::Entity)> on_edited;
};

} // namespace Editor

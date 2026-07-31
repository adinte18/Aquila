#pragma once

#include "Aquila/Foundation/Signal.h"
#include "Aquila/Scene/Entity.h"

#include <vector>

namespace Aquila::UI::Core {
class Collapsible;
class PropertyGrid;
}

namespace Editor {

struct ComponentSignal {
	const char *name;
	Signal<void()> *signal;
};

class IComponentUI {
  public:
	virtual ~IComponentUI() = default;
	virtual bool matches(Aquila::SceneManagement::Entity entity) const = 0;
	virtual void build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) = 0;
	virtual void show(Aquila::SceneManagement::Entity entity) = 0;
	virtual std::vector<ComponentSignal> signals(Aquila::SceneManagement::Entity entity) const { return {}; }
};

} // namespace Editor

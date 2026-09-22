#include "UI/Inspectors/ReflectedComponentUI.h"

#include "Aquila/Scene/ComponentRegistry.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;

ReflectedComponentUI::ReflectedComponentUI(const ComponentDescriptor &descriptor, GFX::GfxContext *context)
	: m_descriptor(descriptor), m_context(context) {}

bool ReflectedComponentUI::matches(Entity entity) const {
	return m_descriptor.has(entity);
}

void ReflectedComponentUI::build(UI::Core::Collapsible *, UI::Core::PropertyGrid *grid) {
	m_properties = std::make_unique<UI::Core::ReflectedPropertyGrid>(*grid, m_descriptor.get_type(), m_context);
}

void ReflectedComponentUI::show(Entity entity) {
	const ComponentDescriptor &descriptor = m_descriptor;
	m_properties->bind(descriptor.get(entity), [&descriptor, entity] {
		if (Signal<void()> *changed = descriptor.get_changed_signal(entity)) {
			(*changed)();
		}
	});
}

std::vector<ComponentSignal> ReflectedComponentUI::signals(Entity entity) const {
	if (Signal<void()> *changed = m_descriptor.get_changed_signal(entity)) {
		return { { .name = "changed", .signal = changed } };
	}
	return {};
}

}

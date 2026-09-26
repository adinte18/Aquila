#include "UI/Inspectors/MaterialComponentUI.h"

#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/UI/Widgets/AssetSlot.h"
#include "Aquila/UI/Widgets/Collapsible.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

MaterialComponentUI::MaterialComponentUI(const ComponentDescriptor &descriptor, GFX::GfxContext &context)
	: ReflectedComponentUI(descriptor, &context) {}

void MaterialComponentUI::build(UI::Core::Collapsible *section, UI::Core::PropertyGrid *grid) {
	ReflectedComponentUI::build(section, grid);
	m_texture_area = section->add_child<UI::Core::PropertyGrid>();
	m_texture_area->set_split(grid->is_split());
}

void MaterialComponentUI::show(Entity entity) {
	ReflectedComponentUI::show(entity);

	while (!m_texture_area->get_children().empty()) {
		m_texture_area->remove_child(m_texture_area->get_children().front().get());
	}

	auto &mat = entity.get_component<MaterialComponent>();
	if (mat.material) {
		for (const auto &param : mat.material->get_parameters()) {
			if (param.type != Graphics::ParameterType::Texture2D) {
				continue;
			}
			m_texture_area->add_row<UI::Core::AssetSlot>(param.name, "Texture2D");
		}
	}
}

} // namespace Editor

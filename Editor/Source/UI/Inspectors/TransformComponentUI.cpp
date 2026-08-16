#include "UI/Inspectors/TransformComponentUI.h"

#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/VecField.h"
#include "UI/Inspectors/ComponentBinder.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

bool TransformComponentUI::matches(Entity entity) const {
	return entity.has_component<TransformComponent>();
}

void TransformComponentUI::build(UI::Core::Collapsible *, UI::Core::PropertyGrid *grid) {
	m_position = grid->add_row<UI::Core::Vec3Field>("Position");
	m_position->set_speed(0.1F);

	m_scale = grid->add_row<UI::Core::Vec3Field>("Scale");
	m_scale->set_speed(0.1F);
}

void TransformComponentUI::show(Entity entity) {
	ComponentBinder<TransformComponent> bind(entity, &TransformComponent::on_changed);
	bind.bind(m_position, &TransformComponent::get_local_position, &TransformComponent::set_local_position);
	bind.bind(m_scale, &TransformComponent::get_local_scale, &TransformComponent::set_local_scale);
}

std::vector<ComponentSignal> TransformComponentUI::signals(Entity entity) const {
	return { { "changed", &entity.get_component<TransformComponent>().on_changed } };
}

} // namespace Editor

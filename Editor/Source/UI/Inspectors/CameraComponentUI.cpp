#include "UI/Inspectors/CameraComponentUI.h"

#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/UI/Widgets/Checkbox.h"
#include "Aquila/UI/Widgets/DragFloat.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/Toggle.h"
#include "UI/Inspectors/ComponentBinder.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

bool CameraComponentUI::matches(Entity entity) const {
	return entity.has_component<CameraComponent>();
}

void CameraComponentUI::build(UI::Core::Collapsible *, UI::Core::PropertyGrid *grid) {
	using UI::Core::DragFloat;
	m_fov =
		grid->add_row<DragFloat>("FOV", DragFloat::Config{ .min = 1.F, .max = 179.F, .speed = 0.5f, .precision = 1 });
	m_near = grid->add_row<DragFloat>("Near",
									  DragFloat::Config{ .min = 0.001f, .max = 100.F, .speed = 0.01f, .precision = 3 });
	m_far = grid->add_row<DragFloat>("Far",
									 DragFloat::Config{ .min = 0.1f, .max = 10000.F, .speed = 0.01f, .precision = 1 });
	m_primary = grid->add_row<UI::Core::Toggle>("Primary", false);
	m_ortho = grid->add_row<UI::Core::Checkbox>("Orthographic", false);
}

void CameraComponentUI::show(Entity entity) {
	ComponentBinder<CameraComponent> bind(entity, &CameraComponent::on_changed);
	bind.bind(m_fov, [](auto &c) -> float & { return c.fov; });
	bind.bind(m_near, [](auto &c) -> float & { return c.near_plane; });
	bind.bind(m_far, [](auto &c) -> float & { return c.far_plane; });
	bind.bind(m_primary, [](auto &c) -> bool & { return c.primary; });
	bind.bind(m_ortho, [](auto &c) -> bool & { return c.is_orthographic; });
}

std::vector<ComponentSignal> CameraComponentUI::signals(Entity entity) const {
	return { { "changed", &entity.get_component<CameraComponent>().on_changed } };
}

} // namespace Editor

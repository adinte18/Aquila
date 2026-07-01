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

bool CameraComponentUI::Matches(Entity entity) const {
	return entity.HasComponent<CameraComponent>();
}

void CameraComponentUI::Build(UI::Core::Collapsible *, UI::Core::PropertyGrid *grid) {
	using UI::Core::DragFloat;
	m_Fov = grid->AddRow<DragFloat>("FOV", DragFloat::Config{ .min = 1.f, .max = 179.f, .speed = 0.5f, .precision = 1 });
	m_Near = grid->AddRow<DragFloat>("Near", DragFloat::Config{ .min = 0.001f, .max = 100.f, .speed = 0.01f, .precision = 3 });
	m_Far = grid->AddRow<DragFloat>("Far", DragFloat::Config{ .min = 0.1f, .max = 10000.f, .speed = 0.01f, .precision = 1 });
	m_Primary = grid->AddRow<UI::Core::Toggle>("Primary", false);
	m_Ortho = grid->AddRow<UI::Core::Checkbox>("Orthographic", false);
}

void CameraComponentUI::Show(Entity entity) {
	ComponentBinder<CameraComponent> bind(entity);
	bind.Bind(m_Fov, [](auto &c) -> float & { return c.fov; });
	bind.Bind(m_Near, [](auto &c) -> float & { return c.nearPlane; });
	bind.Bind(m_Far, [](auto &c) -> float & { return c.farPlane; });
	bind.Bind(m_Primary, [](auto &c) -> bool & { return c.primary; });
	bind.Bind(m_Ortho, [](auto &c) -> bool & { return c.isOrthographic; });
}

} // namespace Editor

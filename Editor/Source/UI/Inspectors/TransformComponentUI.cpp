#include "UI/Inspectors/TransformComponentUI.h"

#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/VecField.h"
#include "UI/Inspectors/ComponentBinder.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

bool TransformComponentUI::Matches(Entity entity) const {
	return entity.HasComponent<TransformComponent>();
}

void TransformComponentUI::Build(UI::Core::Collapsible *, UI::Core::PropertyGrid *grid) {
	m_Position = grid->AddRow<UI::Core::Vec3Field>("Position");
	m_Position->SetSpeed(0.1f);

	m_Scale = grid->AddRow<UI::Core::Vec3Field>("Scale");
	m_Scale->SetSpeed(0.1f);
}

void TransformComponentUI::Show(Entity entity) {
	ComponentBinder<TransformComponent> bind(entity);
	bind.Bind(m_Position, &TransformComponent::GetLocalPosition, &TransformComponent::SetLocalPosition);
	bind.Bind(m_Scale, &TransformComponent::GetLocalScale, &TransformComponent::SetLocalScale);
}

} // namespace Editor

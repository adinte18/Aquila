#ifndef EDITORLAYER_H
#define EDITORLAYER_H

#include "Aquila/Core/Application.h"
#include "Aquila/Core/Defines.h"
#include "Aquila/Core/Layer.h"
#include "Aquila/Events/Event.h"
#include "Aquila/Events/InputEvent.h"
#include "Aquila/Scene/Entity.h"
#include "UI/UIConfig.h"

namespace Engine {
class Application;
}

namespace Editor {

enum class GizmoOperation : uint8 { Translate, Rotate, Scale };

class EditorLayer final : public Aquila::Core::Layer {
  public:
	EditorLayer(Aquila::Core::Application &app);
	~EditorLayer() override;

	AQUILA_NONCOPYABLE(EditorLayer);
	AQUILA_NONMOVEABLE(EditorLayer);

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(f32 deltaTime) override;
	void OnImGuiRender() override;
	void OnEvent(Aquila::Events::Event &event) override;

  private:
	void RenderDockspace() const;

	// Event handlers
	bool OnKeyPressed(const Aquila::Events::KeyPressedEvent &event);
	bool OnMouseButtonPressed(const Aquila::Events::MouseButtonPressedEvent &event);

	Aquila::Core::Application &m_App;

	Aquila::SceneManagement::Entity m_SelectedEntity;
	GizmoOperation m_GizmoOperation;
	bool m_ViewportFocused;
	bool m_ViewportHovered;
};

} // namespace Editor
#endif // EDITORLAYER_H

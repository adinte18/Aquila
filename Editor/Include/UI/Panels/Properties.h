#ifndef PROPERTIES_PANEL_H
#define PROPERTIES_PANEL_H

#include "Aquila/Core/Layer.h"
#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/Scene/Scene.h"

namespace Aquila::Core {
class Application;
}

namespace Editor {

class PropertiesPanel final : public Aquila::Core::Layer {
  public:
	PropertiesPanel(Aquila::Core::Application &app);
	~PropertiesPanel() override = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(f32 deltaTime) override;
	void DrawComponent_SkyLight(Aquila::SceneManagement::Entity entity);
	void OnImGuiRender() override;
	void OnEvent(Aquila::Events::Event &event) override;

	Aquila::SceneManagement::Entity GetSelectedEntity() const { return m_SelectedEntity; }
	void SetSelectedEntity(Aquila::SceneManagement::Entity entity) { m_SelectedEntity = entity; }

  private:
	struct ComponentMenuAction {
		enum Type { RESET, REMOVE_COMPONENT, CUSTOM };

		Type type;
		const char *icon;
		const char *label;
		std::function<void()> callback;

		ComponentMenuAction(Type t, const char *i, const char *l, std::function<void()> c = nullptr)
			: type(t), icon(i), label(l), callback(c) {}
	};

	// Component drawing methods
	void DrawComponent_Transform(Aquila::SceneManagement::Entity entity);
	void DrawComponent_Metadata(Aquila::SceneManagement::Entity entity);
	void DrawComponent_Mesh(Aquila::SceneManagement::Entity entity);
	void DrawComponent_Camera(Aquila::SceneManagement::Entity entity);
	void DrawComponent_Light(Aquila::SceneManagement::Entity entity);
	void DrawComponent_Material(Aquila::SceneManagement::Entity entity);

	void DrawAsset_Texture();
	void DrawAsset_Material();
	void DrawAsset_Shader();
	void DrawAsset_Mesh();

	// Helper methods
	bool DrawComponentHeader(const char *icon, const char *label, const char *menuId,
							 const std::vector<ComponentMenuAction> &menuActions = {});
	void DrawInlineMaterialProperties(Ref<Aquila::Graphics::Material::Material> material, uint32 slotIndex);
	void DrawAddComponentMenu(Aquila::SceneManagement::Entity entity);
	void UpdateChildrenTransforms(Aquila::SceneManagement::Entity parentEntity);

	Aquila::Core::Application &m_App;
	Aquila::SceneManagement::Entity m_SelectedEntity = Aquila::SceneManagement::Entity::Null();
	std::string m_SelectedAssetPath = "";
	std::string m_SelectedAssetExtension = "";

	bool m_ShowCreateMaterialFromShaderPopup = false;
	char m_CreateMatNameBuffer[256] = "";
	std::string m_CreateMatShaderBasePath = "";
};

} // namespace Editor

#endif // PROPERTIES_PANEL_H

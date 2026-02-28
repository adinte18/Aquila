#pragma once

#include "Aquila/Core/Layer.h"
#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Graphics/Material/MaterialLibrary.h"
#include "Aquila/Scene/Entity.h"
#include "UI/Panels/MaterialPreviewSystem.h"

namespace Engine {
class Application;
}

namespace Editor {

class MaterialEditorPanel : public Aquila::Core::Layer {
  public:
	MaterialEditorPanel(Aquila::Core::Application &app);
	~MaterialEditorPanel() override;

	bool IsOpen() const { return m_IsOpen; }
	void SetOpen(bool open) { m_IsOpen = open; }
	void ToggleOpen() { m_IsOpen = !m_IsOpen; }

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(f32 deltaTime) override;
	void OnImGuiRender() override;
	void RenderToolbar();
	void RenderCreateMaterialDialog();
	void RenderPropertiesPanel();
	void RenderPreviewPanel();
	void OnEvent(Aquila::Events::Event &event) override;
	void OnRender() override;

	void SetSelectedMaterial(Ref<Aquila::Graphics::Material::Material> material);

  private:
	void RenderMenuBar();
	void RenderDetailsPanel();
	void RenderMaterialList();
	void RenderMaterialProperties();
	void RenderTextureSlots();
	void RenderMaterialPreview();
	void RenderStatsPanel();
	void RenderShaderGraph();

	void RenderParameterSection();
	void RenderRenderStateSection();
	void RenderTemplateSelector();

	void RenderColorProperty(const std::string &name, Aquila::Graphics::Material::MaterialParameter &param);
	void RenderFloatProperty(const std::string &name, Aquila::Graphics::Material::MaterialParameter &param);
	void RenderTextureProperty(const std::string &name, Aquila::Graphics::Material::MaterialParameter &param);

	bool IsPropertyOverridden(const std::string &propName);

	void ChangePreviewMesh(PreviewMesh mesh);
	void CreateNewMaterial();
	void DuplicateMaterial();
	void DeleteMaterial();
	void SaveMaterial();
	void LoadMaterial();

	void RefreshMaterialList();

	const char *GetMaterialIcon(const std::string &templateName);
	const char *GetPreviewMeshName(PreviewMesh mesh);

	Aquila::Core::Application &m_App;

	Ref<Aquila::Graphics::Material::Material> m_SelectedMaterial = nullptr;
	std::vector<Ref<Aquila::Graphics::Material::Material>> m_Materials;
	std::unordered_map<std::string, Ref<Aquila::Graphics::Material::MaterialTemplate>> m_AvailableTemplates;

	MaterialPreviewSystem m_PreviewSystem;
	bool m_PreviewNeedsUpdate = false;

	PreviewMesh m_CurrentPreviewMesh = PreviewMesh::Sphere;
	bool m_AutoRotate = true;
	f32 m_PreviewRotation = 0.0f;
	f32 m_RotationSpeed = 45.0f;

	std::string m_SearchFilter = "";
	char m_NewMaterialName[256] = { 0 };
	int m_SelectedTemplate = 0;

	bool m_ShowCreateDialog = false;
	bool m_IsOpen = false;
	bool m_ShowAdvancedOptions = false;
	bool m_ShowShaderGraph = false;
	bool m_ShowStats = true;
	bool m_ShowBottomPanel = false;

	f32 m_LeftPanelWidth = 250.0f;
	f32 m_RightPanelWidth = 350.0f;
	f32 m_BottomPanelHeight = 300.0f;
	bool m_IsFirstMouse;
};

} // namespace Editor

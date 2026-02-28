#include "UI/Panels/MaterialEditor.h"
#include "Aquila/Core/Application.h"
#include "Aquila/Graphics/Core/Renderer.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "UI/Managers/FontManager.h"
#include "lucide.h"

namespace Editor {
//TODO : THIS IS NOT FUNCTIONAL!!!
MaterialEditorPanel::MaterialEditorPanel(Aquila::Core::Application &app) : Layer("MaterialEditor"), m_App(app) {
	AQUILA_LOG_INFO("MaterialEditorPanel created");
}

MaterialEditorPanel::~MaterialEditorPanel() {
	if (m_PreviewSystem.IsInitialized()) {
		m_PreviewSystem.Shutdown();
	}
	AQUILA_LOG_INFO("MaterialEditorPanel destroyed");
}

void MaterialEditorPanel::OnAttach() {
	// AQUILA_LOG_INFO("MaterialEditorPanel attached");
	// m_IsOpen = true;
	// AQUILA_LOG_INFO("Material Editor forced open for testing");

	// auto *materialSystem = m_App.GetRenderer().GetMaterialSystem();
	// if (!materialSystem) {
	// 	AQUILA_LOG_ERROR("MaterialSystem is null in MaterialEditorPanel::OnAttach");
	// 	return;
	// }

	// auto &library = materialSystem->GetLibrary();
	// m_AvailableTemplates = library.GetAllTemplates();
	// m_PreviewSystem.Initialize(m_App);
	// RefreshMaterialList();
}

void MaterialEditorPanel::OnDetach() {
	m_SelectedMaterial = nullptr;
	m_PreviewSystem.Shutdown();
	AQUILA_LOG_INFO("MaterialEditorPanel detached");
}

void MaterialEditorPanel::OnUpdate(f32 deltaTime) {
	m_PreviewSystem.SetAutoRotate(m_AutoRotate);
	m_PreviewSystem.SetRotationSpeed(m_RotationSpeed);

	// Let the preview system handle all rotation logic
	m_PreviewSystem.Update(deltaTime);

	// Sync the rotation value back for the UI slider
	m_PreviewRotation = m_PreviewSystem.GetRotation();
}

void MaterialEditorPanel::OnImGuiRender() {
	if (!m_IsOpen)
		return;

	ImGui::SetNextWindowSize(ImVec2(1600, 900), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));

	if (!ImGui::Begin("Material Editor", &m_IsOpen)) {
		ImGui::PopStyleVar();
		ImGui::End();
		return;
	}
	ImGui::PopStyleVar();

	RenderToolbar();
	RenderCreateMaterialDialog();

	ImGui::Dummy(ImVec2(0, 8));

	ImVec2 contentSize = ImGui::GetContentRegionAvail();
	f32 leftPanelWidth = 320.0f;
	f32 rightPanelWidth = 380.0f;
	f32 spacing = 8.0f;
	f32 centerWidth = contentSize.x - leftPanelWidth - rightPanelWidth - (spacing * 2);

	if (centerWidth < 300.0f) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.0f, 1.0f));
		ImGui::TextWrapped("Window is too narrow. Please resize the Material Editor window to see all panels.");
		ImGui::PopStyleColor();
		ImGui::End();
		return;
	}

	ImGui::BeginChild("##LeftPanel", ImVec2(leftPanelWidth, contentSize.y), true);
	RenderPropertiesPanel();
	ImGui::EndChild();

	ImGui::SameLine(0, spacing);

	ImGui::BeginChild("##CenterPanel", ImVec2(centerWidth, contentSize.y), false);
	f32 previewHeight = m_ShowShaderGraph ? contentSize.y * 0.55f : contentSize.y;
	ImGui::BeginChild("##PreviewSection", ImVec2(0, previewHeight), true);
	RenderPreviewPanel();
	ImGui::EndChild();

	if (m_ShowShaderGraph) {
		ImGui::Spacing();
		ImGui::BeginChild("##ShaderGraphSection", ImVec2(0, 0), true);
		RenderShaderGraph();
		ImGui::EndChild();
	}
	ImGui::EndChild();

	ImGui::SameLine(0, spacing);

	ImGui::BeginChild("##RightPanel", ImVec2(rightPanelWidth, contentSize.y), true);
	RenderDetailsPanel();
	ImGui::EndChild();

	ImGui::End();
}

void MaterialEditorPanel::RenderToolbar() {
	// Create a custom toolbar as a child window instead of menu bar
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.17f, 1.0f));

	// Calculate toolbar height based on button size
	f32 toolbarHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;

	ImGui::BeginChild("##Toolbar", ImVec2(0, toolbarHeight), true, ImGuiWindowFlags_NoScrollbar);

	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (toolbarHeight - ImGui::GetFrameHeight()) * 0.5f -
						 ImGui::GetStyle().WindowPadding.y);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 6));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.27f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.37f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.42f, 1.0f));

	std::string currentMatName = m_SelectedMaterial ? m_SelectedMaterial->name : "Select Material";
	if (ImGui::Button((ICON_LC_PALETTE " " + currentMatName).c_str())) {
		ImGui::OpenPopup("MaterialSelector");
	}

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();

	if (ImGui::BeginPopup("MaterialSelector")) {
		RenderMaterialList();
		ImGui::EndPopup();
	}

	ImGui::SameLine();
	ImGui::Dummy(ImVec2(12, 0));
	ImGui::SameLine();

	// Vertical separator
	ImVec2 cursorPos = ImGui::GetCursorScreenPos();
	ImDrawList *drawList = ImGui::GetWindowDrawList();
	f32 separatorHeight = ImGui::GetFrameHeight();
	drawList->AddLine(ImVec2(cursorPos.x, cursorPos.y + 4), ImVec2(cursorPos.x, cursorPos.y + separatorHeight - 4),
					  IM_COL32(80, 80, 85, 255), 1.0f);

	ImGui::SameLine();
	ImGui::Dummy(ImVec2(12, 0));
	ImGui::SameLine();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));

	if (ImGui::Button(ICON_LC_PLUS " New")) {
		m_ShowCreateDialog = true;
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Create New Material (Ctrl+N)");

	ImGui::SameLine();
	if (ImGui::Button(ICON_LC_COPY " Duplicate")) {
		DuplicateMaterial();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Duplicate Material (Ctrl+D)");

	ImGui::SameLine();
	if (ImGui::Button(ICON_LC_SAVE " Save")) {
		SaveMaterial();
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Save Material (Ctrl+S)");

	ImGui::PopStyleVar();

	ImGui::SameLine();
	ImGui::Dummy(ImVec2(12, 0));
	ImGui::SameLine();

	// Another separator
	cursorPos = ImGui::GetCursorScreenPos();
	drawList->AddLine(ImVec2(cursorPos.x, cursorPos.y + 4), ImVec2(cursorPos.x, cursorPos.y + separatorHeight - 4),
					  IM_COL32(80, 80, 85, 255), 1.0f);

	ImGui::SameLine();
	ImGui::Dummy(ImVec2(12, 0));
	ImGui::SameLine();

	// View menu
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.32f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.37f, 1.0f));

	if (ImGui::Button("View")) {
		ImGui::OpenPopup("ViewMenu");
	}

	if (ImGui::BeginPopup("ViewMenu")) {
		ImGui::MenuItem("Shader Graph", nullptr, &m_ShowShaderGraph);
		ImGui::MenuItem("Statistics", nullptr, &m_ShowStats);
		ImGui::MenuItem("Advanced", nullptr, &m_ShowAdvancedOptions);
		ImGui::EndPopup();
	}

	ImGui::SameLine();
	if (ImGui::Button("Preview")) {
		ImGui::OpenPopup("PreviewMenu");
	}
	if (ImGui::BeginPopup("PreviewMenu")) {
		if (ImGui::MenuItem("Sphere", nullptr, m_CurrentPreviewMesh == PreviewMesh::Sphere)) {
			ChangePreviewMesh(PreviewMesh::Sphere);
		}
		if (ImGui::MenuItem("Cube", nullptr, m_CurrentPreviewMesh == PreviewMesh::Cube)) {
			ChangePreviewMesh(PreviewMesh::Cube);
		}
		if (ImGui::MenuItem("Cylinder", nullptr, m_CurrentPreviewMesh == PreviewMesh::Cylinder)) {
			ChangePreviewMesh(PreviewMesh::Cylinder);
		}
		if (ImGui::MenuItem("Plane", nullptr, m_CurrentPreviewMesh == PreviewMesh::Plane)) {
			ChangePreviewMesh(PreviewMesh::Plane);
		}
		ImGui::Separator();
		ImGui::MenuItem("Auto Rotate", nullptr, &m_AutoRotate);
		ImGui::EndPopup();
	}

	ImGui::PopStyleColor(3);

	ImGui::EndChild();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}

void MaterialEditorPanel::RenderCreateMaterialDialog() {
	// if (m_ShowCreateDialog) {
	// 	ImGui::OpenPopup("CreateMaterialDialog");
	// 	m_ShowCreateDialog = false;
	// }

	// if (ImGui::BeginPopupModal("CreateMaterialDialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
	// 	ImGui::Text("Create New Material");
	// 	ImGui::Separator();
	// 	ImGui::Spacing();

	// 	static char nameBuffer[256] = "NewMaterial";
	// 	static int selectedTemplate = 0;

	// 	ImGui::Text("Name:");
	// 	ImGui::SetNextItemWidth(300);
	// 	ImGui::InputText("##MaterialName", nameBuffer, 256);

	// 	ImGui::Spacing();
	// 	ImGui::Text("Template:");
	// 	ImGui::SetNextItemWidth(300);

	// 	std::vector<std::string> templateNamesList;
	// 	std::vector<const char *> templateNamesCStr;
	// 	for (const auto &[name, tmpl] : m_AvailableTemplates) {
	// 		templateNamesList.push_back(name);
	// 	}
	// 	for (const auto &name : templateNamesList) {
	// 		templateNamesCStr.push_back(name.c_str());
	// 	}
	// 	if (!templateNamesCStr.empty()) {
	// 		ImGui::Combo("##Template", &selectedTemplate, templateNamesCStr.data(), templateNamesCStr.size());
	// 	}

	// 	ImGui::Spacing();
	// 	ImGui::Separator();
	// 	ImGui::Spacing();

	// 	if (ImGui::Button("Create", ImVec2(120, 0))) {
	// 		if (strlen(nameBuffer) > 0 && selectedTemplate < templateNamesList.size()) {
	// 			auto *materialSystem = m_App.GetRenderer().GetMaterialSystem();
	// 			if (materialSystem) {
	// 				auto newMat =
	// 					materialSystem->GetLibrary().CreateMaterial(nameBuffer, templateNamesList[selectedTemplate]);
	// 				SetSelectedMaterial(newMat);
	// 				RefreshMaterialList();
	// 				AQUILA_LOG_INFO("Created material: {}", nameBuffer);
	// 			}
	// 		}
	// 		ImGui::CloseCurrentPopup();
	// 	}

	// 	ImGui::SameLine();
	// 	if (ImGui::Button("Cancel", ImVec2(120, 0))) {
	// 		ImGui::CloseCurrentPopup();
	// 	}

	// 	ImGui::EndPopup();
	// }
}

void MaterialEditorPanel::RenderPropertiesPanel() {
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

	ImGui::Spacing();
	ImGui::Indent(12);

	ImGui::PushFont(UI::FontManager::Get().GetFont18());
	ImGui::Text(ICON_LC_SLIDERS_HORIZONTAL " PROPERTIES");
	ImGui::PopFont();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (!m_SelectedMaterial) {
		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::TextWrapped("No material selected. Choose a material from the dropdown above or create a new one.");
		ImGui::PopStyleColor();
		ImGui::Unindent(12);
		ImGui::PopStyleVar(2);
		return;
	}

	ImGui::PushFont(UI::FontManager::Get().GetFont16());
	ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", m_SelectedMaterial->name.c_str());
	ImGui::PopFont();

	auto tmpl = m_SelectedMaterial->GetTemplate();
	if (tmpl) {
		ImGui::Text("Template: %s", tmpl->name.c_str());
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::CollapsingHeader(ICON_LC_COG " Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Indent(8);
		ImGui::Spacing();

		auto properties = m_SelectedMaterial->GetAllProperties();

		for (auto &[propName, param] : properties) {
			if (!param.m_IsEditable)
				continue;

			ImGui::PushID(propName.c_str());

			switch (param.m_Type) {
			case Aquila::Graphics::Material::ParameterType::Float:
				RenderFloatProperty(propName, param);
				break;
			case Aquila::Graphics::Material::ParameterType::Color:
				RenderColorProperty(propName, param);
				break;
			default:
				break;
			}

			if (IsPropertyOverridden(propName)) {
				ImGui::SameLine();
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.2f, 0.6f));
				if (ImGui::SmallButton(ICON_LC_ROTATE_CCW)) {
					m_SelectedMaterial->ReSetParameter(propName);
					m_PreviewNeedsUpdate = true;
				}
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Reset to default");
				}
			}

			ImGui::PopID();
		}

		ImGui::Spacing();
		if (ImGui::Button(ICON_LC_REFRESH_CW " Reset All", ImVec2(-1, 0))) {
			m_SelectedMaterial->ResetAllProperties();
			m_PreviewNeedsUpdate = true;
		}

		ImGui::Unindent(8);
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	RenderRenderStateSection();

	ImGui::Unindent(12);
	ImGui::PopStyleVar(2);
}

void MaterialEditorPanel::RenderPreviewPanel() {
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

	ImGui::Spacing();
	ImGui::Indent(12);

	ImGui::PushFont(UI::FontManager::Get().GetFont18());
	ImGui::Text(ICON_LC_EYE " PREVIEW");
	ImGui::PopFont();

	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x - 280);

	ImGui::SetNextItemWidth(120);
	const char *meshNames[] = { "Sphere", "Cube", "Cylinder", "Plane" };
	int currentMesh = static_cast<int>(m_CurrentPreviewMesh);
	if (ImGui::Combo("##Mesh", &currentMesh, meshNames, 4)) {
		ChangePreviewMesh(static_cast<PreviewMesh>(currentMesh));
	}

	ImGui::SameLine();
	if (ImGui::Checkbox("Auto Rotate", &m_AutoRotate)) {
		if (!m_AutoRotate) {
			m_PreviewSystem.SetRotation(m_PreviewRotation);
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImVec2 previewSize = ImGui::GetContentRegionAvail();
	previewSize.x -= 24;
	previewSize.y -= 12;

	f32 aspect = previewSize.x / previewSize.y;
	if (aspect > 1.0f) {
		previewSize.x = previewSize.y;
	} else {
		previewSize.y = previewSize.x;
	}

	ImVec2 cursorPos = ImGui::GetCursorPos();
	ImVec2 windowSize = ImGui::GetContentRegionAvail();
	ImGui::SetCursorPos(ImVec2(cursorPos.x + (windowSize.x - previewSize.x) * 0.5f, cursorPos.y));

	static ImVec2 lastPreviewSize = { 0, 0 };
	if (previewSize.x != lastPreviewSize.x || previewSize.y != lastPreviewSize.y) {
		m_PreviewSystem.Resize(static_cast<uint32>(previewSize.x), static_cast<uint32>(previewSize.y));
		lastPreviewSize = previewSize;
	}

	VkDescriptorSet previewTexture = m_PreviewSystem.GetPreviewTexture();

	if (previewTexture != VK_NULL_HANDLE) {
		ImTextureID texId = reinterpret_cast<ImTextureID>(previewTexture);
		ImVec2 screenPos = ImGui::GetCursorScreenPos();
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->AddRect(screenPos, ImVec2(screenPos.x + previewSize.x, screenPos.y + previewSize.y),
						  IM_COL32(60, 60, 65, 255), 4.0f, 0, 2.0f);
		ImGui::Image(texId, previewSize, { 0, 1 }, { 1, 0 });
	} else {
		ImVec2 screenPos = ImGui::GetCursorScreenPos();
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(screenPos, ImVec2(screenPos.x + previewSize.x, screenPos.y + previewSize.y),
								IM_COL32(25, 25, 28, 255));
		drawList->AddRect(screenPos, ImVec2(screenPos.x + previewSize.x, screenPos.y + previewSize.y),
						  IM_COL32(60, 60, 65, 255), 4.0f, 0, 2.0f);
		const char *text = m_SelectedMaterial ? "Loading..." : "No Material";
		ImGui::PushFont(UI::FontManager::Get().GetFont18());
		ImVec2 textSize = ImGui::CalcTextSize(text);
		ImVec2 textPos = ImVec2(screenPos.x + (previewSize.x - textSize.x) * 0.5f,
								screenPos.y + (previewSize.y - textSize.y) * 0.5f);
		drawList->AddText(textPos, IM_COL32(150, 150, 155, 255), text);
		ImGui::PopFont();
		ImGui::Dummy(previewSize);
	}

	if (!m_AutoRotate) {
		ImGui::Spacing();
		ImGui::SetNextItemWidth(-12);
		if (ImGui::SliderFloat("##ManualRotation", &m_PreviewRotation, 0.0f, 360.0f, "Rotation: %.1f°")) {
			m_PreviewSystem.SetRotation(m_PreviewRotation);
		}
	}

	ImGui::Unindent(12);
	ImGui::PopStyleVar(2);
}

void MaterialEditorPanel::RenderDetailsPanel() {
	// ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
	// ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

	// ImGui::Spacing();
	// ImGui::Indent(12);

	// ImGui::PushFont(UI::FontManager::Get().GetFont18());
	// ImGui::Text(ICON_LC_INFO " DETAILS");
	// ImGui::PopFont();

	// ImGui::Spacing();
	// ImGui::Separator();
	// ImGui::Spacing();

	// if (!m_SelectedMaterial) {
	// 	ImGui::TextDisabled("No material selected");
	// 	ImGui::Unindent(12);
	// 	ImGui::PopStyleVar(2);
	// 	return;
	// }

	// if (ImGui::CollapsingHeader(ICON_LC_FILE " Material Info", ImGuiTreeNodeFlags_DefaultOpen)) {
	// 	ImGui::Indent(8);

	// 	auto tmpl = m_SelectedMaterial->GetTemplate();
	// 	if (tmpl) {
	// 		ImGui::Text("Name:");
	// 		ImGui::SameLine(100);
	// 		ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", m_SelectedMaterial->name.c_str());

	// 		ImGui::Text("Template:");
	// 		ImGui::SameLine(100);
	// 		ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.4f, 1.0f), "%s", tmpl->name.c_str());

	// 		if (tmpl->shader) {
	// 			ImGui::Text("Shader:");
	// 			ImGui::SameLine(100);
	// 			ImGui::Text("%s", tmpl->shader->m_Name.c_str());
	// 		}
	// 	}

	// 	ImGui::Unindent(8);
	// }

	// ImGui::Spacing();

	// if (m_ShowStats && ImGui::CollapsingHeader(ICON_LC_CHART_COLUMN " Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
	// 	ImGui::Indent(8);

	// 	auto properties = m_SelectedMaterial->GetAllProperties();
	// 	int texCount = 0, paramCount = 0;
	// 	for (const auto &[name, prop] : properties) {
	// 		if (prop.m_Type == Aquila::Graphics::Material::ParameterType::Texture2D) {
	// 			texCount++;
	// 		} else {
	// 			paramCount++;
	// 		}
	// 	}

	// 	ImGui::Text("Parameters:");
	// 	ImGui::SameLine(120);
	// 	ImGui::Text("%d", paramCount);

	// 	ImGui::Text("Textures:");
	// 	ImGui::SameLine(120);
	// 	ImGui::Text("%d", texCount);

	// 	ImGui::Spacing();
	// 	ImGui::Separator();
	// 	ImGui::Spacing();

	// 	ImGui::Text("Status:");
	// 	ImGui::Indent(8);

	// 	if (m_SelectedMaterial->IsDirty()) {
	// 		ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), ICON_LC_TRIANGLE_ALERT " Modified");
	// 	} else {
	// 		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), ICON_LC_CHECK " Saved");
	// 	}

	// 	if (m_SelectedMaterial->GetPipeline() != VK_NULL_HANDLE) {
	// 		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), ICON_LC_CHECK " Pipeline Valid");
	// 	} else {
	// 		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ICON_LC_X " Pipeline Invalid");
	// 	}

	// 	ImGui::Unindent(8);
	// 	ImGui::Unindent(8);
	// }

	// ImGui::Unindent(12);
	// ImGui::PopStyleVar(2);
}

void MaterialEditorPanel::RenderMaterialList() {
	// ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
	// ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));

	// ImGui::SetNextItemWidth(250);
	// char searchBuffer[256];
	// strncpy(searchBuffer, m_SearchFilter.c_str(), 255);
	// if (ImGui::InputTextWithHint("##Search", ICON_LC_SEARCH " Search materials...", searchBuffer, 256)) {
	// 	m_SearchFilter = searchBuffer;
	// }

	// ImGui::Spacing();
	// ImGui::Separator();
	// ImGui::Spacing();

	// ImGui::BeginChild("##MaterialListScroll", ImVec2(300, 400), false);

	// auto *materialSystem = m_App.GetRenderer().GetMaterialSystem();
	// if (!materialSystem) {
	// 	ImGui::TextDisabled("Material system not available");
	// 	ImGui::EndChild();
	// 	ImGui::PopStyleVar(2);
	// 	return;
	// }

	// const auto &materials = materialSystem->GetLibrary().GetAllMaterials();

	// for (const auto &[name, material] : materials) {
	// 	if (name == "FallbackMaterial") continue;

	// 	if (!m_SearchFilter.empty() && name.find(m_SearchFilter) == std::string::npos) {
	// 		continue;
	// 	}

	// 	bool isSelected = (m_SelectedMaterial == material);
	// 	const char *icon = GetMaterialIcon(material->GetTemplate() ? material->GetTemplate()->name : "");

	// 	if (isSelected) {
	// 		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
	// 	}

	// 	std::string displayName = std::string(icon) + "  " + name;

	// 	if (ImGui::Selectable(displayName.c_str(), isSelected, 0, ImVec2(0, 28))) {
	// 		SetSelectedMaterial(material);
	// 		ImGui::CloseCurrentPopup();
	// 	}

	// 	if (isSelected) {
	// 		ImGui::PopStyleColor();
	// 	}
	// }

	// ImGui::EndChild();
	// ImGui::PopStyleVar(2);
}

void MaterialEditorPanel::RenderRenderStateSection() {
	if (!ImGui::CollapsingHeader(ICON_LC_SETTINGS " Render State", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	ImGui::Indent(8);
	ImGui::Spacing();

	auto renderState = m_SelectedMaterial->GetRenderState();
	bool stateChanged = false;

	const char *blendModes[] = { "Opaque", "Alpha Blend", "Additive", "Multiply" };
	int currentBlend = static_cast<int>(renderState.m_BlendMode);
	if (ImGui::Combo("Blend Mode", &currentBlend, blendModes, 4)) {
		renderState.m_BlendMode = static_cast<Aquila::Graphics::Material::BlendMode>(currentBlend);
		stateChanged = true;
	}

	const char *cullModes[] = { "None", "Front", "Back" };
	int currentCull = static_cast<int>(renderState.m_CullMode);
	if (ImGui::Combo("Cull Mode", &currentCull, cullModes, 3)) {
		renderState.m_CullMode = static_cast<Aquila::Graphics::Material::CullMode>(currentCull);
		stateChanged = true;
	}

	if (ImGui::Checkbox("Depth Test", &renderState.m_DepthTest))
		stateChanged = true;
	if (ImGui::Checkbox("Depth Write", &renderState.m_DepthWrite))
		stateChanged = true;
	if (ImGui::Checkbox("Wireframe", &renderState.m_Wireframe))
		stateChanged = true;

	if (renderState.m_Wireframe) {
		if (ImGui::SliderFloat("Line Width", &renderState.m_LineWidth, 1.0f, 10.0f)) {
			stateChanged = true;
		}
	}

	if (stateChanged) {
		m_SelectedMaterial->SetRenderState(renderState);
		m_PreviewNeedsUpdate = true;
	}

	ImGui::Unindent(8);
}

void MaterialEditorPanel::RenderShaderGraph() {
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));

	ImGui::Spacing();
	ImGui::Indent(12);

	ImGui::PushFont(UI::FontManager::Get().GetFont18());
	ImGui::Text(ICON_LC_NETWORK " SHADER GRAPH");
	ImGui::PopFont();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (!m_SelectedMaterial) {
		ImGui::TextDisabled("No material selected");
		ImGui::Unindent(12);
		ImGui::PopStyleVar();
		return;
	}

	ImVec2 graphSize = ImGui::GetContentRegionAvail();
	graphSize.x -= 24;
	graphSize.y -= 24;

	ImVec2 cursorPos = ImGui::GetCursorScreenPos();
	ImDrawList *drawList = ImGui::GetWindowDrawList();

	drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + graphSize.x, cursorPos.y + graphSize.y),
							IM_COL32(20, 20, 22, 255));

	f32 gridSize = 50.0f;
	for (f32 x = 0; x < graphSize.x; x += gridSize) {
		drawList->AddLine(ImVec2(cursorPos.x + x, cursorPos.y), ImVec2(cursorPos.x + x, cursorPos.y + graphSize.y),
						  IM_COL32(35, 35, 37, 255));
	}
	for (f32 y = 0; y < graphSize.y; y += gridSize) {
		drawList->AddLine(ImVec2(cursorPos.x, cursorPos.y + y), ImVec2(cursorPos.x + graphSize.x, cursorPos.y + y),
						  IM_COL32(35, 35, 37, 255));
	}

	drawList->AddRect(cursorPos, ImVec2(cursorPos.x + graphSize.x, cursorPos.y + graphSize.y),
					  IM_COL32(60, 60, 65, 255), 0.0f, 0, 2.0f);

	const char *text = "Shader Graph Coming Soon";
	ImGui::PushFont(UI::FontManager::Get().GetFont16());
	ImVec2 textSize = ImGui::CalcTextSize(text);
	ImVec2 textPos =
		ImVec2(cursorPos.x + (graphSize.x - textSize.x) * 0.5f, cursorPos.y + (graphSize.y - textSize.y) * 0.5f);
	drawList->AddText(textPos, IM_COL32(100, 100, 105, 255), text);
	ImGui::PopFont();

	ImGui::Dummy(graphSize);

	ImGui::Unindent(12);
	ImGui::PopStyleVar();
}

void MaterialEditorPanel::RenderColorProperty(const std::string &name,
											  Aquila::Graphics::Material::MaterialParameter &param) {
	std::string displayName = param.m_DisplayName.empty() ? name : param.m_DisplayName;

	auto value = m_SelectedMaterial->GetProperty(name);
	vec4 color = std::holds_alternative<vec4>(value) ? std::get<vec4>(value) : vec4(1.0f);

	ImGui::PushItemWidth(-80);
	if (ImGui::ColorEdit4(displayName.c_str(), &color[0], ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float)) {
		m_SelectedMaterial->SetParameter(name, color);
		m_PreviewNeedsUpdate = true;
	}
	ImGui::PopItemWidth();
}

void MaterialEditorPanel::RenderFloatProperty(const std::string &name,
											  Aquila::Graphics::Material::MaterialParameter &param) {
	std::string displayName = param.m_DisplayName.empty() ? name : param.m_DisplayName;

	auto propValue = m_SelectedMaterial->GetProperty(name);
	f32 value = std::holds_alternative<f32>(propValue) ? std::get<f32>(propValue) : 0.0f;

	f32 minValue = param.m_MinValue;
	f32 maxValue = param.m_MaxValue;

	ImGui::PushItemWidth(-80);
	bool changed = false;

	if (minValue < maxValue) {
		changed = ImGui::SliderFloat(displayName.c_str(), &value, minValue, maxValue, "%.3f");
	} else {
		changed = ImGui::DragFloat(displayName.c_str(), &value, 0.01f, 0.0f, 0.0f, "%.3f");
	}

	ImGui::PopItemWidth();

	if (changed) {
		m_SelectedMaterial->SetParameter(name, value);
		m_PreviewNeedsUpdate = true;
	}
}

const char *MaterialEditorPanel::GetMaterialIcon(const std::string &templateName) {
	if (templateName == "PBR")
		return ICON_LC_SPARKLES;
	if (templateName == "Unlit")
		return ICON_LC_SUN;
	if (templateName == "Transparent")
		return ICON_LC_DROPLET;
	return ICON_LC_PALETTE;
}

const char *MaterialEditorPanel::GetPreviewMeshName(PreviewMesh mesh) {
	switch (mesh) {
	case PreviewMesh::Sphere:
		return "Sphere";
	case PreviewMesh::Cube:
		return "Cube";
	case PreviewMesh::Cylinder:
		return "Cylinder";
	case PreviewMesh::Plane:
		return "Plane";
	default:
		return "Unknown";
	}
}

void MaterialEditorPanel::SetSelectedMaterial(Ref<Aquila::Graphics::Material::Material> material) {
	if (m_SelectedMaterial != material) {
		m_SelectedMaterial = material;
		m_PreviewNeedsUpdate = true;
		m_PreviewSystem.SetMaterial(material);
		AQUILA_LOG_INFO("Selected material: {}", material ? material->name : "None");
	}
}

void MaterialEditorPanel::RefreshMaterialList() {
	// m_Materials.clear();

	// auto *materialSystem = m_App.GetRenderer().GetMaterialSystem();
	// if (!materialSystem) return;

	// const auto &materials = materialSystem->GetLibrary().GetAllMaterials();
	// for (const auto &[name, material] : materials) {
	// 	if (name != "FallbackMaterial") {
	// 		m_Materials.push_back(material);
	// 	}
	// }
}

void MaterialEditorPanel::OnEvent(Aquila::Events::Event &event) {}

void MaterialEditorPanel::OnRender() {
	// if (m_PreviewSystem.IsInitialized() && m_SelectedMaterial) {
	// 	m_PreviewSystem.Render();
	// }
}

void MaterialEditorPanel::ChangePreviewMesh(PreviewMesh mesh) {
	if (m_CurrentPreviewMesh == mesh) {
		return;
	}

	m_CurrentPreviewMesh = mesh;

	PreviewMesh sysPreviewMesh;
	switch (mesh) {
	case PreviewMesh::Sphere:
		sysPreviewMesh = PreviewMesh::Sphere;
		break;
	case PreviewMesh::Cube:
		sysPreviewMesh = PreviewMesh::Cube;
		break;
	case PreviewMesh::Cylinder:
		sysPreviewMesh = PreviewMesh::Cylinder;
		break;
	case PreviewMesh::Plane:
		sysPreviewMesh = PreviewMesh::Plane;
		break;
	default:
		sysPreviewMesh = PreviewMesh::Sphere;
		break;
	}

	m_PreviewSystem.SetPreviewMesh(sysPreviewMesh);
	m_PreviewNeedsUpdate = true;
	AQUILA_LOG_INFO("Changed preview mesh to: {}", GetPreviewMeshName(mesh));
}

void MaterialEditorPanel::CreateNewMaterial() {
	m_ShowCreateDialog = true;
}

void MaterialEditorPanel::DuplicateMaterial() {
	// if (!m_SelectedMaterial) return;

	// auto *materialSystem = m_App.GetRenderer().GetMaterialSystem();
	// if (!materialSystem) return;

	// auto tmpl = m_SelectedMaterial->GetTemplate();
	// if (!tmpl) return;

	// std::string newName = m_SelectedMaterial->name + "_Copy";
	// int suffix = 1;
	// while (materialSystem->GetLibrary().HasMaterial(newName)) {
	// 	newName = m_SelectedMaterial->name + "_Copy" + std::to_string(suffix++);
	// }

	// auto newMat = materialSystem->GetLibrary().CreateMaterial(newName, tmpl->name);

	// auto properties = m_SelectedMaterial->GetAllProperties();
	// for (const auto &[propName, param] : properties) {
	// 	switch (param.m_Type) {
	// 	case Aquila::Graphics::Material::ParameterType::Float: {
	// 		auto val = m_SelectedMaterial->GetProperty(propName);
	// 		if (std::holds_alternative<f32>(val)) {
	// 			newMat->Setf32(propName, std::get<f32>(val));
	// 		}
	// 		break;
	// 	}
	// 	case Aquila::Graphics::Material::ParameterType::Color: {
	// 		auto val = m_SelectedMaterial->GetProperty(propName);
	// 		if (std::holds_alternative<vec4>(val)) {
	// 			newMat->SetColor(propName, std::get<vec4>(val));
	// 		}
	// 		break;
	// 	}
	// 	case Aquila::Graphics::Material::ParameterType::Texture2D: {
	// 		auto tex = m_SelectedMaterial->GetTexture(propName);
	// 		if (tex) {
	// 			newMat->SetTexture(propName, tex);
	// 		}
	// 		break;
	// 	}
	// 	default:
	// 		break;
	// 	}
	// }

	// newMat->SetRenderState(m_SelectedMaterial->GetRenderState());
	// newMat->MarkDirty();

	// SetSelectedMaterial(newMat);
	// RefreshMaterialList();

	// AQUILA_LOG_INFO("Duplicated material: {} -> {}", m_SelectedMaterial->name, newName);
}

void MaterialEditorPanel::DeleteMaterial() {
	if (!m_SelectedMaterial)
		return;
	ImGui::OpenPopup("DeleteMaterialConfirm");
}

void MaterialEditorPanel::SaveMaterial() {
	if (!m_SelectedMaterial)
		return;

	std::string fullPath = "assets://materials/" + m_SelectedMaterial->name + ".aqmat";

	if (Aquila::Graphics::Material::MaterialSerializer::SaveToFileVFS(m_SelectedMaterial, fullPath)) {
		AQUILA_LOG_INFO("Material saved to: {}", fullPath);
	} else {
		AQUILA_LOG_ERROR("Failed to save material: {}", m_SelectedMaterial->name);
	}
}

void MaterialEditorPanel::LoadMaterial() {
	AQUILA_LOG_INFO("Load material dialog");
}

bool MaterialEditorPanel::IsPropertyOverridden(const std::string &propName) {
	if (!m_SelectedMaterial)
		return false;

	auto tmpl = m_SelectedMaterial->GetTemplate();
	if (!tmpl)
		return false;

	const auto *templateProp = tmpl->GetProperty(propName);
	if (!templateProp)
		return false;

	auto currentValue = m_SelectedMaterial->GetProperty(propName);
	return currentValue != templateProp->m_DefaultValue;
}

} // namespace Editor

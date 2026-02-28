#include "UI/Windows/PreferencesWindow.h"
#include "UI/Managers/ThemeManager.h"
#include "UI/Managers/FontManager.h"
#include "Aquila/Core/Application.h"
#include "Aquila/Core/Defines.h"
#include "lucide.h"
#include <imgui.h>

namespace Editor::UI {

PreferencesWindow::PreferencesWindow(Aquila::Core::Application &app) : Layer("PreferencesWindow"), m_App(app) {
	AQUILA_LOG_INFO("PreferencesWindow created");
	m_WorkingPrefs = Config::GetPreferences();
}

void PreferencesWindow::OnImGuiRender() {
	if (!m_IsOpen) {
		return;
	}

	if (ImGui::IsWindowAppearing()) {
		m_WorkingPrefs = Config::GetPreferences();
		m_HasUnsavedChanges = false;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
	static int selectedCategory = 0;

	if (ImGui::Begin(ICON_LC_SETTINGS " Editor Preferences", &m_IsOpen)) {
		ImGui::BeginChild("PreferencesCategories", ImVec2(200, -40), true);
		{
			if (ImGui::Selectable(ICON_LC_SETTINGS " General", selectedCategory == 0)) {
				selectedCategory = 0;
			}
			if (ImGui::Selectable(ICON_LC_PALETTE " Appearance", selectedCategory == 1)) {
				selectedCategory = 1;
			}
			if (ImGui::Selectable(ICON_LC_CAMERA " Viewport", selectedCategory == 2)) {
				selectedCategory = 2;
			}
			if (ImGui::Selectable(ICON_LC_FOLDER " Asset Browser", selectedCategory == 3)) {
				selectedCategory = 3;
			}
			if (ImGui::Selectable(ICON_LC_KEYBOARD " Shortcuts", selectedCategory == 4)) {
				selectedCategory = 4;
			}

			ImGui::Separator();

			if (ImGui::Button(ICON_LC_ROTATE_CCW " Reset All", ImVec2(-1, 0))) {
				ImGui::OpenPopup("ResetConfirm");
			}

			if (ImGui::BeginPopupModal("ResetConfirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::TextUnformatted("Are you sure you want to reset all preferences to defaults?");
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				if (ImGui::Button("Yes, Reset", ImVec2(120, 0))) {
					ResetToDefaults();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			ImGui::EndChild();
		}

		ImGui::SameLine();

		ImGui::BeginChild("PreferencesContent", ImVec2(0, -40), true);
		{
			switch (selectedCategory) {
			case 0:
				RenderGeneralSettings();
				break;
			case 1:
				RenderAppearanceSettings();
				break;
			case 2:
				RenderViewportSettings();
				break;
			case 3:
				RenderAssetBrowserSettings();
				break;
			case 4:
				RenderShortcutsSettings();
				break;
			}
		}
		ImGui::EndChild();

		ImGui::Separator();

		if (m_HasUnsavedChanges) {
			ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), ICON_LC_TRIANGLE_ALERT " Unsaved changes");
			ImGui::SameLine();
		}

		f32 buttonWidth = 100.0f;
		f32 spacing = ImGui::GetStyle().ItemSpacing.x;
		f32 totalWidth = (buttonWidth * 3) + (spacing * 2);
		f32 offsetX = ImGui::GetContentRegionAvail().x - totalWidth;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

		if (ImGui::Button(ICON_LC_CHECK " Apply", ImVec2(buttonWidth, 0))) {
			ApplyChanges();
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_SAVE " Save", ImVec2(buttonWidth, 0))) {
			ApplyChanges();
			m_WorkingPrefs.SaveToFile();
			m_HasUnsavedChanges = false;
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_X " Close", ImVec2(buttonWidth, 0))) {
			if (m_HasUnsavedChanges) {
				ImGui::OpenPopup("UnsavedChanges");
			} else {
				m_IsOpen = false;
			}
		}

		if (ImGui::BeginPopupModal("UnsavedChanges", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextUnformatted("You have unsaved changes. What would you like to do?");
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Button("Save & Close", ImVec2(120, 0))) {
				ApplyChanges();
				m_WorkingPrefs.SaveToFile();
				m_HasUnsavedChanges = false;
				m_IsOpen = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Discard & Close", ImVec2(120, 0))) {
				m_WorkingPrefs = Config::GetPreferences();
				m_HasUnsavedChanges = false;
				m_IsOpen = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
	}
	ImGui::End();
}

void PreferencesWindow::RenderGeneralSettings() {
	ImGui::TextUnformatted(ICON_LC_SETTINGS " General Settings");
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Checkbox("Enable Auto-Save", &m_WorkingPrefs.autoSave)) {
		m_HasUnsavedChanges = true;
	}

	if (m_WorkingPrefs.autoSave) {
		ImGui::Indent();
		ImGui::Text("Auto-save interval (seconds):");
		if (ImGui::SliderInt("##AutoSaveInterval", reinterpret_cast<int *>(&m_WorkingPrefs.autoSaveInterval), 60,
							 600)) {
			m_HasUnsavedChanges = true;
		}
		ImGui::Unindent();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextUnformatted("Window Settings:");

	if (ImGui::Checkbox("Enable VSync", &m_WorkingPrefs.window.vsync)) {
		m_HasUnsavedChanges = true;
	}

	if (ImGui::Checkbox("Start Maximized", &m_WorkingPrefs.window.maximized)) {
		m_HasUnsavedChanges = true;
	}

	ImGui::Spacing();
	ImGui::Text("Default Window Size:");
	ImGui::SetNextItemWidth(150);
	if (ImGui::InputScalar("Width##WindowWidth", ImGuiDataType_U32, &m_WorkingPrefs.window.width)) {
		m_HasUnsavedChanges = true;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(150);
	if (ImGui::InputScalar("Height##WindowHeight", ImGuiDataType_U32, &m_WorkingPrefs.window.height)) {
		m_HasUnsavedChanges = true;
	}
}

void PreferencesWindow::RenderAppearanceSettings() {
	ImGui::TextUnformatted(ICON_LC_PALETTE " Appearance Settings");
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextUnformatted("Theme:");
	const char *themeNames[] = { "Aquila", "Aquila 2", "Dark", "Light", "Custom" };
	int currentTheme = static_cast<int>(m_WorkingPrefs.currentTheme);

	if (ImGui::Combo("##Theme", &currentTheme, themeNames, IM_ARRAYSIZE(themeNames))) {
		m_WorkingPrefs.currentTheme = static_cast<Editor::Config::Theme>(currentTheme);
		m_HasUnsavedChanges = true;

		ThemeManager::Get().ApplyTheme(m_WorkingPrefs.currentTheme);
	}

	ImGui::Spacing();

	if (ImGui::Button(ICON_LC_PALETTE " Open Theme Editor")) {
		AQUILA_LOG_INFO("Opening theme editor...");
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextUnformatted("Font Settings:");

	ImGui::Text("Default Font Size:");
	f32 fontSize = m_WorkingPrefs.fonts.defaultFontSize;
	if (ImGui::SliderFloat("##FontSize", &fontSize, 10.0f, 32.0f, "%.0f pt")) {
		m_WorkingPrefs.fonts.defaultFontSize = fontSize;
		m_HasUnsavedChanges = true;
	}

	ImGui::Spacing();

	ImGui::TextUnformatted("Main Font Path:");
	char fontPathBuffer[256];
	strncpy(fontPathBuffer, m_WorkingPrefs.fonts.mainFontPath.c_str(), sizeof(fontPathBuffer) - 1);
	fontPathBuffer[sizeof(fontPathBuffer) - 1] = '\0';

	ImGui::SetNextItemWidth(-80);
	if (ImGui::InputText("##MainFontPath", fontPathBuffer, sizeof(fontPathBuffer))) {
		m_WorkingPrefs.fonts.mainFontPath = fontPathBuffer;
		m_HasUnsavedChanges = true;
	}
	ImGui::SameLine();
	if (ImGui::Button(ICON_LC_FOLDER_OPEN "##BrowseFont")) {
		AQUILA_LOG_INFO("Opening file browser for font...");
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextUnformatted("System Fonts:");

	static std::string selectedFamily = "";
	static std::string selectedStyle = "Regular";

	FontManager::Get().ShowFontSelector("Select System Font", selectedFamily, selectedStyle);

	if (!selectedFamily.empty() && ImGui::Button(ICON_LC_DOWNLOAD " Load Selected Font")) {
		const auto *sysFont = FontManager::Get().FindSystemFont(selectedFamily, selectedStyle);
		if (sysFont) {
			m_WorkingPrefs.fonts.mainFontPath = sysFont->path;
			m_HasUnsavedChanges = true;
			AQUILA_LOG_INFO("Selected font: {} - {}", selectedFamily, sysFont->path);
		}
	}

	ImGui::Spacing();

	if (ImGui::Button(ICON_LC_ROTATE_CCW " Reload Fonts")) {
		FontManager::Get().ReloadFonts();
	}
}

void PreferencesWindow::RenderViewportSettings() {
	ImGui::TextUnformatted(ICON_LC_CAMERA " Viewport Settings");
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Checkbox("Show Grid", &m_WorkingPrefs.showGrid)) {
		m_HasUnsavedChanges = true;
	}

	if (ImGui::Checkbox("Show Gizmos", &m_WorkingPrefs.showGizmos)) {
		m_HasUnsavedChanges = true;
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextUnformatted("Camera Controls:");

	ImGui::Text("Move Speed:");
	if (ImGui::SliderFloat("##CameraMoveSpeed", &m_WorkingPrefs.cameraMoveSpeed, 0.1f, 20.0f, "%.1f")) {
		m_HasUnsavedChanges = true;
	}

	ImGui::Text("Rotate Speed:");
	if (ImGui::SliderFloat("##CameraRotateSpeed", &m_WorkingPrefs.cameraRotateSpeed, 0.1f, 2.0f, "%.2f")) {
		m_HasUnsavedChanges = true;
	}

	if (ImGui::Checkbox("Invert Mouse Y Axis", &m_WorkingPrefs.invertMouseY)) {
		m_HasUnsavedChanges = true;
	}
}

void PreferencesWindow::RenderAssetBrowserSettings() {
	ImGui::TextUnformatted(ICON_LC_FOLDER " Asset Browser Settings");
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("Thumbnail Size:");
	if (ImGui::SliderFloat("##ThumbnailSize", &m_WorkingPrefs.thumbnailSize, 32.0f, 256.0f, "%.0f px")) {
		m_HasUnsavedChanges = true;
	}

	ImGui::Spacing();

	if (ImGui::Checkbox("Show File Extensions", &m_WorkingPrefs.showFileExtensions)) {
		m_HasUnsavedChanges = true;
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextUnformatted("Import Settings:");
	ImGui::TextWrapped("Configure default import settings for different asset types here.");
}

void PreferencesWindow::RenderShortcutsSettings() {
	ImGui::TextUnformatted(ICON_LC_KEYBOARD " Keyboard Shortcuts");
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::TextWrapped("Customize keyboard shortcuts for common editor actions.");
	ImGui::Spacing();

	if (ImGui::BeginTable("Shortcuts", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
		ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Shortcut", ImGuiTableColumnFlags_WidthFixed, 150.0f);
		ImGui::TableHeadersRow();

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("New Scene");
		ImGui::TableNextColumn();
		ImGui::Text("Ctrl+N");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Open Scene");
		ImGui::TableNextColumn();
		ImGui::Text("Ctrl+O");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Save Scene");
		ImGui::TableNextColumn();
		ImGui::Text("Ctrl+S");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Save Scene As");
		ImGui::TableNextColumn();
		ImGui::Text("Ctrl+Shift+S");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Undo");
		ImGui::TableNextColumn();
		ImGui::Text("Ctrl+Z");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Redo");
		ImGui::TableNextColumn();
		ImGui::Text("Ctrl+Y");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Copy");
		ImGui::TableNextColumn();
		ImGui::Text("Ctrl+C");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Paste");
		ImGui::TableNextColumn();
		ImGui::Text("Ctrl+V");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Duplicate");
		ImGui::TableNextColumn();
		ImGui::Text("Ctrl+D");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Delete");
		ImGui::TableNextColumn();
		ImGui::Text("Delete");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Focus Selected");
		ImGui::TableNextColumn();
		ImGui::Text("F");

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Toggle Gizmo Mode");
		ImGui::TableNextColumn();
		ImGui::Text("Q/W/E/R");

		ImGui::EndTable();
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Note: Shortcut customization coming soon!");
}

void PreferencesWindow::ApplyChanges() {
	Config::GetPreferences() = m_WorkingPrefs;

	ThemeManager::Get().ApplyTheme(m_WorkingPrefs.currentTheme);

	if (m_DeferredOperationCallback) {
		m_DeferredOperationCallback();
		AQUILA_LOG_INFO("Changes applied - font reload scheduled for next frame");
	}
}

void PreferencesWindow::ResetToDefaults() {
	m_WorkingPrefs.ResetToDefaults();
	ApplyChanges();
	m_HasUnsavedChanges = true;

	AQUILA_LOG_INFO("Preferences reset to defaults");
}

} // namespace Editor::UI

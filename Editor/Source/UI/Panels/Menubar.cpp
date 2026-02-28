#include "UI/Panels/Menubar.h"
#include "UI/Panels/MaterialEditor.h"
#include "UI/Windows/AboutWindow.h"
#include "UI/Windows/PreferencesWindow.h"
#include "UI/Managers/ThemeManager.h"
#include "Aquila/Assets/AssetManager.h"
#include "Aquila/Scene/SceneManager.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Core/Application.h"
#include "lucide.h"

namespace Editor {

Menubar::Menubar(Aquila::Core::Application &app) : Layer("Menubar"), m_App(app) {
	AQUILA_LOG_INFO("Menubar created");

	// Initialize buffers
	memset(m_NewSceneNameBuffer, 0, sizeof(m_NewSceneNameBuffer));
	strcpy_s(m_NewSceneNameBuffer, sizeof(m_NewSceneNameBuffer), "Untitled Scene");

	memset(m_SaveSceneNameBuffer, 0, sizeof(m_SaveSceneNameBuffer));
	memset(m_SaveScenePathBuffer, 0, sizeof(m_SaveScenePathBuffer));
}

void Menubar::OnAttach() {
	AQUILA_LOG_INFO("Menubar attached");
}

void Menubar::OnDetach() {
	AQUILA_LOG_INFO("Menubar detached");
}

void Menubar::OnUpdate(const f32 deltaTime) {
	Layer::OnUpdate(deltaTime);
}

void Menubar::OnImGuiRender() {
	if (ImGui::BeginMainMenuBar()) {
		RenderFileMenu();
		RenderEditMenu();
		RenderViewMenu();
		RenderHelpMenu();

		ImGui::EndMainMenuBar();
	}

	// Open modals when flags are set
	if (m_ShowNewSceneModal) {
		ImGui::OpenPopup("NewSceneModal");
		m_ShowNewSceneModal = false;
	}

	if (m_ShowSaveSceneAsModal) {
		ImGui::OpenPopup("SaveSceneAsModal");
		m_ShowSaveSceneAsModal = false;
	}

	// Render modals
	RenderNewSceneModal();
	RenderSaveSceneAsModal();
}

void Menubar::OnEvent(Aquila::Events::Event &event) {
	Layer::OnEvent(event);
}

void Menubar::RenderFileMenu() {
	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem(ICON_LC_FILE " New Scene", "Ctrl+N")) {
			m_ShowNewSceneModal = true;
		}

		if (ImGui::MenuItem(ICON_LC_FOLDER_OPEN " Open Scene...", "Ctrl+O")) {
			OpenSceneDialog();
		}

		if (ImGui::MenuItem(ICON_LC_SAVE " Save Scene", "Ctrl+S")) {
			SaveCurrentScene();
		}

		if (ImGui::MenuItem(ICON_LC_SAVE " Save Scene As...", "Ctrl+Shift+S")) {
			m_ShowSaveSceneAsModal = true;
		}

		ImGui::Separator();

		if (ImGui::MenuItem(ICON_LC_X " Exit", "Alt+F4")) {
			m_App.Close();
		}

		ImGui::EndMenu();
	}
}

void Menubar::RenderEditMenu() {
	if (ImGui::BeginMenu("Edit")) {
		if (ImGui::MenuItem(ICON_LC_COPY " Copy", "Ctrl+C", false, false)) {
			// TODO: not a priority right now
		}

		if (ImGui::MenuItem(ICON_LC_CLIPBOARD_PASTE " Paste", "Ctrl+V", false, false)) {
			// TODO: not a priority right now
		}

		if (ImGui::MenuItem(ICON_LC_COPY " Duplicate", "Ctrl+D", false, false)) {
			// TODO: not a priority right now
		}

		if (ImGui::MenuItem(ICON_LC_TRASH_2 " Delete", "Delete", false, false)) {
			// TODO: not a priority right now
		}

		ImGui::Separator();

		if (ImGui::MenuItem(ICON_LC_SETTINGS " Preferences...", "Ctrl+,")) {
			if (m_PreferencesWindow != nullptr) {
				m_PreferencesWindow->Show();
			}
		}

		ImGui::EndMenu();
	}
}

void Menubar::RenderViewMenu() {
	if (ImGui::BeginMenu("View")) {
		if (m_MaterialEditor != nullptr) {
			bool isOpen = m_MaterialEditor->IsOpen();
			if (ImGui::MenuItem(ICON_LC_PALETTE " Material Editor", "Ctrl+M", &isOpen)) {
				m_MaterialEditor->SetOpen(isOpen);
			}
		}

		ImGui::Separator();

		ImGui::TextDisabled("Panels:");
		if (ImGui::MenuItem(ICON_LC_LAYOUT_DASHBOARD " Scene Hierarchy", nullptr, true)) {
			// TODO: not a priority right now
		}
		if (ImGui::MenuItem(ICON_LC_SLIDERS_HORIZONTAL " Properties", nullptr, true)) {
			// TODO: not a priority right now
		}
		if (ImGui::MenuItem(ICON_LC_FOLDER " Content Browser", nullptr, true)) {
			// TODO: not a priority right now
		}
		if (ImGui::MenuItem(ICON_LC_MAXIMIZE_2 " Viewport", nullptr, true)) {
			// TODO: not a priority right now
		}

		ImGui::Separator();

		// Theme selector
		if (ImGui::BeginMenu(ICON_LC_PALETTE " Theme")) {
			UI::ThemeManager::Get().ShowThemeSelector();
			ImGui::EndMenu();
		}

		ImGui::Separator();

		if (ImGui::MenuItem(ICON_LC_LAYOUT_DASHBOARD " Reset Layout")) {
			// TODO: Reset docking layout
			AQUILA_LOG_INFO("Reset layout not implemented yet");
		}

		ImGui::EndMenu();
	}
}

void Menubar::RenderHelpMenu() {
	if (ImGui::BeginMenu("Help")) {
		if (ImGui::MenuItem(ICON_LC_BOOK_OPEN " Documentation", "F1")) {
#if defined(AQUILA_PLATFORM_WINDOWS)
			ShellExecute(nullptr, "open", "https://github.com/adinte18/Aquila/wiki", NULL, NULL, SW_SHOWNORMAL);
#elif defined(AQUILA_PLATFORM_LINUX)
			system("xdg-open https://github.com/adinte18/Aquila/wiki");
#elif defined(AQUILA_PLATFORM_MACOS)
			system("open https://github.com/adinte18/Aquila/wiki");
#endif

			AQUILA_LOG_INFO("Opening documentation...");
		}

		ImGui::Separator();

		if (ImGui::MenuItem(ICON_LC_GITHUB " GitHub Repository")) {
#if defined(AQUILA_PLATFORM_WINDOWS)
			ShellExecute(nullptr, "open", "https://github.com/adinte18/Aquila/", NULL, NULL, SW_SHOWNORMAL);
#elif defined(AQUILA_PLATFORM_LINUX)
			system("xdg-open https://github.com/adinte18/Aquila/");
#elif defined(AQUILA_PLATFORM_MACOS)
			system("open https://github.com/adinte18/Aquila/");
#endif
		}

		if (ImGui::MenuItem(ICON_LC_MESSAGE_CIRCLE " Report Issue")) {
#if defined(AQUILA_PLATFORM_WINDOWS)
			ShellExecute(nullptr, "open", "https://github.com/adinte18/Aquila/issues/new", NULL, NULL, SW_SHOWNORMAL);
#elif defined(AQUILA_PLATFORM_LINUX)
			system("xdg-open https://github.com/adinte18/Aquila/issues/new");
#elif defined(AQUILA_PLATFORM_MACOS)
			system("open https://github.com/adinte18/Aquila/issues/new");
#endif
		}

		ImGui::Separator();

		if (ImGui::MenuItem(ICON_LC_INFO " About Aquila Editor")) {
			if (m_AboutWindow != nullptr) {
				m_AboutWindow->Show();
			}
		}

		ImGui::EndMenu();
	}
}

void Menubar::RenderNewSceneModal() {
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(450, 0), ImGuiCond_Appearing);

	if (ImGui::BeginPopupModal("NewSceneModal", nullptr, ImGuiWindowFlags_NoResize)) {
		ImGui::TextUnformatted(ICON_LC_FILE " Create New Scene");
		ImGui::Separator();
		ImGui::Spacing();

		// Scene name input
		ImGui::TextUnformatted("Scene Name:");
		ImGui::SetNextItemWidth(-1);
		if (ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere();
		}
		ImGui::InputText("##NewSceneName", m_NewSceneNameBuffer, sizeof(m_NewSceneNameBuffer));

		ImGui::Spacing();

		// Template selection
		ImGui::TextUnformatted("Template:");
		ImGui::SetNextItemWidth(-1);
		const char *templates[] = { "Empty", "Basic (with light)", "Outdoor", "Indoor" };
		static int selectedTemplate = 0;
		ImGui::Combo("##Template", &selectedTemplate, templates, IM_ARRAYSIZE(templates));

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Buttons
		f32 buttonWidth = 100.0f;
		f32 spacing = ImGui::GetStyle().ItemSpacing.x;
		f32 totalWidth = (buttonWidth * 2) + spacing;
		f32 offsetX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

		bool canCreate = strlen(m_NewSceneNameBuffer) > 0;

		if (!canCreate) {
			ImGui::BeginDisabled();
		}

		if (ImGui::Button(ICON_LC_CHECK " Create", ImVec2(buttonWidth, 0)) ||
			(canCreate && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
			CreateNewScene(m_NewSceneNameBuffer, selectedTemplate);
			ImGui::CloseCurrentPopup();
		}

		if (!canCreate) {
			ImGui::EndDisabled();
		}

		ImGui::SameLine();

		if (ImGui::Button(ICON_LC_X " Cancel", ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			strcpy_s(m_NewSceneNameBuffer, sizeof(m_NewSceneNameBuffer), "Untitled Scene");
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Menubar::RenderSaveSceneAsModal() {
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(500, 0), ImGuiCond_Appearing);

	if (ImGui::BeginPopupModal("SaveSceneAsModal", nullptr, ImGuiWindowFlags_NoResize)) {
		ImGui::TextUnformatted(ICON_LC_SAVE " Save Scene As");
		ImGui::Separator();
		ImGui::Spacing();

		// Get current scene name as default
		if (ImGui::IsWindowAppearing()) {
			if (auto *scene = m_App.GetAssetManager().GetActiveScene()) {
				strcpy_s(m_SaveSceneNameBuffer, sizeof(m_SaveSceneNameBuffer), scene->GetSceneName().c_str());
			}
			strcpy_s(m_SaveScenePathBuffer, sizeof(m_SaveScenePathBuffer), "assets://scenes/");
		}

		// Scene name
		ImGui::TextUnformatted("Scene Name:");
		ImGui::SetNextItemWidth(-1);
		if (ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere();
		}
		ImGui::InputText("##SaveSceneName", m_SaveSceneNameBuffer, sizeof(m_SaveSceneNameBuffer));

		ImGui::Spacing();

		// File path
		ImGui::TextUnformatted("Save Location:");
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##SaveScenePath", m_SaveScenePathBuffer, sizeof(m_SaveScenePathBuffer));

		ImGui::SameLine();
		if (ImGui::Button(ICON_LC_FOLDER_OPEN)) {
			// TODO: Open folder browser
			AQUILA_LOG_INFO("Folder browser not implemented yet");
		}

		ImGui::Spacing();

		// Preview full path
		ImGui::TextDisabled("Full path:");
		std::string fullPath = std::string(m_SaveScenePathBuffer) + std::string(m_SaveSceneNameBuffer) + ".aqscene";
		ImGui::TextWrapped("%s", fullPath.c_str());

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Buttons
		f32 buttonWidth = 100.0f;
		f32 spacing = ImGui::GetStyle().ItemSpacing.x;
		f32 totalWidth = (buttonWidth * 2) + spacing;
		f32 offsetX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

		bool canSave = strlen(m_SaveSceneNameBuffer) > 0 && strlen(m_SaveScenePathBuffer) > 0;

		if (!canSave) {
			ImGui::BeginDisabled();
		}

		if (ImGui::Button(ICON_LC_SAVE " Save", ImVec2(buttonWidth, 0)) ||
			(canSave && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
			SaveSceneAs(fullPath);
			ImGui::CloseCurrentPopup();
		}

		if (!canSave) {
			ImGui::EndDisabled();
		}

		ImGui::SameLine();

		if (ImGui::Button(ICON_LC_X " Cancel", ImVec2(buttonWidth, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Menubar::CreateNewScene(const std::string &sceneName, int templateType) const {
	auto &sceneManager = m_App.GetAssetManager();

	auto *newScene = sceneManager.CreateScene(sceneName);
	if (!newScene) {
		AQUILA_LOG_ERROR("Failed to create new scene: {}", sceneName);
		return;
	}

	// Apply template
	switch (templateType) {
	case 0: // Empty
		break;

	case 1: { // Basic (with light)
		auto *entityManager = newScene->GetEntityManager();
		auto lightEntity = entityManager->CreateEntity("Directional Light");
		auto &light = lightEntity.AddComponent<Aquila::SceneManagement::Components::LightComponent>();
		light.m_Type = Aquila::SceneManagement::Components::LightComponent::Type::Directional;
		light.m_Color = vec3(1.0f, 1.0f, 1.0f);
		light.m_Intensity = 1.0f;

		auto &lightTransform = lightEntity.GetComponent<Aquila::SceneManagement::Components::TransformComponent>();
		lightTransform.SetLocalRotation(vec3(-45.0f, 45.0f, 0.0f));
		break;
	}

	case 2: // Outdoor
		// TODO: Add outdoor template
		break;

	case 3: // Indoor
		// TODO: Add indoor template
		break;
	}

	sceneManager.ActivateScene(newScene);
	AQUILA_LOG_INFO("Created and activated new scene: {}", sceneName);
}

void Menubar::SaveCurrentScene() const {
	auto &sceneManager = m_App.GetAssetManager();
	auto *scene = sceneManager.GetActiveScene();

	if (!scene) {
		AQUILA_LOG_WARNING("No active scene to save");
		return;
	}

	// For now, just open Save As dialog
	// TODO: Check if scene has been saved before
	ImGui::OpenPopup("SaveSceneAsModal");
}

void Menubar::SaveSceneAs(const std::string &filepath) const {
	auto &sceneManager = m_App.GetAssetManager();
	auto *scene = sceneManager.GetActiveScene();

	if (!scene) {
		AQUILA_LOG_ERROR("No active scene to save");
		return;
	}

	if (sceneManager.SaveScene(scene->GetHandle(), filepath)) {
		AQUILA_LOG_INFO("Scene saved successfully: {}", filepath);
	} else {
		AQUILA_LOG_ERROR("Failed to save scene: {}", filepath);
	}
}

void Menubar::OpenSceneDialog() {
	// TODO: should implement file browser dialog
	AQUILA_LOG_INFO("Open scene dialog not implemented yet");
}

} // namespace Editor

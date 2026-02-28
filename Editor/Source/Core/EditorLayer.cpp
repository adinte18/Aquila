#include "Core/EditorLayer.h"
#include "Aquila/Core/Application.h"
#include "Aquila/Events/Event.h"
#include "Aquila/Graphics/Core/Renderer.h"
#include "UI/Managers/FontManager.h"
#include "UI/Managers/ThemeManager.h"
#include "lucide.h"

#include <imgui.h>

namespace Editor {

static bool showThemeEditor = false;

EditorLayer::EditorLayer(Aquila::Core::Application &app) : Layer("EditorLayer"), m_App(app) {
	AQUILA_LOG_INFO("EditorLayer created");
}

EditorLayer::~EditorLayer() {
	AQUILA_LOG_INFO("EditorLayer destroyed");
}

void EditorLayer::OnAttach() {
	AQUILA_LOG_INFO("EditorLayer attached");

	m_SelectedEntity = Aquila::SceneManagement::Entity::Null();
	m_GizmoOperation = GizmoOperation::Translate;
	m_ViewportFocused = false;
	m_ViewportHovered = false;
}

void EditorLayer::OnDetach() {
	AQUILA_LOG_INFO("EditorLayer detached");
}

void EditorLayer::OnUpdate(f32 deltaTime) {
	// Editor update logic
}

void EditorLayer::RenderDockspace() const {
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
	window_flags |= ImGuiWindowFlags_MenuBar;

	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0F, 0.0F));

	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	if ((dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0) {
		window_flags |= ImGuiWindowFlags_NoBackground;
	}

	ImGui::Begin("AquilaDockspace", nullptr, window_flags);
	ImGui::PopStyleVar(3);

	const ImGuiIO &io = ImGui::GetIO();

	if ((io.ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0) {
		const ImGuiID dockspace_id = ImGui::GetID("AquilaDockspaceID");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0F, 0.0F), dockspace_flags);
	}

	ImGui::End();
}

void EditorLayer::OnImGuiRender() {
	RenderDockspace();

	UI::ThemeManager::Get().ShowThemeEditorWindow(&showThemeEditor);
}

void EditorLayer::OnEvent(Aquila::Events::Event &event) {
	Aquila::Events::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Aquila::Events::KeyPressedEvent>(
		[this](const Aquila::Events::KeyPressedEvent &event) { return OnKeyPressed(event); });

	dispatcher.Dispatch<Aquila::Events::MouseButtonPressedEvent>(
		[this](const Aquila::Events::MouseButtonPressedEvent &event) { return OnMouseButtonPressed(event); });
}

bool EditorLayer::OnKeyPressed(const Aquila::Events::KeyPressedEvent &event) {
	if (event.IsRepeat()) {
		return false;
	}

	// Handle shortcuts
	return false;
}

bool EditorLayer::OnMouseButtonPressed(const Aquila::Events::MouseButtonPressedEvent &event) {
	if (!m_ViewportHovered) {
		return false;
	}

	// Handle viewport clicks
	return false;
}

} // namespace Editor

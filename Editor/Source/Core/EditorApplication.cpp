#include "Core/EditorApplication.h"

#include "UI/EditorDockLayout.h"
#include "UI/Managers/FontManager.h"
#include "UI/Panels/ConsolePanel.h"
#include "UI/Panels/HierarchyPanel.h"
#include "UI/Panels/InspectorPanel.h"
#include "UI/Panels/ViewportPanel.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Graphics/Material/MaterialFactory.h"
#include "Aquila/Graphics/Resources/Mesh.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/UI/Core/Clipboard.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/UI/Core/ViewSystem.h"
#include "Aquila/UI/Rendering/ViewRenderingSystem.h"
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/ColorPicker.h"
#include "Aquila/UI/Widgets/ContextMenu.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;
using namespace Aquila::Application;

EditorApplication::EditorApplication(const ApplicationSpec &spec) : Application(spec) {}

EditorApplication::~EditorApplication() = default;

void EditorApplication::OnInit() {
	Aquila::UI::Core::ViewSystem::Init(GetWindow().GetWidth(), GetWindow().GetHeight());
	GetRenderer2D().AddSystem<Aquila::UI::Rendering::ViewRenderingSystem>();

	{
		GLFWwindow *nativeWin = GetWindow().GetNativeWindow();
		Aquila::UI::Core::Clipboard::Init(
			[nativeWin]() -> std::string {
				const char *s = glfwGetClipboardString(nativeWin);
				return s ? s : "";
			},
			[nativeWin](const std::string &t) { glfwSetClipboardString(nativeWin, t.c_str()); });
	}

	Graphics::MaterialFactory::Get()->EnableHotReload(true);

	UI::FontManager::Get().Initialize(GetContext(), Config::GetPreferences().fonts);

	SetupScene();
	SetupEditorUI();
}

void EditorApplication::OnShutdown() {
	m_HierarchyPanel.reset();
	m_ViewportPanel.reset();
	m_InspectorPanel.reset();
	m_ConsolePanel.reset();
	m_TextureCache.reset();

	Aquila::UI::Core::ViewSystem::Shutdown();
	UI::FontManager::Get().Shutdown();
}

void EditorApplication::OnPreRender(f32 deltaTime) {
	Aquila::UI::Core::ViewSystem::Get()->Update(deltaTime);
	Aquila::UI::Core::ViewSystem::Get()->Compute();

	const bool uiDirty = Aquila::UI::Core::ViewSystem::Get()->IsAnyLayerDirty(Aquila::UI::Core::UILayer::ScreenOverlay,
																			  Aquila::UI::Core::UILayer::Editor);

	GetRenderer2D().SetUIDirty(uiDirty);
	if (uiDirty) {
		Aquila::UI::Core::ViewSystem::Get()->ClearLayerDirtyFlags(Aquila::UI::Core::UILayer::ScreenOverlay,
																  Aquila::UI::Core::UILayer::Editor);
	}
}

void EditorApplication::OnEvent(Events::Event &event) {
	Aquila::UI::Core::ViewSystem::Get()->OnEvent(event);
}

void EditorApplication::OnResize(uint32 width, uint32 height) {
	Aquila::UI::Core::ViewSystem::Get()->Resize(width, height);
	if (m_ViewportPanel) {
		m_ViewportPanel->SetTexture(&GetRenderOutput());
	}
}

void EditorApplication::SetupScene() {
	auto *em = GetScene().GetEntityManager();

	auto cam = em->CreateEntity("Camera");
	auto &camComp = cam.AddComponent<CameraComponent>();
	camComp.fov = 60.f;
	camComp.nearPlane = 0.1f;
	camComp.farPlane = 500.f;
	camComp.aspectRatio = static_cast<f32>(GetWindow().GetWidth()) / static_cast<f32>(GetWindow().GetHeight());
	camComp.primary = true;
	cam.GetComponent<TransformComponent>().SetLocalPosition({ 0.f, 1.5f, -5.f });
	GetScene().SetActiveCamera(cam);

	auto litMat = Graphics::MaterialFactory::Get()->Create(GetContext(), SharedConstants::SHADERS_DIR + "Basic.slang",
														   {
															   .type = Graphics::MaterialType::Lit,
															   .colorFormats = { RHI::TextureFormat::RGBA16F },
															   .depthTest = true,
															   .depthWrite = false,
														   });

	auto addCube = [&](const char *name, vec3 pos) {
		auto entity = em->CreateEntity(name);
		auto mesh = CreateRef<Graphics::Resources::Mesh>(name);
		mesh->LoadFromData(Graphics::Resources::Mesh::GenerateCube(0.5f));
		entity.AddComponent<MeshComponent>().SetMesh(mesh);
		entity.GetComponent<TransformComponent>().SetLocalPosition(pos);
		auto &mat = entity.AddComponent<MaterialComponent>(litMat);
		mat.surfaceProperties.albedo = vec4(0.8f, 0.6f, 0.4f, 1.f);
		mat.surfaceProperties.metallic = 0.0f;
		mat.surfaceProperties.roughness = 0.6f;
	};
	addCube("CubeA", { -1.5f, 0.f, 2.f });
	addCube("CubeB", { 1.5f, 0.f, 2.f });
	addCube("Floor", { 0.0f, 0.5f, 2.f });

	{
		auto e = em->CreateEntity("SunLight");
		auto &light = e.AddComponent<LightComponent>(LightComponent::Type::Directional, vec3(1.0f, 0.95f, 0.8f), 2.0f);
		light.SetDirection(glm::normalize(vec3(0.4f, -1.0f, 0.6f)));
	}
	{
		auto e = em->CreateEntity("PointA");
		e.GetComponent<TransformComponent>().SetLocalPosition({ -1.5f, -0.5f, 1.5f });
		auto &light = e.AddComponent<LightComponent>(LightComponent::Type::Point, vec3(1.0f, 0.4f, 0.1f), 5.0f);
		light.SetRange(6.0f);
	}
	{
		auto e = em->CreateEntity("PointB");
		e.GetComponent<TransformComponent>().SetLocalPosition({ 1.5f, -0.5f, 1.5f });
		auto &light = e.AddComponent<LightComponent>(LightComponent::Type::Point, vec3(0.2f, 0.5f, 1.0f), 5.0f);
		light.SetRange(6.0f);
	}
}

void EditorApplication::SetupEditorUI() {
	auto &editorCanvas = Aquila::UI::Core::ViewSystem::Get()->GetLayer(Aquila::UI::Core::UILayer::Editor);
	const auto &cfg = Config::GetPreferences();

	m_TextureCache = CreateUnique<Aquila::UI::Core::TextureCache>(GetContext(), cfg.ui.resourcesPath);

	Aquila::UI::StyleParser::LoadFile(cfg.ui.stylePath, editorCanvas.GetStyleSheet());

	Aquila::UI::Core::LayoutLoader loader;
	loader.RegisterFont("regular", UI::FontManager::Get().GetFont("regular"));
	loader.RegisterTextureCache(m_TextureCache.get());
	loader.RegisterWidget("ColorPicker",
						  [this](std::string_view, Aquila::UI::Text::FontAtlas *) -> Unique<Aquila::UI::Core::View> {
							  return CreateUnique<Aquila::UI::Core::ColorPicker>(GetContext());
						  });

	auto root = loader.LoadFile(cfg.ui.layoutPath);
	if (!root) {
		AQUILA_LOG_ERROR("EditorApplication: failed to load editor layout from {}", cfg.ui.layoutPath);
		return;
	}

	Aquila::UI::Core::View *layoutRoot = editorCanvas.GetRoot()->AddChild(std::move(root));

	auto *dockRoot = layoutRoot->FindById("dock-root");
	if (!dockRoot) {
		AQUILA_LOG_ERROR("EditorApplication: dock-root not found in layout");
		return;
	}

	auto dockPanels = EditorDockLayout::Build(dockRoot);

	m_HierarchyPanel = CreateUnique<HierarchyPanel>(*GetScene().GetEntityManager());
	m_ViewportPanel = CreateUnique<ViewportPanel>(GetRenderOutput());
	m_InspectorPanel = CreateUnique<InspectorPanel>(GetContext());
	m_ConsolePanel = CreateUnique<ConsolePanel>();

	m_HierarchyPanel->Build(dockPanels.hierarchy, layoutRoot);
	m_ViewportPanel->Build(dockPanels.viewport, layoutRoot);
	m_InspectorPanel->Build(dockPanels.inspector, layoutRoot);
	m_ConsolePanel->Build(dockPanels.console, layoutRoot);

	m_HierarchyPanel->SetOnEntitySelected([this](Entity entity) { m_InspectorPanel->ShowEntity(entity); });

	WireMenubar(layoutRoot);

	editorCanvas.ReloadStyles();
}

void EditorApplication::WireMenubar(Aquila::UI::Core::View *layoutRoot) {
	auto wireBtn = [&](const char *id, const char *action) {
		if (auto *v = layoutRoot->FindById(id)) {
			if (auto *btn = dynamic_cast<Aquila::UI::Core::Button *>(v)) {
				btn->SetOnClick([action] { AQUILA_LOG_INFO("EditorApplication: {}", action); });
			}
		}
	};
	wireBtn("btn-play", "Play");
	wireBtn("btn-pause", "Pause");
	wireBtn("btn-stop", "Stop");

	using MenuItem = std::pair<const char *, std::function<void()>>;
	auto makeMenu = [&](const char *btnId, std::initializer_list<MenuItem> items) {
		auto menuUniq = CreateUnique<Aquila::UI::Core::ContextMenu>();
		auto *menu = static_cast<Aquila::UI::Core::ContextMenu *>(layoutRoot->AddChild(std::move(menuUniq)));
		for (auto &[label, cb] : items) {
			menu->AddItem(label, cb);
		}
		if (auto *v = layoutRoot->FindById(btnId)) {
			if (auto *btn = dynamic_cast<Aquila::UI::Core::Button *>(v)) {
				btn->SetOnClick([menu, btn] { menu->OpenAt(btn->GetAbsolutePosition()); });
			}
		}
	};

	makeMenu("btn-file",
			 {
				 { "New Scene", [] { AQUILA_LOG_INFO("File: New Scene"); } },
				 { "Open Scene...", [] { AQUILA_LOG_INFO("File: Open Scene"); } },
				 { "Save Scene", [] { AQUILA_LOG_INFO("File: Save Scene"); } },
				 { "Exit", [] { AQUILA_LOG_INFO("File: Exit"); } },
			 });
	makeMenu("btn-edit",
			 {
				 { "Undo", [] { AQUILA_LOG_INFO("Edit: Undo"); } },
				 { "Redo", [] { AQUILA_LOG_INFO("Edit: Redo"); } },
				 { "Duplicate", [] { AQUILA_LOG_INFO("Edit: Duplicate"); } },
				 { "Delete", [] { AQUILA_LOG_INFO("Edit: Delete"); } },
			 });
	makeMenu("btn-view",
			 {
				 { "Toggle Grid", [] { AQUILA_LOG_INFO("View: Toggle Grid"); } },
				 { "Toggle Stats", [] { AQUILA_LOG_INFO("View: Toggle Stats"); } },
				 { "Fullscreen", [] { AQUILA_LOG_INFO("View: Fullscreen"); } },
			 });
	makeMenu("btn-window",
			 {
				 { "Scene", [] { AQUILA_LOG_INFO("Window: Scene"); } },
				 { "Inspector", [] { AQUILA_LOG_INFO("Window: Inspector"); } },
				 { "Console", [] { AQUILA_LOG_INFO("Window: Console"); } },
			 });
}

} // namespace Editor

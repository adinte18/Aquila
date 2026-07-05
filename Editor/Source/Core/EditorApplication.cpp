#include "Core/EditorApplication.h"

#include "UI/Managers/FontManager.h"
#include "UI/Panels/ConsolePanel.h"
#include "UI/Panels/HierarchyPanel.h"
#include "UI/Debug/FloatingPanelWindow.h"
#include "UI/Debug/UIDebugPanel.h"
#include "UI/Debug/UIDebugWindow.h"
#include "UI/Debug/WidgetGalleryWindow.h"
#include "UI/Debug/PickerOverlay.h"
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
#include "Aquila/UI/Core/CanvasManager.h"
#include "Aquila/UI/Rendering/ViewRenderingSystem.h"
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/ColorPicker.h"
#include "Aquila/UI/Widgets/ContextMenu.h"
#include "Aquila/UI/Widgets/DockNode.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/DockSpace.h"
#include "Aquila/UI/Widgets/DockTypes.h"
#include "Aquila/UI/Widgets/Menubar.h"
#include "Aquila/Application/Events/InputEvent.h"

#include <algorithm>

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;
using namespace Aquila::Application;

EditorApplication::EditorApplication(const ApplicationSpec &spec) : Application(spec) {}

EditorApplication::~EditorApplication() = default;

void EditorApplication::OnInit() {
	Aquila::UI::Core::CanvasManager::Init(GetWindow().GetWidth(), GetWindow().GetHeight());
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
	m_FloatingPanels.clear();
	m_WidgetGalleryWindow.reset();
	m_UIDebugWindow.reset();
	m_UIDebugPanel.reset();
	m_HierarchyPanel.reset();
	m_ViewportPanel.reset();
	m_InspectorPanel.reset();
	m_ConsolePanel.reset();
	m_TextureCache.reset();

	Aquila::UI::Core::CanvasManager::Shutdown();
	UI::FontManager::Get().Shutdown();
}

void EditorApplication::OnPreRender(f32 deltaTime) {
	if (m_ConsolePanel) {
		m_ConsolePanel->FlushPending();
	}
	Aquila::UI::Core::CanvasManager::Get()->Update(deltaTime);
	Aquila::UI::Core::CanvasManager::Get()->Compute();
}

void EditorApplication::OnEvent(Events::Event &event) {
	Events::EventDispatcher dispatcher(event);

	if (m_PickMode) {
		bool consumed = false;
		dispatcher.Dispatch<Events::MouseMovedEvent>([&](Events::MouseMovedEvent &e) {
			auto &editorCanvas = Aquila::UI::Core::CanvasManager::Get()->GetLayer(Aquila::UI::Core::UILayer::Editor);
			Aquila::UI::Core::View *hit = editorCanvas.HitTest({ e.GetX(), e.GetY() });
			if (m_Picker) {
				hit ? m_Picker->SetTarget(hit->GetAbsoluteRect()) : m_Picker->Clear();
			}
			if (m_UIDebugWindow && hit) {
				m_UIDebugWindow->SelectView(hit);
			}
			consumed = true;
			return true;
		});
		dispatcher.Dispatch<Events::MouseButtonPressedEvent>([&](Events::MouseButtonPressedEvent &) {
			m_PickMode = false;
			if (m_Picker) {
				m_Picker->Clear();
			}
			consumed = true;
			return true;
		});
		dispatcher.Dispatch<Events::KeyPressedEvent>([&](Events::KeyPressedEvent &e) {
			if (e.GetKeyCode() == Events::KeyCode::Escape) {
				m_PickMode = false;
				if (m_Picker) {
					m_Picker->Clear();
				}
			}
			consumed = true;
			return true;
		});
		if (consumed) {
			return;
		}
	}

	dispatcher.Dispatch<Events::KeyPressedEvent>([this](Events::KeyPressedEvent &e) {
		if (e.GetKeyCode() == Events::KeyCode::F1 && !e.IsRepeat()) {
			if (m_UIDebugPanel) {
				m_UIDebugPanel->Toggle();
			}
			return true;
		}
		return false;
	});

	Aquila::UI::Core::CanvasManager::Get()->OnEvent(event);
}

void EditorApplication::OnResize(uint32 width, uint32 height) {
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
	auto &editorCanvas = Aquila::UI::Core::CanvasManager::Get()->GetLayer(Aquila::UI::Core::UILayer::Editor);
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

	loader.RegisterCommand("entity.create", [this] {
		auto entity = GetScene().GetEntityManager()->CreateEntity("New Entity");
		if (m_HierarchyPanel) {
			m_HierarchyPanel->AddEntity(entity);
		}
	});
	loader.RegisterCommand("console.clear", [this] {
		if (m_ConsolePanel) {
			m_ConsolePanel->ClearAll();
		}
	});

	auto root = loader.LoadFile(cfg.ui.layoutPath);
	if (!root) {
		AQUILA_LOG_ERROR("EditorApplication: failed to load editor layout from {}", cfg.ui.layoutPath);
		return;
	}

	Aquila::UI::Core::View *layoutRoot = editorCanvas.GetRoot()->AddChild(std::move(root));

	m_DockSpace = layoutRoot->FindById<Aquila::UI::Core::DockSpace>("editor-dock");
	auto *hierarchyPanel = layoutRoot->FindById<Aquila::UI::Core::DockPanel>("panel-hierarchy");
	auto *viewportPanel = layoutRoot->FindById<Aquila::UI::Core::DockPanel>("panel-viewport");
	auto *inspectorPanel = layoutRoot->FindById<Aquila::UI::Core::DockPanel>("panel-inspector");
	auto *consolePanel = layoutRoot->FindById<Aquila::UI::Core::DockPanel>("panel-console");
	if (!m_DockSpace || !hierarchyPanel || !viewportPanel || !inspectorPanel || !consolePanel) {
		AQUILA_LOG_ERROR("EditorApplication: editor dock layout not found — check editor.aqlayout");
		return;
	}

	WireDockSpace(m_DockSpace, GetWindow().GetNativeWindow());

	m_HierarchyPanel = CreateUnique<HierarchyPanel>(*GetScene().GetEntityManager());
	m_ViewportPanel = CreateUnique<ViewportPanel>(GetRenderOutput());
	m_InspectorPanel = CreateUnique<InspectorPanel>(GetContext());
	m_ConsolePanel = CreateUnique<ConsolePanel>(m_TextureCache.get());

	m_HierarchyPanel->Build(hierarchyPanel, layoutRoot);
	m_ViewportPanel->Build(viewportPanel, layoutRoot);
	m_InspectorPanel->Build(inspectorPanel, layoutRoot);
	m_ConsolePanel->Build(consolePanel, layoutRoot);

	m_HierarchyPanel->onEntitySelected.Connect([this](Entity entity) { m_InspectorPanel->ShowEntity(entity); });

	WireMenubar(layoutRoot);

	m_UIDebugPanel = CreateUnique<UIDebugPanel>();
	m_UIDebugPanel->Build(layoutRoot, &editorCanvas);

	auto ctxUniq = CreateUnique<Aquila::UI::Core::ContextMenu>();
	auto *ctx = static_cast<Aquila::UI::Core::ContextMenu *>(layoutRoot->AddChild(std::move(ctxUniq)));
	ctx->AddItem("Open UI Inspector", [this] { OpenUIInspectorWindow(); });
	ctx->AddItem("Open Widget Gallery", [this] { OpenWidgetGalleryWindow(); });
	layoutRoot->onContextMenu.Connect([ctx](vec2 pos) { ctx->OpenAt(pos); });
	viewportPanel->onContextMenu.Connect([ctx](vec2 pos) { ctx->OpenAt(pos); });

	m_Picker = editorCanvas.GetRoot()->AddChild<PickerOverlay>();

	editorCanvas.ReloadStyles();
}

void EditorApplication::OpenUIInspectorWindow() {
	if (m_UIDebugWindow) {
		return; // already open
	}

	auto &editorCanvas = Aquila::UI::Core::CanvasManager::Get()->GetLayer(Aquila::UI::Core::UILayer::Editor);
	RenderWindow &rw = CreateSecondaryWindow(800, 600, "Aquila - UI Inspector");

	m_UIDebugWindow = CreateUnique<UIDebugWindow>();
	m_UIDebugWindow->Build(&editorCanvas, 800, 600, Config::GetPreferences().ui.stylePath);

	m_UIDebugWindow->onPickRequested = [this] { StartPick(); };

	UIDebugWindow *win = m_UIDebugWindow.get();
	rw.onUpdate = [win](f32 dt) { win->Update(dt); };
	rw.onRender = [win](auto &batcher, auto &cmd) { win->Render(batcher, cmd); };
	rw.onEvent = [win](Events::Event &event) { win->OnEvent(event); };
	rw.onClose = [this] {
		m_PickMode = false;
		if (m_Picker) {
			m_Picker->Clear();
		}
		m_UIDebugWindow.reset();
	};
}

void EditorApplication::StartPick() {
	if (!m_UIDebugWindow) {
		return;
	}
	m_UIDebugWindow->Refresh();
	m_PickMode = true;
}

void EditorApplication::OpenWidgetGalleryWindow() {
	if (m_WidgetGalleryWindow) {
		return;
	}

	RenderWindow &rw = CreateSecondaryWindow(420, 720, "Aquila - Widget Gallery");

	m_WidgetGalleryWindow = CreateUnique<WidgetGalleryWindow>();
	m_WidgetGalleryWindow->Build(GetContext(), m_TextureCache.get(), 420, 720, Config::GetPreferences().ui.stylePath);

	WidgetGalleryWindow *win = m_WidgetGalleryWindow.get();
	rw.onUpdate = [win](f32 dt) { win->Update(dt); };
	rw.onRender = [win](auto &batcher, auto &cmd) { win->Render(batcher, cmd); };
	rw.onEvent = [win](Events::Event &event) { win->OnEvent(event); };
	rw.onClose = [this] { m_WidgetGalleryWindow.reset(); };
}

void EditorApplication::WireDockSpace(Aquila::UI::Core::DockSpace *dockSpace, GLFWwindow *sourceNative) {
	dockSpace->SetTearOffCallback(
		[this, sourceNative](Unique<Aquila::UI::Core::View> sub, std::string title, vec2 pos) {
			HandleTearOff(sourceNative, std::move(sub), std::move(title), pos);
		});
	dockSpace->SetExternalDragObserver([this, sourceNative](vec2 pos) { PreviewDockTargets(sourceNative, pos); },
									   [this] { ClearDockTargetPreviews(); });
	// For the main dock space this is a no-op — the main window is never in m_FloatingPanels.
	dockSpace->SetEmptiedCallback([this, sourceNative] { CloseFloatingWindow(sourceNative); });
}

void EditorApplication::CloseFloatingWindow(GLFWwindow *native) {
	for (auto &entry : m_FloatingPanels) {
		if (entry.window->window->GetNativeWindow() == native) {
			glfwSetWindowShouldClose(native, GLFW_TRUE);
			return;
		}
	}
}

Aquila::UI::Core::DockSpace *EditorApplication::FindDockTargetAtScreen(vec2 screenPos, GLFWwindow *exclude,
																	   vec2 &outLocal) {
	struct Candidate {
		Aquila::UI::Core::DockSpace *dockSpace;
		GLFWwindow *native;
		float width;
		float height;
	};
	std::vector<Candidate> candidates;
	for (auto &entry : m_FloatingPanels) {
		candidates.push_back({ entry.panel->GetDockSpace(), entry.window->window->GetNativeWindow(),
							   static_cast<float>(entry.window->window->GetWidth()),
							   static_cast<float>(entry.window->window->GetHeight()) });
	}
	candidates.push_back({ m_DockSpace, GetWindow().GetNativeWindow(), static_cast<float>(GetWindow().GetWidth()),
						   static_cast<float>(GetWindow().GetHeight()) });

	for (auto &c : candidates) {
		if (c.native == exclude) {
			continue;
		}
		int cx = 0, cy = 0;
		glfwGetWindowPos(c.native, &cx, &cy);
		const vec2 local = { screenPos.x - static_cast<float>(cx), screenPos.y - static_cast<float>(cy) };
		if (local.x >= 0.f && local.y >= 0.f && local.x < c.width && local.y < c.height) {
			outLocal = local;
			return c.dockSpace;
		}
	}
	return nullptr;
}

void EditorApplication::PreviewDockTargets(GLFWwindow *sourceNative, vec2 sourceLocal) {
	int sx = 0, sy = 0;
	glfwGetWindowPos(sourceNative, &sx, &sy);
	const vec2 screen = { static_cast<float>(sx) + sourceLocal.x, static_cast<float>(sy) + sourceLocal.y };

	ClearDockTargetPreviews();

	vec2 targetLocal{ 0.f, 0.f };
	if (Aquila::UI::Core::DockSpace *target = FindDockTargetAtScreen(screen, sourceNative, targetLocal)) {
		target->PreviewExternalDrag(targetLocal);
	}
}

void EditorApplication::ClearDockTargetPreviews() {
	m_DockSpace->ClearExternalDrag();
	for (auto &entry : m_FloatingPanels) {
		entry.panel->GetDockSpace()->ClearExternalDrag();
	}
}

void EditorApplication::HandleTearOff(GLFWwindow *sourceNative, Unique<Aquila::UI::Core::View> content,
									  std::string title, vec2 sourceLocal) {
	ClearDockTargetPreviews();

	int sx = 0, sy = 0;
	glfwGetWindowPos(sourceNative, &sx, &sy);
	const vec2 screen = { static_cast<float>(sx) + sourceLocal.x, static_cast<float>(sy) + sourceLocal.y };

	// A release inside the source window's own bounds floats the panel — the source sits on top
	int sw = 0, sh = 0;
	glfwGetWindowSize(sourceNative, &sw, &sh);
	const bool insideSource = sourceLocal.x >= 0.f && sourceLocal.y >= 0.f && sourceLocal.x < static_cast<float>(sw) &&
		sourceLocal.y < static_cast<float>(sh);

	vec2 targetLocal{ 0.f, 0.f };
	Aquila::UI::Core::DockSpace *target =
		insideSource ? nullptr : FindDockTargetAtScreen(screen, sourceNative, targetLocal);
	const bool docked = target && target->TryDockExternal(content, title, targetLocal);

	if (!docked) {
		SpawnFloatingPanel(std::move(content), std::move(title), screen);
	}

	// A floating window drained of its last tab has nothing left to show — close it.
	for (auto &entry : m_FloatingPanels) {
		if (entry.window->window->GetNativeWindow() == sourceNative && !entry.panel->HasContent()) {
			CloseFloatingWindow(sourceNative);
			break;
		}
	}
}

void EditorApplication::SpawnFloatingPanel(Unique<Aquila::UI::Core::View> panelSubtree, std::string title,
										   vec2 screenPos) {
	RenderWindow &rw = CreateSecondaryWindow(800, 600, title);
	glfwSetWindowPos(rw.window->GetNativeWindow(), static_cast<int>(screenPos.x) - 60,
					 static_cast<int>(screenPos.y) - 12);

	auto fpw = CreateUnique<FloatingPanelWindow>();
	fpw->Build(std::move(panelSubtree), title, 800, 600, Config::GetPreferences().ui.stylePath);

	FloatingPanelWindow *panel = fpw.get();
	RenderWindow *window = &rw;

	rw.onUpdate = [panel](f32 dt) { panel->Update(dt); };
	rw.onRender = [panel](auto &batcher, auto &cmd) { panel->Render(batcher, cmd); };
	rw.onEvent = [panel](Events::Event &event) { panel->OnEvent(event); };
	rw.onClose = [this, panel] { OnFloatingClosed(panel); };

	WireDockSpace(panel->GetDockSpace(), window->window->GetNativeWindow());

	m_FloatingPanels.push_back({ std::move(fpw), window });
}

void EditorApplication::OnFloatingClosed(FloatingPanelWindow *panel) {
	auto it = std::find_if(m_FloatingPanels.begin(), m_FloatingPanels.end(),
						   [panel](const FloatingEntry &e) { return e.panel.get() == panel; });
	if (it == m_FloatingPanels.end()) {
		return;
	}

	while (panel->HasContent()) {
		auto content = panel->DetachContent();
		if (!content) {
			break;
		}
		DockBackToCenter(std::move(content), panel->GetTitle());
	}

	m_FloatingPanels.erase(it);
}

void EditorApplication::DockBackToCenter(Unique<Aquila::UI::Core::View> content, const std::string &title) {
	if (!content || !m_DockSpace) {
		return;
	}

	Aquila::UI::Core::DockNode *node = m_DockSpace->GetRootNode()->HitTestNode(m_DockSpace->GetAbsoluteRect().Center());
	if (!node) {
		return;
	}
	node->AcceptPanel(std::move(content), title, Aquila::UI::Core::DropZone::Center);
}

void EditorApplication::WireMenubar(Aquila::UI::Core::View *layoutRoot) {
	auto wireBtn = [&](const char *id, const char *action) {
		if (auto *v = layoutRoot->FindById(id)) {
			if (auto *btn = dynamic_cast<Aquila::UI::Core::Button *>(v)) {
				btn->onClick.Connect([action] { AQUILA_LOG_INFO("EditorApplication: {}", action); });
			}
		}
	};
	wireBtn("btn-play", "Play");
	wireBtn("btn-pause", "Pause");
	wireBtn("btn-stop", "Stop");

	auto *menuBarView = layoutRoot->FindById("main-menubar");
	auto *menuBar = dynamic_cast<Aquila::UI::Core::MenuBar *>(menuBarView);
	if (!menuBar) {
		return;
	}

	menuBar->SetOverlayRoot(layoutRoot);

	auto *fileMenu = menuBar->AddMenu("File");
	fileMenu->AddItem("Exit", [this] { Close(); });

	auto *windowMenu = menuBar->AddMenu("Window");
	windowMenu->AddItem("UI Inspector", [this] { OpenUIInspectorWindow(); });
	windowMenu->AddItem("Widget Gallery", [this] { OpenWidgetGalleryWindow(); });
	windowMenu->AddItem("Hierarchy", [] { AQUILA_LOG_INFO("Window: Hierarchy"); });
	windowMenu->AddItem("Inspector", [] { AQUILA_LOG_INFO("Window: Inspector"); });
	windowMenu->AddItem("Viewport", [] { AQUILA_LOG_INFO("Window: Viewport"); });
	windowMenu->AddItem("Console", [] { AQUILA_LOG_INFO("Window: Console"); });
}

} // namespace Editor

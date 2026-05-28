#include "Core/EditorApplication.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Graphics/Resources/Mesh.h"
#include "Aquila/Graphics/Material/MaterialFactory.h"
#include "Aquila/UI/Core/ViewSystem.h"
#include "Aquila/UI/Core/Clipboard.h"
#include "Aquila/UI/Core/FontRegistry.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Rendering/ViewRenderingSystem.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Image.h"
#include "Aquila/UI/Widgets/ColorPicker.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;
using namespace Aquila::Application;

EditorApplication::EditorApplication(const ApplicationSpec &spec) : Application(spec) {}

void EditorApplication::OnInit() {
	UI::Core::ViewSystem::Init(GetWindow().GetWidth(), GetWindow().GetHeight());
	GetRenderer2D().AddSystem<UI::Rendering::ViewRenderingSystem>();

	{
		GLFWwindow *nativeWin = GetWindow().GetNativeWindow();
		UI::Core::Clipboard::Init(
			[nativeWin]() -> std::string {
				const char *s = glfwGetClipboardString(nativeWin);
				return s ? s : "";
			},
			[nativeWin](const std::string &t) { glfwSetClipboardString(nativeWin, t.c_str()); });
	}

	Graphics::MaterialFactory::Get()->EnableHotReload(true);

	SetupScene();
	SetupEditorUI();
}

void EditorApplication::OnShutdown() {
	UI::Core::ViewSystem::Shutdown();
	for (auto &font : m_Fonts) {
		font.reset();
	}
	for (auto &cache : m_TextureCaches) {
		cache.reset();
	}
}

void EditorApplication::OnPreRender(f32 deltaTime) {
	UI::Core::ViewSystem::Get()->Update(deltaTime);
	UI::Core::ViewSystem::Get()->Compute();

	const bool uiDirty =
		UI::Core::ViewSystem::Get()->IsAnyLayerDirty(UI::Core::UILayer::ScreenOverlay, UI::Core::UILayer::Editor);

	GetRenderer2D().SetUIDirty(uiDirty);
	if (uiDirty) {
		UI::Core::ViewSystem::Get()->ClearLayerDirtyFlags(UI::Core::UILayer::ScreenOverlay, UI::Core::UILayer::Editor);
	}
}

void EditorApplication::OnEvent(Events::Event &event) {
	UI::Core::ViewSystem::Get()->OnEvent(event);
}

void EditorApplication::OnResize(uint32 width, uint32 height) {
	UI::Core::ViewSystem::Get()->Resize(width, height);
	if (m_ViewportImage) {
		m_ViewportImage->SetTexture(&GetRenderOutput());
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
	auto &editorCanvas = UI::Core::ViewSystem::Get()->GetLayer(UI::Core::UILayer::Editor);

	m_TextureCaches.push_back(CreateUnique<UI::Core::TextureCache>(GetContext(), "/resources"));

	auto addFont = [&](const char *name, const char *path) {
		m_Fonts.push_back(UI::Text::FontAtlas::CreateFromFile(GetContext(), path, 48.f));
		UI::Core::FontRegistry::Register(name, m_Fonts.back().get());
	};
	addFont("regular", "/resources/Engine/Fonts/Lexend/Lexend-Regular.ttf");
	addFont("thin", "/resources/Engine/Fonts/Lexend/Lexend-Thin.ttf");
	addFont("medium", "/resources/Engine/Fonts/Lexend/Lexend-Medium.ttf");
	addFont("bold", "/resources/Engine/Fonts/Lexend/Lexend-Bold.ttf");

	UI::StyleParser::LoadFile("/resources/Engine/UI/widget_test.aqstyle", editorCanvas.GetStyleSheet());

	UI::Core::LayoutLoader loader;
	loader.RegisterFont("regular", m_Fonts[0].get());
	loader.RegisterTextureCache(m_TextureCaches.back().get());
	loader.RegisterWidget("ColorPicker", [this](std::string_view, UI::Text::FontAtlas *) -> Unique<UI::Core::View> {
		return CreateUnique<UI::Core::ColorPicker>(GetContext());
	});

	auto root = loader.LoadFile("/resources/Engine/UI/widget_test.aqlayout");
	if (!root) {
		AQUILA_LOG_ERROR("EditorApplication: failed to load editor layout");
		return;
	}

	if (auto *view = root->FindById("btn-click")) {
		if (auto *btn = dynamic_cast<UI::Core::Button *>(view)) {
			btn->SetOnClick([]() { AQUILA_LOG_INFO("Button clicked!"); });
		}
	}

	if (auto *view = root->FindById("viewport")) {
		if (auto *img = dynamic_cast<UI::Core::Image *>(view)) {
			img->SetTexture(&GetRenderOutput());
			m_ViewportImage = img;
		}
	}

	editorCanvas.GetRoot()->AddChild(std::move(root));
	editorCanvas.ReloadStyles();
}

} // namespace Editor

#include "UI/Panels/MaterialPreviewSystem.h"

#include "Aquila/Assets/AssetManager.h"
#include "Aquila/Core/Application.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"

namespace Editor {

MaterialPreviewSystem::~MaterialPreviewSystem() {}

void MaterialPreviewSystem::Initialize(Aquila::Core::Application &app) {
	if (m_Initialized)
		return;

	m_App = &app;

	m_PreviewCamera = CreateUnique<Aquila::Rendering::Camera>();
	m_PreviewCamera->SetPerspectiveProjection(45.0f, 1.0f, 0.1f, 100.0f);
	m_PreviewCamera->SetViewYXZ(glm::vec3(0.0f, 0.0f, -3.5f), glm::vec3(0.0f, 0.0f, 0.0f));

	CreatePreviewScene(app);

	m_Initialized = true;

	AQUILA_LOG_INFO("MaterialPreviewSystem initialized");
}

void MaterialPreviewSystem::Shutdown() {
	if (!m_Initialized)
		return;

	if (m_PreviewEntity.IsValid()) {
		m_PreviewEntity.Kill();
	}

	if (m_DirectionalLight.IsValid()) {
		m_DirectionalLight.Kill();
	}

	m_CurrentMaterial.reset();
	m_PreviewScene = nullptr;
	m_PreviewCamera.reset();

	m_Initialized = false;
	m_App = nullptr;

	AQUILA_LOG_INFO("MaterialPreviewSystem shutdown");
}

void MaterialPreviewSystem::CreatePreviewScene(Aquila::Core::Application &app) {
	auto &assetManager = app.GetAssetManager();

	m_PreviewScene = assetManager.CreateScene("MaterialPreview");
	if (!m_PreviewScene) {
		AQUILA_LOG_ERROR("Failed to create preview scene");
		return;
	}

	m_PreviewScene->OnStart();

	m_PreviewEntity = m_PreviewScene->GetEntityManager()->CreateEntity("PreviewMesh");
	if (!m_PreviewEntity.IsValid()) {
		AQUILA_LOG_ERROR("Failed to create preview entity");
		return;
	}

	m_PreviewCamera->SetOrbitTarget(
		m_PreviewEntity.GetComponent<Aquila::SceneManagement::Components::TransformComponent>().GetLocalPosition());

	if (!m_PreviewEntity.HasComponent<Aquila::SceneManagement::Components::TransformComponent>()) {
		m_PreviewEntity.AddComponent<Aquila::SceneManagement::Components::TransformComponent>();
	}

	auto &transform = m_PreviewEntity.GetOrEmplace<Aquila::SceneManagement::Components::TransformComponent>();
	transform.SetLocalPosition(glm::vec3(0.0f, 0.0f, 0.0f));
	transform.SetLocalScale(glm::vec3(1.0f));
	transform.SetLocalRotation(glm::vec3(0.0f));

	auto &meshComp = m_PreviewEntity.GetOrEmplace<Aquila::SceneManagement::Components::MeshComponent>();
	meshComp.data = assetManager.CreateProceduralMesh("sphere", "preview_sphere");

	if (!meshComp.data) {
		AQUILA_LOG_ERROR("Failed to create preview sphere mesh");
		return;
	}

	m_PreviewEntity.GetOrEmplace<Aquila::SceneManagement::Components::MaterialComponent>();

	if (m_PreviewEntity.HasComponent<Aquila::SceneManagement::Components::MetadataComponent>()) {
		auto &meta = m_PreviewEntity.GetOrEmplace<Aquila::SceneManagement::Components::MetadataComponent>();
		meta.SetVisible(true);
	}

	m_DirectionalLight = m_PreviewScene->GetEntityManager()->CreateEntity("PreviewLight");
	if (!m_DirectionalLight.IsValid()) {
		AQUILA_LOG_ERROR("Failed to create preview light");
		return;
	}

	if (!m_DirectionalLight.HasComponent<Aquila::SceneManagement::Components::TransformComponent>()) {
		m_DirectionalLight.GetOrEmplace<Aquila::SceneManagement::Components::TransformComponent>();
	}

	auto &lightTransform = m_DirectionalLight.GetOrEmplace<Aquila::SceneManagement::Components::TransformComponent>();
	lightTransform.SetLocalPosition(glm::vec3(0.0f, 1000.0f, 0.0f));
	lightTransform.SetLocalRotation(glm::vec3(glm::radians(-45.0f), glm::radians(-45.0f), 0.0f));

	auto &light = m_DirectionalLight.GetOrEmplace<Aquila::SceneManagement::Components::LightComponent>();
	light.m_Type = Aquila::SceneManagement::Components::LightComponent::Type::Directional;
	light.m_Color = glm::vec3(1.0f, 1.0f, 1.0f);
	light.m_Intensity = 1.5f;
	light.m_Direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.5f));
	light.m_IsActive = true;

	if (m_DirectionalLight.HasComponent<Aquila::SceneManagement::Components::MetadataComponent>()) {
		auto &meta = m_DirectionalLight.GetOrEmplace<Aquila::SceneManagement::Components::MetadataComponent>();
		meta.SetVisible(true);
	}

	AQUILA_LOG_INFO("Preview scene created successfully");
}

void MaterialPreviewSystem::SetMaterial(const Ref<Aquila::Graphics::Material::Material> &material) {
	if (!m_Initialized || !m_PreviewEntity.IsValid()) {
		return;
	}

	m_CurrentMaterial = material;
	if (m_PreviewEntity.HasComponent<Aquila::SceneManagement::Components::MaterialComponent>()) {
		auto &matComp = m_PreviewEntity.GetOrEmplace<Aquila::SceneManagement::Components::MaterialComponent>();
		matComp.SetMaterial(material);
	}

	AQUILA_LOG_DEBUG("Preview material set to: {}", material ? material->name : "None");
}

void MaterialPreviewSystem::SetPreviewMesh(PreviewMesh mesh) {
	if (!m_Initialized || !m_PreviewEntity.IsValid()) {
		AQUILA_LOG_CRITICAL("Entity is not valid or system not initialized");
		return;
	}
	if (m_CurrentMesh == mesh) {
		AQUILA_LOG_CRITICAL("Current mesh is the same as the one you are trying to load");
		return;
	}

	m_CurrentMesh = mesh;
	LoadPreviewMesh(mesh);

	AQUILA_LOG_DEBUG("Preview mesh changed to: {}", static_cast<int>(mesh));
}

void MaterialPreviewSystem::LoadPreviewMesh(const PreviewMesh mesh) {
	AQUILA_LOG_INFO("LoadPreviewMesh called for mesh type: {}", static_cast<int>(mesh));

	if (!m_PreviewEntity.IsValid()) {
		AQUILA_LOG_ERROR("Cannot load preview mesh: entity is invalid");
		return;
	}

	if (!m_PreviewEntity.HasComponent<Aquila::SceneManagement::Components::MeshComponent>()) {
		AQUILA_LOG_ERROR("Cannot load preview mesh: no MeshComponent");
		return;
	}

	if (m_App == nullptr) {
		AQUILA_LOG_ERROR("Cannot load preview mesh: no app reference");
		return;
	}

	auto &meshComp = m_PreviewEntity.GetComponent<Aquila::SceneManagement::Components::MeshComponent>();
	auto &assetManager = m_App->GetAssetManager();

	auto oldMesh = meshComp.data;

	Ref<Aquila::Graphics::Resources::Mesh> newMesh = nullptr;
	switch (mesh) {
	case PreviewMesh::Sphere:
		newMesh = assetManager.CreateProceduralMesh("sphere", "preview_sphere");
		break;
	case PreviewMesh::Cube:
		newMesh = assetManager.CreateProceduralMesh("cube", "preview_cube");
		break;
	case PreviewMesh::Cylinder:
		newMesh = assetManager.CreateProceduralMesh("cylinder", "preview_cylinder");
		break;
	case PreviewMesh::Plane:
		newMesh = assetManager.CreateProceduralMesh("plane", "preview_plane");
		break;
	}

	meshComp.SetMesh(newMesh);

	if (!meshComp.data) {
		AQUILA_LOG_ERROR("Failed to create preview mesh from AssetManager");
		return;
	}
	if (m_PreviewEntity.HasComponent<Aquila::SceneManagement::Components::MetadataComponent>()) {
		auto &meta = m_PreviewEntity.GetComponent<Aquila::SceneManagement::Components::MetadataComponent>();
		meta.SetVisible(true);
	}

	if (m_CurrentMaterial && m_PreviewEntity.HasComponent<Aquila::SceneManagement::Components::MaterialComponent>()) {
		auto &matComp = m_PreviewEntity.GetComponent<Aquila::SceneManagement::Components::MaterialComponent>();
		matComp.SetMaterial(m_CurrentMaterial);
	}

	AQUILA_LOG_INFO("Successfully loaded preview mesh: {}", static_cast<int>(mesh));
}

void MaterialPreviewSystem::Update(const f32 deltaTime) {
	if (!m_Initialized || !m_PreviewEntity.IsValid()) {
		return;
	}

	if (m_AutoRotate) {
		m_Rotation += deltaTime * m_RotationSpeed;
		if (m_Rotation >= 360.0f) {
			m_Rotation -= 360.0f;
		}
	}

	if (m_PreviewEntity.HasComponent<Aquila::SceneManagement::Components::TransformComponent>()) {
		auto &transform = m_PreviewEntity.GetOrEmplace<Aquila::SceneManagement::Components::TransformComponent>();
		transform.SetLocalRotation(glm::vec3(0.0f, glm::radians(m_Rotation), 0.0f));
	}
}

void MaterialPreviewSystem::Render() const {
	if (!m_Initialized || !m_App || !m_PreviewScene || !m_PreviewCamera) {
		return;
	}

	auto &renderer = m_App->GetRenderer();
	renderer.RenderSceneBatched(m_PreviewScene, *m_PreviewCamera, m_PreviewWidth, m_PreviewHeight);
}

VkDescriptorSet MaterialPreviewSystem::GetPreviewTexture() const {
	if (!m_Initialized || !m_App || !m_PreviewScene) {
		return VK_NULL_HANDLE;
	}

	auto &renderer = m_App->GetRenderer();
	auto outputTexture = renderer.GetSceneOutputTexture(m_PreviewScene);

	if (outputTexture && outputTexture->GetDescriptorSet() != VK_NULL_HANDLE) {
		return outputTexture->GetDescriptorSet();
	}

	return VK_NULL_HANDLE;
}

void MaterialPreviewSystem::SetRotation(f32 rotation) {
	m_Rotation = rotation;

	if (m_PreviewEntity.IsValid() &&
		m_PreviewEntity.HasComponent<Aquila::SceneManagement::Components::TransformComponent>()) {
		auto &transform = m_PreviewEntity.GetComponent<Aquila::SceneManagement::Components::TransformComponent>();
		transform.SetLocalRotation(glm::vec3(0.0f, glm::radians(m_Rotation), 0.0f));
	}
}

void MaterialPreviewSystem::Resize(uint32 width, uint32 height) {
	if (width == 0 || height == 0) {
		AQUILA_LOG_WARNING("Invalid preview size: {}x{}", width, height);
		return;
	}

	m_PreviewWidth = width;
	m_PreviewHeight = height;

	if (m_PreviewCamera) {
		f32 aspect = static_cast<f32>(width) / static_cast<f32>(height);
		m_PreviewCamera->SetPerspectiveProjection(45.0f, aspect, 0.1f, 100.0f);
	}

	AQUILA_LOG_DEBUG("Preview resized to: {}x{}", width, height);
}

} // namespace Editor

#ifndef EDITOR_MATERIAL_PREVIEW_SYSTEM_H
#define EDITOR_MATERIAL_PREVIEW_SYSTEM_H

#include "Aquila/Core/Application.h"
#include "Aquila/Graphics/Core/Renderer.h"
#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Rendering/Camera.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Scene/Entity.h"

namespace Editor {

enum class PreviewMesh { Sphere, Cube, Cylinder, Plane };

/**
 * @brief Material preview system that renders materials through the standard PBR pipeline
 */
class MaterialPreviewSystem {
  public:
	MaterialPreviewSystem() = default;
	~MaterialPreviewSystem();

	/**
	 * @brief Initialize the preview system with a dedicated scene and rendering context
	 */
	void Initialize(Aquila::Core::Application &app);

	/**
	 * @brief Shutdown and cleanup preview resources
	 */
	void Shutdown();

	/**
	 * @brief Update preview (rotation, animations, etc.)
	 */
	void Update(f32 deltaTime);

	/**
	 * @brief Render the preview scene through the PBR pipeline
	 * This is called automatically during the frame
	 */
	void Render() const;

	/**
	 * @brief Get the preview output texture for ImGui rendering
	 * Returns the final composite output from the render graph
	 */
	VkDescriptorSet GetPreviewTexture() const;

	/**
	 * @brief Set the material to preview
	 */
	void SetMaterial(const Ref<Aquila::Graphics::Material::Material> &material);

	/**
	 * @brief Change the preview mesh
	 */
	void SetPreviewMesh(PreviewMesh mesh);

	/**
	 * @brief Set preview rotation (manual control)
	 */
	void SetRotation(f32 degrees);

	/**
	 * @brief Enable/disable auto rotation
	 */
	void SetAutoRotate(bool enable) { m_AutoRotate = enable; }

	/**
	 * @brief Get preview camera
	 */
	Aquila::Rendering::Camera &GetPreviewCamera() { return *m_PreviewCamera; }

	/**
	 * @brief Check if system is initialized
	 */
	[[nodiscard]] bool IsInitialized() const { return m_Initialized; }

	/**
	 * @brief Resize the preview viewport
	 */
	void Resize(uint32 width, uint32 height);

	void SetRotationSpeed(f32 speed) { m_RotationSpeed = speed; }
	[[nodiscard]] f32 GetRotation() const { return m_Rotation; }
	[[nodiscard]] bool GetAutoRotate() const { return m_AutoRotate; }

  private:
	void CreatePreviewScene(Aquila::Core::Application &app);
	void LoadPreviewMesh(PreviewMesh mesh);

	Aquila::Core::Application *m_App = nullptr;

	Aquila::SceneManagement::Scene *m_PreviewScene = nullptr;
	Aquila::SceneManagement::Entity m_PreviewEntity;
	Aquila::SceneManagement::Entity m_DirectionalLight;

	Unique<Aquila::Rendering::Camera> m_PreviewCamera;

	PreviewMesh m_CurrentMesh = PreviewMesh::Sphere;
	Ref<Aquila::Graphics::Material::Material> m_CurrentMaterial;
	f32 m_Rotation = 0.0f;
	f32 m_RotationSpeed = 45.0f; // degrees per second
	bool m_AutoRotate = true;

	uint32 m_PreviewWidth = 512;
	uint32 m_PreviewHeight = 512;

	bool m_Initialized = false;
};

} // namespace Editor

#endif // EDITOR_MATERIAL_PREVIEW_SYSTEM_H

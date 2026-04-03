// #ifndef AQUILA_APPLICATION_H
// #define AQUILA_APPLICATION_H

// #include "Aquila/Graphics/Material/MaterialSystem.h"
// #include "imgui_impl_glfw.h"
// #include "imgui_impl_vulkan.h"
// #include "Aquila/Core/AquilaCore.h"
// #include "Aquila/Core/Input.h"
// #include "Aquila/Core/Layer.h"
// #include "Aquila/Events/Event.h"
// #include "Aquila/Events/InputEvent.h"
// #include "Aquila/Events/SceneEvent.h"
// #include "Aquila/Events/WindowEvent.h"
// #include "Aquila/Platform/Filesystem/NativeFileSystem.h"
// #include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
// #include "Aquila/Foundation/Timer.h"
// #include "imgui/imgui.h"

// namespace Aquila {
// namespace SceneManagement {
// class SceneManager;
// }
// namespace Assets {
// class AssetManager;
// }
// namespace Core {
// class Window;
// }
// namespace Graphics {
// class Device;
// class Renderer;
// namespace RenderingPipeline {
// class DescriptorAllocator;
// }
// } // namespace Graphics
// namespace Rendering {
// class Camera;
// }
// } // namespace Aquila

// namespace Aquila::Core {

// struct ApplicationConfig {
// 	std::string windowTitle = "Aquila Engine";
// 	uint32 windowWidth = 1920;
// 	uint32 windowHeight = 1080;
// 	std::string assetPath = "Assets";
// };

// class Application {
//   public:
// 	Application(const ApplicationConfig &config);
// 	virtual ~Application();

// 	void Run();
// 	void Render() const;
// 	void Close();

// 	void OnEvent(Event &event);

// 	virtual void BeginUIFrame() {}
// 	virtual void EndUIFrame(VkCommandBuffer commandBuffer) {}

// 	void PushLayer(Unique<Layer> layer);
// 	void PushOverlay(Unique<Layer> overlay);

// 	// Core system accessors for layers
// 	Window &GetWindow() { return *m_Window; }
// 	Graphics::Device &GetDevice() { return *m_Device; }
// 	Graphics::Renderer &GetRenderer() { return *m_Renderer; }
// 	Rendering::Camera &GetEditorCamera() { return *m_EditorCamera; }
// 	Input &GetInput() { return m_Input; }
// 	LayerStack &GetLayerStack() { return m_LayerStack; }
// 	Assets::AssetManager &GetAssetManager() { return *m_AssetManager; }

// 	Graphics::Material::MaterialSystem &GetMaterialSystem() { return *m_MaterialSystem; }

// 	// Const accessors
// 	[[nodiscard]] const Assets::AssetManager &GetAssetManager() const { return *m_AssetManager; }
// 	[[nodiscard]] const Window &GetWindow() const { return *m_Window; }
// 	[[nodiscard]] const Graphics::Device &GetDevice() const { return *m_Device; }
// 	[[nodiscard]] const Graphics::Renderer &GetRenderer() const { return *m_Renderer; }
// 	[[nodiscard]] const Rendering::Camera &GetEditorCamera() const { return *m_EditorCamera; }
// 	[[nodiscard]] const Input &GetInput() const { return m_Input; }
// 	[[nodiscard]] const Platform::Filesystem::VirtualFileSystem &GetVFS() const { return *m_VFS; }

//   protected:
// 	virtual void OnInit() {}
// 	virtual void OnUpdate(f32 deltaTime) {}
// 	virtual void OnRender(VkCommandBuffer commandBuffer) {}
// 	virtual void OnShutdown() {}

//   private:
// 	bool Initialize();
// 	void Update();
// 	void Shutdown();

// 	[[nodiscard]] bool OnWindowClose(WindowCloseEvent &event);
// 	[[nodiscard]] bool OnWindowResize(WindowResizeEvent &event);
// 	[[nodiscard]] bool OnViewportResize(ViewportResizeEvent &event);
// 	[[nodiscard]] bool OnSceneLoaded(SceneLoadedEvent &event);
// 	[[nodiscard]] bool OnEntityCreated(EntityCreatedEvent &event);
// 	[[nodiscard]] bool OnEntityDeleted(EntityDeletedEvent &event);
// 	[[nodiscard]] bool OnEntityToggled(EntityVisibilityToggle &event);

// 	ApplicationConfig m_Config;
// 	bool m_Running;
// 	f32 m_DeltaTime = 0.0f;

// 	Unique<Window> m_Window;
// 	Unique<Graphics::Device> m_Device;
// 	Unique<Graphics::Renderer> m_Renderer;
// 	Unique<Rendering::Camera> m_EditorCamera;
// 	Unique<Utils::Stopwatch> m_Stopwatch;
// 	Unique<Assets::AssetManager> m_AssetManager;
// 	Ref<Graphics::Material::MaterialSystem> m_MaterialSystem;

// 	Platform::Filesystem::VirtualFileSystem *m_VFS;

// 	LayerStack m_LayerStack;
// 	Input m_Input;
// };

// } // namespace Aquila::Core

// #endif // AQUILA_APPLICATION_H

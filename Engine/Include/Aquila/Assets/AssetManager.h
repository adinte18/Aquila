// #pragma once

// #include "Aquila/Assets/AssetLoaderBase.h"
// #include "Aquila/Graphics/Material/Material.h"
// #include "Aquila/Graphics/Material/MaterialSerializer.h"
// #include "Aquila/Graphics/Material/MaterialSystem.h"
// #include "Aquila/Graphics/Resources/Mesh.h"
// #include "Aquila/Graphics/Resources/Texture2D.h"
// #include "Aquila/Scene/Scene.h"
// #include "Aquila/Scene/SceneManager.h"
// #include "Aquila/Foundation/DeletionManager.h"

// namespace Aquila::Assets {

// enum class AssetType { Unknown = 0, Mesh, Texture, Material, Shader, Scene, Model, Audio };

// struct AssetMetadata {
// 	Utils::UUID uuid;
// 	std::string filepath;
// 	AssetType type = AssetType::Unknown;
// 	bool isLoaded = false;
// 	bool isValid = false;
// 	uint64 fileSize = 0;
// 	uint64 lastModified = 0;
// };

// class AssetEntry {
//   public:
// 	Utils::UUID uuid;
// 	AssetMetadata metadata;
// 	AssetType type;
// 	Ref<void> asset; // Type-erased asset pointer
// 	uint32 referenceCount{ 0 };
// 	bool isLoading{ false };

// 	template <typename T> Ref<T> GetAs() const { return std::static_pointer_cast<T>(asset); }

// 	template <typename T> void SetAsset(Ref<T> ptr) { asset = std::static_pointer_cast<void>(ptr); }
// };

// // Main asset manager - refactored for clean async architecture
// class AssetManager {
//   public:
// 	AssetManager(Graphics::Device &device, Graphics::Material::MaterialSystem &materialSystem, uint32 currentFrame);
// 	~AssetManager();

// 	// INITIALIZATION

// 	void Initialize(uint32 jobThreads = 0);
// 	void Shutdown();

// 	// MESH LOADING

// 	// Synchronous load (blocks until ready)
// 	Ref<Graphics::Resources::Mesh> LoadMesh(const std::string &filepath);

// 	// Asynchronous load (returns immediately with handle)
// 	Core::JobHandle<Ref<Graphics::Resources::Mesh>>
// 	LoadMeshAsync(const std::string &filepath, Core::JobPriority priority = Core::JobPriority::Normal);

// 	// Get cached mesh or load if not present
// 	Ref<Graphics::Resources::Mesh> GetMesh(const std::string &filepath);
// 	Ref<Graphics::Resources::Mesh> GetMesh(const Utils::UUID &uuid);

// 	// Procedural meshes (CPU-only, immediate)
// 	Ref<Graphics::Resources::Mesh> CreateProceduralMesh(const std::string &type, const std::string &name = "");

// 	// TEXTURE LOADING

// 	// Synchronous loads
// 	Ref<Graphics::Resources::Texture2D> LoadTexture(const std::string &filepath,
// 													VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);
// 	Ref<Graphics::Resources::Texture2D> LoadHDRTexture(const std::string &filepath);

// 	// Asynchronous loads
// 	Core::JobHandle<Ref<Graphics::Resources::Texture2D>>
// 	LoadTextureAsync(const std::string &filepath, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
// 					 Core::JobPriority priority = Core::JobPriority::Normal);

// 	Core::JobHandle<Ref<Graphics::Resources::Texture2D>>
// 	LoadHDRTextureAsync(const std::string &filepath, Core::JobPriority priority = Core::JobPriority::Normal);

// 	// Get cached texture
// 	Ref<Graphics::Resources::Texture2D> GetTexture(const std::string &filepath);
// 	Ref<Graphics::Resources::Texture2D> GetTexture(const Utils::UUID &uuid);

// 	Ref<Graphics::Resources::Texture2D> TryGetTexture(const std::string &filepath);
// 	Ref<Graphics::Resources::Texture2D> TryGetTexture(const Utils::UUID &uuid);

// 	Ref<Graphics::Resources::Mesh> TryGetMesh(const std::string &filepath);
// 	Ref<Graphics::Resources::Mesh> TryGetMesh(const Utils::UUID &uuid);

// 	bool IsAssetLoading(const std::string &filepath) const;
// 	bool IsAssetLoading(const Utils::UUID &uuid) const;

// 	Ref<Graphics::Shader::ShaderProgram> LoadShader(const std::string &filepath);
// 	Ref<Graphics::Shader::ShaderProgram> TryGetShader(const std::string &filepath);

// 	// Runtime texture creation
// 	Ref<Graphics::Resources::Texture2D>
// 	CreateRenderTarget(uint32 width, uint32 height, VkFormat format,
// 					   VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
// 					   VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

// 	Ref<Graphics::Resources::Texture2D> CreateCubemap(uint32 width, uint32 height, VkFormat format,
// 													  VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
// 																				VK_IMAGE_USAGE_SAMPLED_BIT,
// 													  bool mipmapped = false);

// 	// Fallback textures
// 	Ref<Graphics::Resources::Texture2D> GetFallbackTexture(Graphics::Resources::TextureType type);

// 	// Materials
// 	Ref<Graphics::Material::Material> LoadMaterial(const std::string &filepath);
// 	Ref<Graphics::Material::Material> LoadMaterialAsset(const std::string &filepath);
// 	Ref<Graphics::Material::Material> CreateMaterialInstance(const Ref<Graphics::Material::Material> &assetMaterial);
// 	Ref<Graphics::Material::Material> GetMaterialAsset(const std::string &filepath);
// 	Ref<Graphics::Material::Material> TryGetMaterialAsset(const std::string &filepath);
// 	bool SaveMaterialAsset(const Ref<Graphics::Material::Material> &material, const std::string &filepath);
// 	// BATCH OPERATIONS

// 	// Preload multiple assets asynchronously
// 	void PreloadAssets(const std::vector<std::string> &filepaths, Core::JobPriority priority = Core::JobPriority::Low);
// 	void PreloadDirectory(const std::string &directory, bool recursive = false,
// 						  Core::JobPriority priority = Core::JobPriority::Low);

// 	// Wait for all pending loads
// 	void WaitForAllLoads();

// 	// ASSET MANAGEMENT

// 	// Check asset status
// 	bool IsAssetLoaded(const Utils::UUID &uuid) const;
// 	bool IsAssetLoaded(const std::string &filepath) const;
// 	AssetLoadState GetAssetState(const Utils::UUID &uuid) const;
// 	AssetLoadState GetAssetState(const std::string &filepath) const;

// 	// Get metadata
// 	AssetMetadata GetAssetMetadata(const Utils::UUID &uuid) const;
// 	AssetMetadata GetAssetMetadata(const std::string &filepath) const;

// 	// Unload assets
// 	void UnloadAsset(const Utils::UUID &uuid);
// 	void UnloadAsset(const std::string &filepath);
// 	void UnloadAllAssets();

// 	// Reload (hot-reload support)
// 	void ReloadAsset(const Utils::UUID &uuid);
// 	void ReloadAsset(const std::string &filepath);
// 	void CheckForModifiedAssets(); // Checks file timestamps

// 	// QUERIES

// 	std::vector<Utils::UUID> GetAllAssetsOfType(AssetType type) const;
// 	std::vector<std::string> GetAllAssetPaths() const;

// 	size_t GetAssetCount() const;
// 	size_t GetLoadedAssetCount() const;
// 	size_t GetPendingLoadCount() const;

// 	// CACHE MANAGEMENT

// 	void ClearCache();
// 	void ClearUnusedAssets(); // Remove assets with refcount == 1
// 	size_t GetCacheSize() const;

// 	// SCENE MANAGEMENT (unchanged from original)

// 	SceneManagement::Scene *CreateScene(const std::string &name) const;
// 	SceneManagement::Scene *LoadScene(const std::string &filepath);
// 	SceneManagement::Scene *LoadSceneAsync(const std::string &filepath,
// 										   const Delegate<void(SceneManagement::Scene *)> &onLoaded);
// 	SceneManagement::Scene *GetScene(const Utils::UUID &uuid) const;
// 	SceneManagement::Scene *GetScene(const std::string &name);
// 	SceneManagement::Scene *GetActiveScene() const;

// 	void ActivateScene(const Utils::UUID &uuid);
// 	void ActivateScene(const std::string &name);
// 	void ActivateScene(const SceneManagement::Scene *scene);

// 	bool SaveScene(const Utils::UUID &uuid, const std::string &filepath);
// 	bool SaveActiveScene(const std::string &filepath);

// 	void UnloadScene(const Utils::UUID &uuid);
// 	void RemoveScene(const Utils::UUID &uuid);
// 	void ChangeScene(const Utils::UUID &uuid);

// 	SceneManagement::Scene *DuplicateScene(const Utils::UUID &uuid, const std::string &newName);
// 	std::vector<SceneManagement::Scene *> GetAllScenes() const;
// 	std::vector<SceneManagement::Scene *> GetInactiveScenes() const;
// 	bool IsSceneActive(const Utils::UUID &uuid) const;

// 	// CALLBACKS

// 	void SetOnAssetLoaded(std::function<void(Utils::UUID, AssetType)> callback) { m_OnAssetLoaded = callback; }
// 	void SetOnAssetUnloaded(std::function<void(Utils::UUID, AssetType)> callback) { m_OnAssetUnloaded = callback; }
// 	void SetOnSceneLoaded(std::function<void(SceneManagement::Scene *)> callback) { m_OnSceneLoaded = callback; }
// 	void SetOnSceneActivated(std::function<void(SceneManagement::Scene *)> callback) { m_OnSceneActivated = callback; }
// 	void SetOnSceneDeactivated(std::function<void(SceneManagement::Scene *)> callback) {
// 		m_OnSceneDeactivated = callback;
// 	}
// 	void SetOnSceneDestroyed(std::function<void(SceneManagement::Scene *)> callback) { m_OnSceneDestroyed = callback; }

// 	void QueueDescriptorUpdate(std::function<void()> task) {
// 		std::lock_guard<std::mutex> lock(m_MainThreadMutex);
// 		m_MainThreadTasks.push(std::move(task));
// 	}

// 	// Called from main thread every frame
// 	void ProcessMainThreadTasks() {
// 		std::lock_guard<std::mutex> lock(m_MainThreadMutex);
// 		while (!m_MainThreadTasks.empty()) {
// 			m_MainThreadTasks.front()();
// 			m_MainThreadTasks.pop();
// 		}
// 	}

//   private:
// 	// INTERNAL HELPERS

// 	// Mesh loading internals
// 	Ref<Graphics::Resources::Mesh> LoadMeshCPU(const std::string &filepath);
// 	void UploadMeshGPU(Graphics::Resources::Mesh &mesh, VkCommandBuffer cmd);

// 	// Texture loading internals
// 	Ref<Graphics::Resources::Texture2D> LoadTextureCPU(const std::string &filepath, VkFormat format);
// 	Ref<Graphics::Resources::Texture2D> LoadHDRTextureCPU(const std::string &filepath);
// 	void UploadTextureGPU(Graphics::Resources::Texture2D &texture, VkCommandBuffer cmd);

// 	// Registry management
// 	void RegisterAsset(const std::string &filepath, Ref<void> asset, AssetType type);
// 	Utils::UUID GetOrCreateUUID(const std::string &filepath);

// 	AssetEntry *FindAssetEntry(const Utils::UUID &uuid);
// 	AssetEntry *FindAssetEntry(const std::string &filepath);
// 	const AssetEntry *FindAssetEntry(const Utils::UUID &uuid) const;
// 	const AssetEntry *FindAssetEntry(const std::string &filepath) const;

// 	// File utilities
// 	AssetType GetAssetTypeFromExtension(const std::string &filepath) const;
// 	std::vector<std::string> FindAllAssetFiles(const std::string &directory, bool recursive) const;
// 	uint64 GetFileModificationTime(const std::string &filepath) const;
// 	uint64 GetFileSize(const std::string &filepath) const;

// 	// Fallback initialization
// 	void InitializeFallbackTextures();

// 	// MEMBER VARIABLES

// 	// Core systems
// 	Graphics::Device &m_Device;
// 	Graphics::Material::MaterialSystem &m_MaterialSystem;
// 	uint32 m_CurrentFrame;

// 	// Async loading infrastructure
// 	Unique<Graphics::GPUUploadManager> m_GPUUploadManager;
// 	bool m_Initialized = false;

// 	mutable std::mutex m_RegistryMutex;
// 	std::unordered_map<Utils::UUID, AssetEntry> m_AssetRegistry;
// 	std::unordered_map<std::string, Utils::UUID> m_PathToUUID;

// 	// Deduplication: track assets currently being loaded
// 	mutable std::mutex m_LoadingMutex;
// 	std::unordered_map<std::string, Core::JobHandle<Ref<Graphics::Resources::Mesh>>> m_PendingMeshLoads;
// 	std::unordered_map<std::string, Core::JobHandle<Ref<Graphics::Resources::Texture2D>>> m_PendingTextureLoads;

// 	std::queue<std::function<void()>> m_MainThreadTasks;
// 	std::mutex m_MainThreadMutex;
// 	std::atomic<bool> m_ShuttingDown{ false };
// 	// Fallback textures
// 	std::unordered_map<Graphics::Resources::TextureType, Ref<Graphics::Resources::Texture2D>> m_FallbackTextures;

// 	// Scene management
// 	Unique<SceneManagement::SceneManager> m_SceneManager;

// 	// Callbacks
// 	std::function<void(Utils::UUID, AssetType)> m_OnAssetLoaded;
// 	std::function<void(Utils::UUID, AssetType)> m_OnAssetUnloaded;
// 	std::function<void(SceneManagement::Scene *)> m_OnSceneLoaded;
// 	std::function<void(SceneManagement::Scene *)> m_OnSceneActivated;
// 	std::function<void(SceneManagement::Scene *)> m_OnSceneDeactivated;
// 	std::function<void(SceneManagement::Scene *)> m_OnSceneDestroyed;
// };

// } // namespace Aquila::Assets

// #include "Aquila/Assets/AssetManager.h"

// #include <algorithm>
// #include <utility>
// #include "Aquila/Core/Job.h"
// #include "Aquila/Graphics/Core/Device.h"
// #include "Aquila/Graphics/Material/MaterialSystem.h"
// #include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

// namespace Aquila::Assets {

// AssetManager::AssetManager(Graphics::Device &device, Graphics::Material::MaterialSystem &materialSystem,
// 						   uint32 currentFrame)
// 	: m_Device(device), m_MaterialSystem(materialSystem), m_CurrentFrame(currentFrame),
// 	  m_SceneManager(CreateUnique<SceneManagement::SceneManager>()) {}

// AssetManager::~AssetManager() {
// 	Shutdown();
// }

// void AssetManager::Initialize(uint32_t jobThreads) {
// 	if (m_Initialized) {
// 		AQUILA_LOG_WARNING("AssetManager already initialized");
// 		return;
// 	}

// 	// Initialize job system (global)
// 	Core::JobSystem::Get().Initialize(jobThreads);

// 	// Initialize GPU upload manager (per-device)
// 	m_GPUUploadManager = CreateUnique<Graphics::GPUUploadManager>(m_Device);
// 	m_GPUUploadManager->Initialize();

// 	// Initialize fallback textures
// 	InitializeFallbackTextures();

// 	m_Initialized = true;
// 	AQUILA_LOG_INFO("AssetManager initialized");
// }

// void AssetManager::Shutdown() {
// 	if (!m_Initialized) {
// 		return;
// 	}

// 	AQUILA_LOG_INFO("Shutting down AssetManager...");

// 	//Prevent new work from being queued
// 	m_ShuttingDown.store(true, std::memory_order_release);

// 	// Stop the GPU upload thread FIRST (so async jobs can fail fast)
// 	if (m_GPUUploadManager) {
// 		AQUILA_LOG_INFO("Stopping GPU upload manager...");
// 		m_GPUUploadManager->Shutdown(); // This sets m_Running=false and joins thread
// 		AQUILA_LOG_INFO("GPU upload manager stopped");
// 	}

// 	// Now async jobs will fail when they try to upload
// 	AQUILA_LOG_INFO("Waiting for async jobs to complete... / {} jobs", Core::JobSystem::Get().GetActiveJobCount());
// 	Core::JobSystem::Get().WaitForAll(); // Should complete now since GPU uploads fail fast
// 	AQUILA_LOG_INFO("All async jobs completed");

// 	// Clear pending loads (should be empty now, but just in case)
// 	{
// 		std::lock_guard<std::mutex> lock(m_LoadingMutex);
// 		if (!m_PendingMeshLoads.empty() || !m_PendingTextureLoads.empty()) {
// 			AQUILA_LOG_WARNING("Still had {} mesh loads and {} texture loads pending", m_PendingMeshLoads.size(),
// 							   m_PendingTextureLoads.size());
// 		}
// 		m_PendingMeshLoads.clear();
// 		m_PendingTextureLoads.clear();
// 	}

// 	// Wait for GPU to finish any in-flight work
// 	AQUILA_LOG_INFO("Waiting for GPU to idle...");
// 	m_Device.WaitGraphicsQueueIdle();
// 	AQUILA_LOG_INFO("GPU is idle");

// 	// Clean up resources
// 	if (m_GPUUploadManager) {
// 		m_GPUUploadManager.reset();
// 	}

// 	// Unload all assets
// 	AQUILA_LOG_INFO("Unloading all assets...");
// 	UnloadAllAssets();
// 	AQUILA_LOG_INFO("All assets unloaded");

// 	m_Initialized = false;
// 	m_ShuttingDown.store(false, std::memory_order_release);
// 	AQUILA_LOG_INFO("AssetManager shut down successfully");
// }

// /**
//  * @brief Load a material asset synchronously
//  * Creates a material instance from the asset
//  * @param filepath Path to .aqmat file
//  * @return Material instance (can be modified per-mesh without affecting asset)
//  */
// Ref<Graphics::Material::Material> AssetManager::LoadMaterial(const std::string &filepath) {
// 	std::string instancePath = filepath + "::instance";

// 	// Check if instance already exists in registry
// 	{
// 		std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 		if (auto *entry = FindAssetEntry(instancePath)) {
// 			if (entry->metadata.isLoaded) {
// 				return entry->GetAs<Graphics::Material::Material>();
// 			}
// 		}
// 	}

// 	// Load the base asset
// 	auto materialAsset = LoadMaterialAsset(filepath);
// 	if (!materialAsset) {
// 		AQUILA_LOG_ERROR("LoadMaterial: FALLING BACK for path '{}'", filepath);
// 		return m_MaterialSystem.GetLibrary().GetFallbackMaterial();
// 	}

// 	// Create instance and register it under the instance key
// 	auto instance = CreateMaterialInstance(materialAsset);
// 	if (instance) {
// 		RegisterAsset(instancePath, instance, AssetType::Material);
// 	}

// 	return instance;
// }
// /**
//  * @brief Load a material asset (the shared base material)
//  * This is the actual .aqmat file data, shared across all instances
//  */
// Ref<Graphics::Material::Material> AssetManager::LoadMaterialAsset(const std::string &filepath) {
// 	// Check if already loaded
// 	{
// 		std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 		if (auto *entry = FindAssetEntry(filepath)) {
// 			if (entry->metadata.isLoaded) {
// 				return entry->GetAs<Graphics::Material::Material>();
// 			}
// 		}
// 	}

// 	auto file = Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(filepath, "r");
// 	if (!file) {
// 		AQUILA_LOG_ERROR("Failed to open material file: {}", filepath);
// 		return nullptr;
// 	}

// 	int64_t fileSize = file->Size();
// 	if (fileSize <= 0) {
// 		AQUILA_LOG_ERROR("Invalid file size: {}", fileSize);
// 		return nullptr;
// 	}

// 	std::vector<char> buffer(fileSize + 1);
// 	size_t bytesRead = file->Read(buffer.data(), fileSize);
// 	buffer[bytesRead] = '\0';

// 	nlohmann::json json;
// 	try {
// 		json = nlohmann::json::parse(buffer.data());
// 	} catch (const std::exception &e) {
// 		AQUILA_LOG_ERROR("Failed to parse material JSON: {}", e.what());
// 		return nullptr;
// 	}

// 	auto materialData = Graphics::Material::MaterialSerializer::ParseFromJson(json);

// 	auto &matLibrary = m_MaterialSystem.GetLibrary();
// 	Ref<Graphics::Material::Material> material = nullptr;

// 	if (!materialData.shaderPath.empty()) {
// 		if (!matLibrary.GetTemplate(materialData.templateName)) {
// 			AQUILA_LOG_INFO("Template '{}' not found, need to create from shader '{}'", materialData.templateName,
// 							materialData.shaderPath);

// 			// Check if shader already loaded (avoid recompiling every frame!)
// 			auto existingShader = TryGetShader(materialData.shaderPath);
// 			Ref<Graphics::Shader::ShaderProgram> shader;

// 			if (existingShader) {
// 				AQUILA_LOG_INFO("Shader '{}' already loaded, reusing it", materialData.shaderPath);
// 				shader = existingShader;
// 			} else {
// 				AQUILA_LOG_INFO("Loading shader '{}'", materialData.shaderPath);
// 				shader = LoadShader(materialData.shaderPath);
// 			}

// 			if (!shader) {
// 				AQUILA_LOG_ERROR("Failed to load shader '{}' for material '{}'", materialData.shaderPath,
// 								 materialData.name);
// 				return nullptr;
// 			}

// 			AQUILA_LOG_INFO("Creating material '{}' from shader (this will register template)", materialData.name);
// 			material = matLibrary.CreateMaterialFromShader(materialData.name, shader);

// 			if (!material) {
// 				AQUILA_LOG_ERROR("Failed to create material from shader");
// 				return nullptr;
// 			}

// 			AQUILA_LOG_INFO("Material '{}' created with template '{}'", materialData.name, materialData.templateName);
// 		}
// 	}

// 	if (!material) {
// 		material = Graphics::Material::MaterialSerializer::CreateFromData(materialData, m_Device, matLibrary);
// 	}

// 	if (!material) {
// 		AQUILA_LOG_ERROR("Failed to create material from data: {}", filepath);
// 		return nullptr;
// 	}

// 	for (const auto &prop : materialData.properties) {
// 		material->SetParameter(prop.name, prop.value);
// 	}

// 	for (const auto &texRef : materialData.textureReferences) {
// 		std::string ext = std::filesystem::path(texRef.texturePath).extension().string();
// 		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

// 		Ref<Graphics::Resources::Texture2D> texture;

// 		if (ext == ".hdr" || ext == ".exr") {
// 			texture = LoadHDRTexture(texRef.texturePath);
// 		} else {
// 			VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
// 			if (texRef.propertyName.find("albedo") != std::string::npos ||
// 				texRef.propertyName.find("Albedo") != std::string::npos ||
// 				texRef.propertyName.find("diffuse") != std::string::npos ||
// 				texRef.propertyName.find("baseColor") != std::string::npos) {
// 				format = VK_FORMAT_R8G8B8A8_SRGB;
// 			}

// 			texture = LoadTexture(texRef.texturePath, format);
// 		}

// 		if (texture) {
// 			material->SetParameter(texRef.propertyName, texture);
// 		} else {
// 			AQUILA_LOG_WARNING("Failed to load texture: {}", texRef.texturePath);
// 		}
// 	}

// 	material->SetRenderState(materialData.renderState);

// 	if (!materialData.shaderPath.empty()) {
// 		material->SetShaderAsset(materialData.shaderPath);
// 	}

// 	RegisterAsset(filepath, material, AssetType::Material);

// 	if (!matLibrary.HasMaterial(material->name)) {
// 		matLibrary.RegisterMaterial(material);

// 		if (auto *scene = GetActiveScene()) {
// 			m_MaterialSystem.AllocateDescriptorsForMaterial(material, scene);
// 		}
// 	}

// 	AQUILA_LOG_INFO("Loaded material asset: {} ({})", material->name, filepath);
// 	return material;
// }

// /**
//  * @brief Create a material instance from an asset
//  * Instances can have per-object parameter overrides
//  */
// Ref<Graphics::Material::Material>
// AssetManager::CreateMaterialInstance(const Ref<Graphics::Material::Material> &assetMaterial) {
// 	if (!assetMaterial) {
// 		return nullptr;
// 	}

// 	auto instance = CreateRef<Graphics::Material::Material>(m_Device, assetMaterial->name + "_instance",
// 															assetMaterial->GetTemplate());

// 	auto properties = assetMaterial->GetAllProperties();
// 	for (const auto &[propName, prop] : properties) {
// 		instance->SetParameter(propName, prop.m_Value);
// 	}

// 	instance->SetRenderState(assetMaterial->GetRenderState());
// 	instance->MarkDirty();
// 	instance->MarkPipelineDirty();
// 	instance->MarkDescriptorDirty();

// 	m_MaterialSystem.GetLibrary().RegisterMaterial(instance);

// 	if (auto *scene = GetActiveScene()) {
// 		m_MaterialSystem.AllocateDescriptorsForMaterial(instance, scene);
// 	}

// 	m_MaterialSystem.UpdatePipeline(instance, 0);

// 	return instance;
// }

// /**
//  * @brief Get or load a material asset
//  */
// Ref<Graphics::Material::Material> AssetManager::GetMaterialAsset(const std::string &filepath) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	if (auto *entry = FindAssetEntry(filepath)) {
// 		return entry->GetAs<Graphics::Material::Material>();
// 	}

// 	return LoadMaterialAsset(filepath);
// }

// /**
//  * @brief Try to get a material asset without loading
//  */
// Ref<Graphics::Material::Material> AssetManager::TryGetMaterialAsset(const std::string &filepath) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	if (auto *entry = FindAssetEntry(filepath)) {
// 		if (entry->metadata.isLoaded) {
// 			return entry->GetAs<Graphics::Material::Material>();
// 		}
// 	}
// 	return nullptr;
// }

// /**
//  * @brief Save a material to an asset file
//  */
// bool AssetManager::SaveMaterialAsset(const Ref<Graphics::Material::Material> &material, const std::string &filepath) {
// 	if (!material) {
// 		AQUILA_LOG_ERROR("Cannot save null material");
// 		return false;
// 	}

// 	bool success = Graphics::Material::MaterialSerializer::SaveToFileVFS(material, filepath);

// 	if (success) {
// 		// Update or register in asset registry
// 		RegisterAsset(filepath, material, AssetType::Material);
// 		AQUILA_LOG_INFO("Saved material asset: {} to {}", material->name, filepath);
// 	}

// 	return success;
// }

// // Scene Management (unchanged)

// SceneManagement::Scene *AssetManager::CreateScene(const std::string &name) const {
// 	return m_SceneManager->CreateScene(name);
// }

// SceneManagement::Scene *AssetManager::LoadScene(const std::string &filepath) {
// 	auto *scene = m_SceneManager->LoadScene(filepath, *this);
// 	if (scene && m_OnSceneLoaded) {
// 		m_OnSceneLoaded(scene);
// 	}
// 	return scene;
// }

// SceneManagement::Scene *AssetManager::LoadSceneAsync(const std::string &filepath,
// 													 const Delegate<void(SceneManagement::Scene *)> &onLoaded) {
// 	return m_SceneManager->LoadSceneAsync(filepath, *this, onLoaded);
// }

// SceneManagement::Scene *AssetManager::GetScene(const Utils::UUID &uuid) const {
// 	return m_SceneManager->GetScene(uuid);
// }

// SceneManagement::Scene *AssetManager::GetScene(const std::string &name) {
// 	return m_SceneManager->GetSceneByName(name);
// }

// void AssetManager::ActivateScene(const Utils::UUID &uuid) {
// 	auto *oldScene = m_SceneManager->GetActiveScene();
// 	m_SceneManager->ActivateScene(uuid);
// 	auto *newScene = m_SceneManager->GetActiveScene();

// 	if (oldScene != newScene) {
// 		if (oldScene && m_OnSceneDeactivated) {
// 			m_OnSceneDeactivated(oldScene);
// 		}
// 		if (newScene && m_OnSceneActivated) {
// 			m_OnSceneActivated(newScene);
// 		}
// 	}
// }

// void AssetManager::ActivateScene(const SceneManagement::Scene *scene) {
// 	if (scene) {
// 		auto *oldScene = m_SceneManager->GetActiveScene();
// 		m_SceneManager->ActivateScene(scene->GetHandle());

// 		if (oldScene != scene) {
// 			if (oldScene && m_OnSceneDeactivated) {
// 				m_OnSceneDeactivated(oldScene);
// 			}
// 			if (m_OnSceneActivated) {
// 				m_OnSceneActivated(const_cast<SceneManagement::Scene *>(scene));
// 			}
// 		}
// 	}
// }

// void AssetManager::ActivateScene(const std::string &name) {
// 	auto *oldScene = m_SceneManager->GetActiveScene();

// 	if (auto *scene = m_SceneManager->GetSceneByName(name)) {
// 		m_SceneManager->ActivateScene(scene->GetHandle());

// 		if (oldScene != scene) {
// 			if (oldScene && m_OnSceneDeactivated) {
// 				m_OnSceneDeactivated(oldScene);
// 			}
// 			if (m_OnSceneActivated) {
// 				m_OnSceneActivated(scene);
// 			}
// 		}
// 	} else {
// 		AQUILA_LOG_ERROR("Scene not found with name: {}", name);
// 	}
// }

// SceneManagement::Scene *AssetManager::GetActiveScene() const {
// 	return m_SceneManager->GetActiveScene();
// }

// bool AssetManager::SaveScene(const Utils::UUID &uuid, const std::string &filepath) {
// 	return m_SceneManager->SaveScene(uuid, filepath);
// }

// bool AssetManager::SaveActiveScene(const std::string &filepath) {
// 	return m_SceneManager->SaveActiveScene(filepath);
// }

// SceneManagement::Scene *AssetManager::DuplicateScene(const Utils::UUID &uuid, const std::string &newName) {
// 	return m_SceneManager->DuplicateScene(uuid, *this, newName);
// }

// std::vector<SceneManagement::Scene *> AssetManager::GetAllScenes() const {
// 	return m_SceneManager->GetAllScenes();
// }

// std::vector<SceneManagement::Scene *> AssetManager::GetInactiveScenes() const {
// 	return m_SceneManager->GetInactiveScenes();
// }

// bool AssetManager::IsSceneActive(const Utils::UUID &uuid) const {
// 	return m_SceneManager->IsSceneActive(uuid);
// }

// void AssetManager::UnloadScene(const Utils::UUID &uuid) {
// 	if (auto *scene = m_SceneManager->GetScene(uuid)) {
// 		if (m_OnSceneDestroyed) {
// 			m_OnSceneDestroyed(scene);
// 		}
// 	}
// 	m_SceneManager->UnloadScene(uuid);
// }

// void AssetManager::RemoveScene(const Utils::UUID &uuid) {
// 	if (auto *scene = m_SceneManager->GetScene(uuid)) {
// 		if (m_OnSceneDestroyed) {
// 			m_OnSceneDestroyed(scene);
// 		}
// 	}
// 	m_SceneManager->RemoveScene(uuid);
// }

// void AssetManager::ChangeScene(const Utils::UUID &uuid) {
// 	auto *oldScene = m_SceneManager->GetActiveScene();
// 	m_SceneManager->ChangeScene(uuid);
// 	auto *newScene = m_SceneManager->GetActiveScene();

// 	if (oldScene != newScene) {
// 		if (oldScene && m_OnSceneDeactivated) {
// 			m_OnSceneDeactivated(oldScene);
// 		}
// 		if (newScene && m_OnSceneActivated) {
// 			m_OnSceneActivated(newScene);
// 		}
// 	}
// }

// // Mesh Management

// Ref<Graphics::Resources::Mesh> AssetManager::LoadMesh(const std::string &filepath) {
// 	// Check if already loaded
// 	{
// 		std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 		if (auto *entry = FindAssetEntry(filepath)) {
// 			if (entry->metadata.isLoaded) {
// 				return entry->GetAs<Graphics::Resources::Mesh>();
// 			}
// 		}
// 	}

// 	// Load synchronously
// 	auto handle = LoadMeshAsync(filepath, Core::JobPriority::High);
// 	return handle.Get(); // Blocks until ready
// }

// Core::JobHandle<Ref<Graphics::Resources::Mesh>> AssetManager::LoadMeshAsync(const std::string &filepath,
// 																			Core::JobPriority priority) {
// 	// Check if shutting down
// 	if (m_ShuttingDown.load(std::memory_order_acquire)) {
// 		std::promise<Ref<Graphics::Resources::Mesh>> promise;
// 		promise.set_exception(std::make_exception_ptr(std::runtime_error("AssetManager is shutting down")));
// 		return Core::JobHandle<Ref<Graphics::Resources::Mesh>>(promise.get_future());
// 	}
// 	{
// 		std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 		if (auto *entry = FindAssetEntry(filepath)) {
// 			if (entry->metadata.isLoaded) {
// 				std::promise<Ref<Graphics::Resources::Mesh>> promise;
// 				promise.set_value(entry->GetAs<Graphics::Resources::Mesh>());
// 				return Core::JobHandle<Ref<Graphics::Resources::Mesh>>(promise.get_future());
// 			}
// 		}
// 	}

// 	std::lock_guard<std::mutex> lock(m_LoadingMutex);
// 	auto it = m_PendingMeshLoads.find(filepath);
// 	if (it != m_PendingMeshLoads.end()) {
// 		AQUILA_LOG_DEBUG("Mesh already loading, returning existing handle: {}", filepath);
// 		return it->second;
// 	}

// 	// Start new async load
// 	auto handle = Core::JobSystem::Get().Schedule(
// 		priority, "LoadMesh_" + filepath, [this, filepath]() -> Ref<Graphics::Resources::Mesh> {
// 			// Check shutdown flag
// 			if (m_ShuttingDown.load(std::memory_order_acquire)) {
// 				throw std::runtime_error("AssetManager shutting down");
// 			}

// 			// PHASE 1: CPU LOADING
// 			auto mesh = LoadMeshCPU(filepath);
// 			if (!mesh) {
// 				throw std::runtime_error("Failed to load mesh: " + filepath);
// 			}

// 			// Check shutdown flag again before GPU upload
// 			if (m_ShuttingDown.load(std::memory_order_acquire)) {
// 				throw std::runtime_error("AssetManager shutting down");
// 			}

// 			// PHASE 2: GPU UPLOAD
// 			auto uploadFuture = m_GPUUploadManager->QueueUpload([mesh](VkCommandBuffer cmd) { mesh->UploadToGPU(cmd); },
// 																"UploadMesh_" + filepath, 0);

// 			uploadFuture.wait();

// 			m_Device.ExecuteGraphicsCommands([&](VkCommandBuffer cmd) {
// 				VkBufferMemoryBarrier barriers[2]{};
// 				uint32 barrierCount = 1;

// 				barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
// 				barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
// 				barriers[0].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
// 				barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
// 				barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
// 				barriers[0].buffer = mesh->GetVertexBuffer();
// 				barriers[0].offset = 0;
// 				barriers[0].size = VK_WHOLE_SIZE;

// 				if (mesh->HasIndexBuffer()) {
// 					barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
// 					barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
// 					barriers[1].dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
// 					barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
// 					barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
// 					barriers[1].buffer = mesh->GetIndexBuffer();
// 					barriers[1].offset = 0;
// 					barriers[1].size = VK_WHOLE_SIZE;
// 					barrierCount = 2;
// 				}

// 				vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0,
// 									 nullptr, barrierCount, barriers, 0, nullptr);
// 			});

// 			mesh->CleanupStagingResources();

// 			// Register as fully loaded
// 			{
// 				std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 				Utils::UUID uuid = GetOrCreateUUID(filepath);

// 				if (auto *entry = FindAssetEntry(uuid)) {
// 					entry->metadata.isLoaded = true;
// 				} else {
// 					AssetEntry newEntry;
// 					newEntry.uuid = uuid;
// 					newEntry.metadata.uuid = uuid;
// 					newEntry.metadata.filepath = filepath;
// 					newEntry.metadata.type = AssetType::Mesh;
// 					newEntry.metadata.isLoaded = true;
// 					newEntry.metadata.isValid = true;
// 					newEntry.metadata.fileSize = GetFileSize(filepath);
// 					newEntry.metadata.lastModified = GetFileModificationTime(filepath);
// 					newEntry.SetAsset(mesh);
// 					newEntry.type = AssetType::Mesh;
// 					newEntry.referenceCount = 1;

// 					m_AssetRegistry[uuid] = std::move(newEntry);
// 				}

// 				if (m_OnAssetLoaded) {
// 					m_OnAssetLoaded(uuid, AssetType::Mesh);
// 				}
// 			}

// 			// Remove from pending loads
// 			{
// 				std::lock_guard<std::mutex> lock(m_LoadingMutex);
// 				m_PendingMeshLoads.erase(filepath);
// 			}

// 			AQUILA_LOG_INFO("Loaded mesh: {}", filepath);
// 			return mesh;
// 		});

// 	m_PendingMeshLoads[filepath] = handle;

// 	return handle;
// }

// Ref<Graphics::Resources::Mesh> AssetManager::LoadMeshCPU(const std::string &filepath) {
// 	auto mesh = CreateRef<Graphics::Resources::Mesh>(m_Device, filepath);
// 	mesh->LoadData(filepath); // CPU-only: loads vertex/index data
// 	return mesh;
// }

// void AssetManager::UploadMeshGPU(Graphics::Resources::Mesh &mesh, VkCommandBuffer cmd) {
// 	mesh.UploadToGPU(cmd); // GPU upload: creates buffers and copies data
// }

// Ref<Graphics::Resources::Mesh> AssetManager::GetMesh(const std::string &filepath) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	if (auto *entry = FindAssetEntry(filepath)) {
// 		return entry->GetAs<Graphics::Resources::Mesh>();
// 	}

// 	// Not found, load it
// 	return LoadMesh(filepath);
// }

// Ref<Graphics::Resources::Mesh> AssetManager::GetMesh(const Utils::UUID &uuid) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	if (auto *entry = FindAssetEntry(uuid)) {
// 		return entry->GetAs<Graphics::Resources::Mesh>();
// 	}
// 	return nullptr;
// }

// Ref<Graphics::Resources::Mesh> AssetManager::CreateProceduralMesh(const std::string &type, const std::string &name) {
// 	std::string meshName = name.empty() ? type : name;
// 	std::string virtualPath = "procedural://" + meshName;

// 	// Check if already exists
// 	{
// 		std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 		if (auto *entry = FindAssetEntry(virtualPath)) {
// 			return entry->GetAs<Graphics::Resources::Mesh>();
// 		}
// 	}

// 	auto mesh = CreateRef<Graphics::Resources::Mesh>(m_Device, meshName);

// 	Graphics::Resources::MeshData meshData;
// 	if (type == "cube") {
// 		meshData = Graphics::Resources::Mesh::GenerateCube(1.0f);
// 	} else if (type == "sphere") {
// 		meshData = Graphics::Resources::Mesh::GenerateSphere(1.0f, 48, 24);
// 	} else if (type == "cylinder") {
// 		meshData = Graphics::Resources::Mesh::GenerateCylinder(1.0f, 2.0f, 32);
// 	} else if (type == "plane") {
// 		meshData = Graphics::Resources::Mesh::GeneratePlane(2.0f, 2.0f, 10, 10);
// 	} else {
// 		AQUILA_LOG_ERROR("Unknown procedural mesh type: {}", type);
// 		return nullptr;
// 	}

// 	mesh->LoadFromData(meshData);

// 	RegisterAsset(virtualPath, mesh, AssetType::Mesh);

// 	AQUILA_LOG_INFO("Created procedural mesh: {}", virtualPath);
// 	return mesh;
// }

// // Texture Management

// Ref<Graphics::Resources::Texture2D> AssetManager::LoadTexture(const std::string &filepath, VkFormat format) {
// 	AQUILA_LOG_INFO("LoadTexture called for: {}", filepath);

// 	if (filepath.empty()) {
// 		return nullptr;
// 	}

// 	// Check if already loaded
// 	{
// 		std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 		if (auto *entry = FindAssetEntry(filepath)) {
// 			if (entry->metadata.isLoaded) {
// 				AQUILA_LOG_INFO("Texture already loaded: {}", filepath);
// 				return entry->GetAs<Graphics::Resources::Texture2D>();
// 			}
// 		}
// 	}

// 	AQUILA_LOG_INFO("Starting async load for: {}", filepath);
// 	auto handle = LoadTextureAsync(filepath, format, Core::JobPriority::High);

// 	AQUILA_LOG_INFO("Waiting for texture to load: {}", filepath);
// 	auto result = handle.Get(); // BLOCKS HERE
// 	AQUILA_LOG_INFO("Texture loaded successfully: {}", filepath);

// 	return result;
// }

// Core::JobHandle<Ref<Graphics::Resources::Texture2D>>
// AssetManager::LoadTextureAsync(const std::string &filepath, VkFormat format, Core::JobPriority priority) {
// 	// Check if shutting down
// 	if (m_ShuttingDown.load(std::memory_order_acquire)) {
// 		std::promise<Ref<Graphics::Resources::Texture2D>> promise;
// 		promise.set_exception(std::make_exception_ptr(std::runtime_error("AssetManager is shutting down")));
// 		return Core::JobHandle<Ref<Graphics::Resources::Texture2D>>(promise.get_future());
// 	}

// 	Utils::UUID uuid{};
// 	{
// 		std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 		uuid = GetOrCreateUUID(filepath);

// 		if (auto *entry = FindAssetEntry(uuid)) {
// 			if (entry->metadata.isLoaded) {
// 				std::promise<Ref<Graphics::Resources::Texture2D>> promise;
// 				promise.set_value(entry->GetAs<Graphics::Resources::Texture2D>());
// 				return Core::JobHandle<Ref<Graphics::Resources::Texture2D>>(promise.get_future());
// 			}
// 		}
// 	}

// 	// Check if already loaded
// 	{
// 		std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 		if (auto *entry = FindAssetEntry(filepath)) {
// 			if (entry->metadata.isLoaded) {
// 				std::promise<Ref<Graphics::Resources::Texture2D>> promise;
// 				promise.set_value(entry->GetAs<Graphics::Resources::Texture2D>());
// 				return Core::JobHandle<Ref<Graphics::Resources::Texture2D>>(promise.get_future());
// 			}
// 		}
// 	}

// 	// Check if currently loading
// 	std::lock_guard<std::mutex> lock(m_LoadingMutex);

// 	auto it = m_PendingTextureLoads.find(filepath);
// 	if (it != m_PendingTextureLoads.end()) {
// 		return it->second;
// 	}

// 	// Start new async load
// 	auto handle = Core::JobSystem::Get().Schedule(
// 		priority, "LoadTexture_" + filepath, [this, filepath, format]() -> Ref<Graphics::Resources::Texture2D> {
// 			// Check shutdown flag
// 			if (m_ShuttingDown.load(std::memory_order_acquire)) {
// 				throw std::runtime_error("AssetManager shutting down");
// 			}

// 			// PHASE 1: CPU LOADING
// 			auto texture = LoadTextureCPU(filepath, format);
// 			if (!texture) {
// 				throw std::runtime_error("Failed to load texture: " + filepath);
// 			}

// 			// Check shutdown flag before GPU work
// 			if (m_ShuttingDown.load(std::memory_order_acquire)) {
// 				throw std::runtime_error("AssetManager shutting down");
// 			}

// 			// PHASE 2: GPU UPLOAD
// 			auto uploadFuture = m_GPUUploadManager->QueueUpload(
// 				[texture](VkCommandBuffer cmd) { texture->UploadToGPU(cmd); }, "UploadTexture_" + filepath);

// 			uploadFuture.wait();

// 			if (texture->GetMipLevels() > 1) {
// 				Graphics::Texture::ImageOperations::GenerateMipmaps(m_Device, texture->GetImage(), texture->GetFormat(),
// 																	texture->GetWidth(), texture->GetHeight(),
// 																	texture->GetMipLevels());
// 			} else {
// 				m_Device.ExecuteGraphicsCommands([&](VkCommandBuffer cmd) {
// 					VkImageMemoryBarrier barrier{};
// 					barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
// 					barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
// 					barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
// 					barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
// 					barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
// 					barrier.image = texture->GetImage();
// 					barrier.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
// 												 .baseMipLevel = 0,
// 												 .levelCount = 1,
// 												 .baseArrayLayer = 0,
// 												 .layerCount = 1 };
// 					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
// 					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
// 					vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
// 										 0, nullptr, 0, nullptr, 1, &barrier);
// 				});
// 			}

// 			texture->CleanupStagingResources();

// 			// Register
// 			{
// 				std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 				Utils::UUID uuid = GetOrCreateUUID(filepath);

// 				AssetEntry entry;
// 				entry.uuid = uuid;
// 				entry.metadata.uuid = uuid;
// 				entry.metadata.filepath = filepath;
// 				entry.metadata.type = AssetType::Texture;
// 				entry.metadata.isLoaded = true;
// 				entry.metadata.isValid = true;
// 				entry.metadata.fileSize = GetFileSize(filepath);
// 				entry.metadata.lastModified = GetFileModificationTime(filepath);
// 				entry.SetAsset(texture);
// 				entry.type = AssetType::Texture;
// 				entry.referenceCount = 1;

// 				m_AssetRegistry[uuid] = std::move(entry);

// 				if (m_OnAssetLoaded) {
// 					m_OnAssetLoaded(uuid, AssetType::Texture);
// 				}
// 			}

// 			{
// 				std::lock_guard<std::mutex> lock(m_LoadingMutex);
// 				m_PendingTextureLoads.erase(filepath);
// 			}

// 			AQUILA_LOG_INFO("Loaded texture: {}", filepath);
// 			return texture;
// 		});

// 	m_PendingTextureLoads[filepath] = handle;

// 	return handle;
// }

// Ref<Graphics::Resources::Texture2D> AssetManager::LoadTextureCPU(const std::string &filepath, VkFormat format) {
// 	auto texture =
// 		Graphics::Resources::Texture2D::Builder(m_Device, filepath).FromFile(filepath, format).DeferGPUUpload().Build();

// 	return texture;
// }

// Ref<Graphics::Resources::Texture2D> AssetManager::LoadHDRTextureCPU(const std::string &filepath) {
// 	auto texture =
// 		Graphics::Resources::Texture2D::Builder(m_Device, filepath).FromHDRFile(filepath).DeferGPUUpload().Build();

// 	return texture;
// }

// void AssetManager::UploadTextureGPU(Graphics::Resources::Texture2D &texture, VkCommandBuffer cmd) {
// 	texture.UploadToGPU(cmd);
// }

// Core::JobHandle<Ref<Graphics::Resources::Texture2D>> AssetManager::LoadHDRTextureAsync(const std::string &filepath,
// 																					   Core::JobPriority priority) {
// 	// Check if already loaded
// 	{
// 		std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 		if (auto *entry = FindAssetEntry(filepath)) {
// 			if (entry->metadata.isLoaded) {
// 				std::promise<Ref<Graphics::Resources::Texture2D>> promise;
// 				promise.set_value(entry->GetAs<Graphics::Resources::Texture2D>());
// 				return Core::JobHandle<Ref<Graphics::Resources::Texture2D>>(promise.get_future());
// 			}
// 		}
// 	}

// 	std::lock_guard<std::mutex> lock(m_LoadingMutex);
// 	auto it = m_PendingTextureLoads.find(filepath);
// 	if (it != m_PendingTextureLoads.end()) {
// 		return it->second;
// 	}

// 	// Start new async load
// 	auto handle = Core::JobSystem::Get().Schedule(
// 		priority, "LoadTexture_" + filepath, [this, filepath]() -> Ref<Graphics::Resources::Texture2D> {
// 			// PHASE 1: CPU LOADING
// 			auto texture = LoadHDRTextureCPU(filepath);

// 			if (!texture) {
// 				throw std::runtime_error("Failed to load texture: " + filepath);
// 			}

// 			// PHASE 2: GPU UPLOAD
// 			auto uploadFuture = m_GPUUploadManager->QueueUpload(
// 				[texture](VkCommandBuffer cmd) { texture->UploadToGPU(cmd); }, "UploadTexture_" + filepath);

// 			uploadFuture.wait();

// 			if (texture->GetMipLevels() > 1) {
// 				Graphics::Texture::ImageOperations::GenerateMipmaps(m_Device, texture->GetImage(), texture->GetFormat(),
// 																	texture->GetWidth(), texture->GetHeight(),
// 																	texture->GetMipLevels());
// 			} else {
// 				m_Device.ExecuteGraphicsCommands([&](VkCommandBuffer cmd) {
// 					VkImageMemoryBarrier barrier{};
// 					barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
// 					barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
// 					barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
// 					barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
// 					barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
// 					barrier.image = texture->GetImage();
// 					barrier.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
// 												 .baseMipLevel = 0,
// 												 .levelCount = 1,
// 												 .baseArrayLayer = 0,
// 												 .layerCount = 1 };
// 					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
// 					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
// 					vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
// 										 0, nullptr, 0, nullptr, 1, &barrier);
// 				});
// 			}

// 			texture->CleanupStagingResources();

// 			// Register
// 			{
// 				std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 				Utils::UUID uuid = GetOrCreateUUID(filepath);

// 				AssetEntry entry;
// 				entry.uuid = uuid;
// 				entry.metadata.uuid = uuid;
// 				entry.metadata.filepath = filepath;
// 				entry.metadata.type = AssetType::Texture;
// 				entry.metadata.isLoaded = true;
// 				entry.metadata.isValid = true;
// 				entry.metadata.fileSize = GetFileSize(filepath);
// 				entry.metadata.lastModified = GetFileModificationTime(filepath);
// 				entry.SetAsset(texture);
// 				entry.type = AssetType::Texture;
// 				entry.referenceCount = 1;

// 				m_AssetRegistry[uuid] = std::move(entry);

// 				if (m_OnAssetLoaded) {
// 					m_OnAssetLoaded(uuid, AssetType::Texture);
// 				}
// 			}

// 			{
// 				std::lock_guard<std::mutex> lock(m_LoadingMutex);
// 				m_PendingTextureLoads.erase(filepath);
// 			}

// 			AQUILA_LOG_INFO("Loaded texture: {}", filepath);
// 			return texture;
// 		});

// 	m_PendingTextureLoads[filepath] = handle;

// 	return handle;
// }

// Ref<Graphics::Resources::Texture2D> AssetManager::LoadHDRTexture(const std::string &filepath) {
// 	{
// 		std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 		if (const auto *entry = FindAssetEntry(filepath)) {
// 			if (entry->metadata.isLoaded) {
// 				AQUILA_LOG_DEBUG("HDR Texture already loaded: {}", filepath);
// 				return entry->GetAs<Graphics::Resources::Texture2D>();
// 			}
// 		}
// 	}

// 	auto handle = LoadHDRTextureAsync(filepath, Core::JobPriority::High);
// 	return handle.Get();
// }

// Ref<Graphics::Shader::ShaderProgram> AssetManager::LoadShader(const std::string &filepath) {
// 	{
// 		std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 		if (auto *entry = FindAssetEntry(filepath)) {
// 			if (entry->metadata.isLoaded) {
// 				return entry->GetAs<Graphics::Shader::ShaderProgram>();
// 			}
// 		}
// 	}

// 	std::string errorLog;
// 	auto shader = CreateRef<Graphics::Shader::ShaderProgram>(m_Device, std::filesystem::path(filepath).stem().string());

// 	if (!shader->AddStageFromSlang(filepath, errorLog)) {
// 		AQUILA_LOG_ERROR("Failed to compile vertex shader {}: {}", filepath, errorLog);
// 		return nullptr;
// 	}

// 	if (!shader->Reflect()) {
// 		AQUILA_LOG_ERROR("Shader did not reflect anything");
// 	}

// 	switch (shader->GetShaderType()) {
// 	case Graphics::Shader::ShaderProgram::ShaderType::Standard: {
// 		shader->SetTargetFormats(Graphics::Helpers::PipelineRenderingFormats::GBuffer());
// 		break;
// 	}
// 	case Graphics::Shader::ShaderProgram::ShaderType::PostProcess: {
// 		break;
// 	}
// 	case Graphics::Shader::ShaderProgram::ShaderType::Compute: {
// 		break;
// 	}
// 	case Graphics::Shader::ShaderProgram::ShaderType::Custom: {
// 		break;
// 	}
// 	default:
// 		break;
// 	}

// 	RegisterAsset(filepath, shader, AssetType::Shader);
// 	AQUILA_LOG_INFO("Loaded shader: {}", filepath);
// 	return shader;
// }

// Ref<Graphics::Shader::ShaderProgram> AssetManager::TryGetShader(const std::string &filepath) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	if (auto *entry = FindAssetEntry(filepath)) {
// 		if (entry->metadata.isLoaded) {
// 			return entry->GetAs<Graphics::Shader::ShaderProgram>();
// 		}
// 	}
// 	return nullptr;
// }

// Ref<Graphics::Resources::Texture2D> AssetManager::GetTexture(const std::string &filepath) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	if (auto *entry = FindAssetEntry(filepath)) {
// 		return entry->GetAs<Graphics::Resources::Texture2D>();
// 	}

// 	return LoadTexture(filepath);
// }

// Ref<Graphics::Resources::Texture2D> AssetManager::GetTexture(const Utils::UUID &uuid) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	if (auto *entry = FindAssetEntry(uuid)) {
// 		return entry->GetAs<Graphics::Resources::Texture2D>();
// 	}
// 	return nullptr;
// }

// Ref<Graphics::Resources::Texture2D> AssetManager::TryGetTexture(const std::string &filepath) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	if (auto *entry = FindAssetEntry(filepath)) {
// 		if (entry->metadata.isLoaded) {
// 			return entry->GetAs<Graphics::Resources::Texture2D>();
// 		}
// 	}
// 	return nullptr;
// }

// Ref<Graphics::Resources::Texture2D> AssetManager::TryGetTexture(const Utils::UUID &uuid) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	if (auto *entry = FindAssetEntry(uuid)) {
// 		if (entry->metadata.isLoaded) {
// 			return entry->GetAs<Graphics::Resources::Texture2D>();
// 		}
// 	}
// 	return nullptr;
// }

// // Same pattern for meshes
// Ref<Graphics::Resources::Mesh> AssetManager::TryGetMesh(const std::string &filepath) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	if (auto *entry = FindAssetEntry(filepath)) {
// 		if (entry->metadata.isLoaded) {
// 			return entry->GetAs<Graphics::Resources::Mesh>();
// 		}
// 	}
// 	return nullptr;
// }

// Ref<Graphics::Resources::Mesh> AssetManager::TryGetMesh(const Utils::UUID &uuid) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	if (auto *entry = FindAssetEntry(uuid)) {
// 		if (entry->metadata.isLoaded) {
// 			return entry->GetAs<Graphics::Resources::Mesh>();
// 		}
// 	}
// 	return nullptr;
// }

// bool AssetManager::IsAssetLoading(const std::string &filepath) const {
// 	std::lock_guard<std::mutex> lock(m_LoadingMutex);

// 	if (m_PendingTextureLoads.find(filepath) != m_PendingTextureLoads.end()) {
// 		return true;
// 	}

// 	if (m_PendingMeshLoads.find(filepath) != m_PendingMeshLoads.end()) {
// 		return true;
// 	}

// 	return false;
// }

// Ref<Graphics::Resources::Texture2D> AssetManager::CreateRenderTarget(uint32 width, uint32 height, VkFormat format,
// 																	 VkImageUsageFlags usage,
// 																	 VkSampleCountFlagBits samples) {
// 	auto texture = Graphics::Resources::Texture2D::Builder(m_Device, "RenderTarget")
// 					   .AsRenderTarget(width, height, format, usage)
// 					   .WithSamples(samples)
// 					   .Build();

// 	if (texture) {
// 		std::string virtualPath = "runtime://render_target_" + std::to_string(width) + "x" + std::to_string(height);
// 		RegisterAsset(virtualPath, texture, AssetType::Texture);
// 	}

// 	return texture;
// }

// Ref<Graphics::Resources::Texture2D> AssetManager::CreateCubemap(uint32 width, uint32 height, VkFormat format,
// 																VkImageUsageFlags usage, bool mipmapped) {
// 	auto builder = Graphics::Resources::Texture2D::Builder(m_Device, "Cubemap");

// 	Ref<Graphics::Resources::Texture2D> texture;
// 	if (mipmapped) {
// 		texture = builder.AsMipmappedCubemap(width, height, format, usage).Build();
// 	} else {
// 		texture = builder.AsCubemap(width, height, format, usage).Build();
// 	}

// 	if (texture) {
// 		std::string virtualPath = "runtime://cubemap_" + std::to_string(width) + "x" + std::to_string(height);
// 		RegisterAsset(virtualPath, texture, AssetType::Texture);
// 	}

// 	return texture;
// }

// Ref<Graphics::Resources::Texture2D> AssetManager::GetFallbackTexture(Graphics::Resources::TextureType type) {
// 	if (auto it = m_FallbackTextures.find(type); it != m_FallbackTextures.end()) {
// 		return it->second;
// 	}

// 	// Should not happen after initialization
// 	AQUILA_LOG_ERROR("Fallback texture not initialized for type: {}", static_cast<int>(type));
// 	return nullptr;
// }

// void AssetManager::InitializeFallbackTextures() {
// 	std::vector<Graphics::Resources::TextureType> types = {
// 		Graphics::Resources::TextureType::Albedo,	Graphics::Resources::TextureType::Normal,
// 		Graphics::Resources::TextureType::Emissive, Graphics::Resources::TextureType::MetallicRoughness,
// 		Graphics::Resources::TextureType::AO,		Graphics::Resources::TextureType::Cubemap
// 	};

// 	for (auto type : types) {
// 		auto texture = Graphics::Resources::Texture2D::Builder(m_Device, "Fallback").AsFallback(type).Build();

// 		m_FallbackTextures[type] = texture;
// 	}

// 	AQUILA_LOG_INFO("Initialized {} fallback textures", m_FallbackTextures.size());
// }

// // Preloading & Batch Operations

// void AssetManager::PreloadAssets(const std::vector<std::string> &filepaths, Core::JobPriority priority) {
// 	AQUILA_LOG_INFO("Preloading {} assets", filepaths.size());

// 	// First pass: shaders, because materials depend on them
// 	for (const auto &filepath : filepaths) {
// 		if (GetAssetTypeFromExtension(filepath) == AssetType::Shader) {
// 			// Shader base path is .vert or .frag, strip extension to get the base
// 			std::string basePath = filepath.substr(0, filepath.size() - 5);
// 			// Only load once per base (avoid compiling twice for .vert + .frag)
// 			if (!TryGetShader(basePath)) {
// 				LoadShader(basePath);
// 			}
// 		}
// 	}

// 	// Second pass: everything else
// 	for (const auto &filepath : filepaths) {
// 		AssetType type = GetAssetTypeFromExtension(filepath);

// 		switch (type) {
// 		case AssetType::Mesh:
// 			LoadMeshAsync(filepath, priority);
// 			break;

// 		case AssetType::Texture: {
// 			std::string ext = std::filesystem::path(filepath).extension().string();
// 			std::ranges::transform(ext, ext.begin(), ::tolower);
// 			if (ext == ".hdr") {
// 				LoadHDRTextureAsync(filepath, priority);
// 			} else {
// 				LoadTextureAsync(filepath, VK_FORMAT_R8G8B8A8_SRGB, priority);
// 			}
// 			break;
// 		}

// 		case AssetType::Material:
// 			// Shader should already be loaded from first pass
// 			// LoadMaterialAsset(filepath);
// 			break;

// 		case AssetType::Shader:
// 			// Already handled in first pass
// 			break;

// 		default:
// 			AQUILA_LOG_DEBUG("Unsupported asset type for preload: {}", filepath);
// 			break;
// 		}
// 	}
// }

// void AssetManager::PreloadDirectory(const std::string &directory, bool recursive, Core::JobPriority priority) {
// 	auto files = FindAllAssetFiles(directory, recursive);
// 	PreloadAssets(files, priority);
// }

// void AssetManager::WaitForAllLoads() {
// 	Core::JobSystem::Get().WaitForAll();
// 	m_GPUUploadManager->FlushAll();
// }

// size_t AssetManager::GetPendingLoadCount() const {
// 	return Core::JobSystem::Get().GetActiveJobCount() + m_GPUUploadManager->GetQueueSize();
// }

// // Asset Management

// void AssetManager::RegisterAsset(const std::string &filepath, std::shared_ptr<void> asset, AssetType type) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);

// 	Utils::UUID uuid = GetOrCreateUUID(filepath);

// 	AssetEntry entry;
// 	entry.uuid = uuid;
// 	entry.metadata.uuid = uuid;
// 	entry.metadata.filepath = filepath;
// 	entry.metadata.type = type;
// 	entry.metadata.isLoaded = true;
// 	entry.metadata.isValid = true;
// 	entry.asset = std::move(asset);
// 	entry.type = type;
// 	entry.referenceCount = 1;

// 	m_AssetRegistry[uuid] = std::move(entry);
// }

// Utils::UUID AssetManager::GetOrCreateUUID(const std::string &filepath) {
// 	auto it = m_PathToUUID.find(filepath);
// 	if (it != m_PathToUUID.end()) {
// 		return it->second;
// 	}

// 	Utils::UUID uuid = Utils::UUID::FromFilepath(filepath);
// 	m_PathToUUID[filepath] = uuid;
// 	return uuid;
// }

// AssetEntry *AssetManager::FindAssetEntry(const Utils::UUID &uuid) {
// 	auto it = m_AssetRegistry.find(uuid);
// 	return it != m_AssetRegistry.end() ? &it->second : nullptr;
// }

// AssetEntry *AssetManager::FindAssetEntry(const std::string &filepath) {
// 	auto pathIt = m_PathToUUID.find(filepath);
// 	if (pathIt != m_PathToUUID.end()) {
// 		return FindAssetEntry(pathIt->second);
// 	}
// 	return nullptr;
// }

// const AssetEntry *AssetManager::FindAssetEntry(const Utils::UUID &uuid) const {
// 	auto it = m_AssetRegistry.find(uuid);
// 	return it != m_AssetRegistry.end() ? &it->second : nullptr;
// }

// const AssetEntry *AssetManager::FindAssetEntry(const std::string &filepath) const {
// 	auto pathIt = m_PathToUUID.find(filepath);
// 	if (pathIt != m_PathToUUID.end()) {
// 		return FindAssetEntry(pathIt->second);
// 	}
// 	return nullptr;
// }

// void AssetManager::UnloadAllAssets() {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	size_t count = m_AssetRegistry.size();

// 	m_AssetRegistry.clear();
// 	m_PathToUUID.clear();

// 	AQUILA_LOG_INFO("Unloaded all {} assets", count);
// }

// // Hot Reloading

// void AssetManager::ReloadAsset(const Utils::UUID &uuid) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);

// 	auto *entry = FindAssetEntry(uuid);
// 	if (entry == nullptr) {
// 		AQUILA_LOG_WARNING("Cannot reload asset: UUID not found");
// 		return;
// 	}

// 	std::string filepath = entry->metadata.filepath;
// 	AssetType type = entry->type;

// 	m_AssetRegistry.erase(uuid);

// 	if (type == AssetType::Mesh) {
// 		LoadMesh(filepath);
// 	} else if (type == AssetType::Texture) {
// 		std::string ext = std::filesystem::path(filepath).extension().string();
// 		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

// 		if (ext == ".hdr" || filepath.starts_with("HDR:")) {
// 			LoadHDRTexture(filepath);
// 		} else {
// 			LoadTexture(filepath);
// 		}
// 	} else if (type == AssetType::Scene) {
// 		LoadScene(filepath);
// 	}

// 	AQUILA_LOG_INFO("Reloaded asset: {}", filepath);
// }

// void AssetManager::ReloadAsset(const std::string &filepath) {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	auto pathIt = m_PathToUUID.find(filepath);
// 	if (pathIt != m_PathToUUID.end()) {
// 		ReloadAsset(pathIt->second);
// 	}
// }

// void AssetManager::CheckForModifiedAssets() {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	std::vector<Utils::UUID> modifiedAssets;

// 	for (auto &[uuid, entry] : m_AssetRegistry) {
// 		if (entry.metadata.filepath.starts_with("runtime://") || entry.metadata.filepath.starts_with("procedural://") ||
// 			entry.metadata.filepath.starts_with("HDR:")) {
// 			continue;
// 		}

// 		uint64_t currentModTime = GetFileModificationTime(entry.metadata.filepath);

// 		if (currentModTime > entry.metadata.lastModified) {
// 			modifiedAssets.push_back(uuid);
// 			entry.metadata.lastModified = currentModTime;
// 		}
// 	}
// }

// // Queries

// std::vector<Utils::UUID> AssetManager::GetAllAssetsOfType(AssetType type) const {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	std::vector<Utils::UUID> assets;

// 	for (const auto &[uuid, entry] : m_AssetRegistry) {
// 		if (entry.type == type) {
// 			assets.push_back(uuid);
// 		}
// 	}

// 	return assets;
// }

// std::vector<std::string> AssetManager::GetAllAssetPaths() const {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	std::vector<std::string> paths;
// 	paths.reserve(m_AssetRegistry.size());

// 	for (const auto &entry : m_AssetRegistry | std::views::values) {
// 		paths.push_back(entry.metadata.filepath);
// 	}

// 	return paths;
// }

// size_t AssetManager::GetAssetCount() const {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	return m_AssetRegistry.size();
// }

// size_t AssetManager::GetLoadedAssetCount() const {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	size_t count = 0;
// 	for (const auto &entry : m_AssetRegistry | std::views::values) {
// 		if (entry.metadata.isLoaded) {
// 			++count;
// 		}
// 	}
// 	return count;
// }

// // Cache Management

// void AssetManager::ClearCache() {
// 	UnloadAllAssets();
// }

// void AssetManager::ClearUnusedAssets() {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	std::vector<Utils::UUID> toRemove;

// 	for (const auto &[uuid, entry] : m_AssetRegistry) {
// 		if (entry.asset.use_count() <= 1) {
// 			toRemove.push_back(uuid);
// 		}
// 	}

// 	for (const auto &uuid : toRemove) {
// 		auto it = m_AssetRegistry.find(uuid);
// 		if (it != m_AssetRegistry.end()) {
// 			m_PathToUUID.erase(it->second.metadata.filepath);
// 			m_AssetRegistry.erase(it);
// 		}
// 	}

// 	AQUILA_LOG_INFO("Cleared {} unused assets", toRemove.size());
// }

// size_t AssetManager::GetCacheSize() const {
// 	std::lock_guard<std::mutex> lock(m_RegistryMutex);
// 	size_t totalSize = 0;
// 	for (const auto &entry : m_AssetRegistry | std::views::values) {
// 		totalSize += entry.metadata.fileSize;
// 	}
// 	return totalSize;
// }

// // Utility
// std::vector<std::string> AssetManager::FindAllAssetFiles(const std::string &directory, bool recursive) const {
// 	std::vector<std::string> files;

// 	auto *vfs = Platform::Filesystem::VirtualFileSystem::Get();
// 	if (vfs == nullptr) {
// 		AQUILA_LOG_ERROR("VFS not initialized");
// 		return files;
// 	}

// 	if (!vfs->IsDirectory(directory)) {
// 		AQUILA_LOG_WARNING("Path is not a directory or doesn't exist: {}", directory);
// 		return files;
// 	}

// 	std::function<void(const std::string &)> scanDirectory = [&](const std::string &currentDir) {
// 		auto entries = vfs->ListDirectory(currentDir);

// 		for (const auto &entry : entries) {
// 			std::string fullPath = currentDir;
// 			if (!fullPath.empty() && fullPath.back() != '/') {
// 				fullPath += "/";
// 			}
// 			fullPath += entry;

// 			if (vfs->IsDirectory(fullPath)) {
// 				if (recursive) {
// 					scanDirectory(fullPath);
// 				}
// 			} else {
// 				// It's a file
// 				AssetType type = GetAssetTypeFromExtension(fullPath);
// 				if (type != AssetType::Unknown) {
// 					files.push_back(fullPath);
// 				}
// 			}
// 		}
// 	};

// 	try {
// 		scanDirectory(directory);
// 	} catch (const std::exception &e) {
// 		AQUILA_LOG_ERROR("Failed to scan directory {}: {}", directory, e.what());
// 	}

// 	return files;
// }

// AssetType AssetManager::GetAssetTypeFromExtension(const std::string &filepath) const {
// 	std::string ext = std::filesystem::path(filepath).extension().string();
// 	std::ranges::transform(ext, ext.begin(), ::tolower);

// 	if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae") {
// 		return AssetType::Mesh;
// 	}
// 	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" || ext == ".hdr") {
// 		return AssetType::Texture;
// 	}
// 	if (ext == ".aqscene") {
// 		return AssetType::Scene;
// 	}
// 	if (ext == ".vert" || ext == ".frag" || ext == ".comp" || ext == ".geom" || ext == ".spv") {
// 		return AssetType::Shader;
// 	}
// 	if (ext == ".aqmat") {
// 		return AssetType::Material;
// 	}
// 	if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
// 		return AssetType::Audio;
// 	}

// 	return AssetType::Unknown;
// }

// uint64_t AssetManager::GetFileModificationTime(const std::string &filepath) const {
// 	auto vfs = Platform::Filesystem::VirtualFileSystem::Get();
// 	if (!vfs) {
// 		return 0;
// 	}

// 	return vfs->GetLastWriteTime(filepath);
// }

// uint64_t AssetManager::GetFileSize(const std::string &filepath) const {
// 	auto vfs = Platform::Filesystem::VirtualFileSystem::Get();
// 	if (!vfs) {
// 		return 0;
// 	}

// 	int64_t size = vfs->GetFileSize(filepath);
// 	return size > 0 ? static_cast<uint64_t>(size) : 0;
// }
// } // namespace Aquila::Assets

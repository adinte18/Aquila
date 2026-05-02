// #pragma once

// #include "Aquila/Foundation/Job.h"
// #include "Aquila/Graphics/Core/GPUUploadManager.h"

// namespace Aquila::Assets {

// // Asset loading state
// enum class AssetLoadState {
// 	NotLoaded,	  // Initial state
// 	LoadingCPU,	  // CPU-side loading in progress
// 	CPUReady,	  // CPU data ready, waiting for GPU upload
// 	UploadingGPU, // GPU upload in progress
// 	Ready,		  // Fully loaded and ready to use
// 	Failed		  // Loading failed
// };

// // Base class for all loadable assets
// template <typename T> class LoadableAsset {
//   public:
// 	virtual ~LoadableAsset() = default;

// 	AssetLoadState GetState() const { return m_State.load(std::memory_order_acquire); }

// 	bool IsReady() const { return m_State.load(std::memory_order_acquire) == AssetLoadState::Ready; }

// 	bool IsFailed() const { return m_State.load(std::memory_order_acquire) == AssetLoadState::Failed; }

// 	// Get the actual asset (blocks until ready if loading)
// 	Ref<T> Get() {
// 		if (m_LoadHandle.IsComplete()) {
// 			try {
// 				return m_LoadHandle.Get();
// 			} catch (const std::exception &e) {
// 				AQUILA_LOG_ERROR("Asset loading failed: {}", e.what());
// 				m_State.store(AssetLoadState::Failed, std::memory_order_release);
// 				return nullptr;
// 			}
// 		}
// 		return nullptr;
// 	}

// 	// Non-blocking get
// 	Ref<T> TryGet() {
// 		if (IsReady()) {
// 			return m_Asset;
// 		}
// 		return nullptr;
// 	}

// 	void WaitUntilReady() { m_LoadHandle.Wait(); }

//   protected:
// 	void SetState(AssetLoadState state) { m_State.store(state, std::memory_order_release); }

// 	void SetAsset(Ref<T> asset) { m_Asset = asset; }

// 	void SetHandle(Foundation::JobHandle<Ref<T>> handle) { m_LoadHandle = std::move(handle); }

//   private:
// 	std::atomic<AssetLoadState> m_State{ AssetLoadState::NotLoaded };
// 	Ref<T> m_Asset;
// 	Foundation::JobHandle<Ref<T>> m_LoadHandle;
// };

// // Generic async asset loader
// template <typename AssetType> class AsyncAssetLoader {
//   public:
// 	using CPULoadFunc = std::function<void(AssetType &)>;
// 	using GPUUploadFunc = std::function<void(AssetType &, VkCommandBuffer)>;

// 	AsyncAssetLoader(Graphics::Device &device, Graphics::GPUUploadManager &uploadManager)
// 		: m_Device(device), m_UploadManager(uploadManager) {}

// 	// Load asset asynchronously
// 	Core::JobHandle<Ref<AssetType>> LoadAsync(const std::string &filepath, CPULoadFunc cpuLoad, GPUUploadFunc gpuUpload,
// 											  Core::JobPriority priority = Core::JobPriority::Normal) {
// 		return Core::JobSystem::Get().Schedule(
// 			priority, "LoadAsset_" + filepath, [this, filepath, cpuLoad, gpuUpload]() -> Ref<AssetType> {
// 				try {
// 					// Step 1: CPU-side loading (runs on worker thread)
// 					auto asset = CreateRef<AssetType>();
// 					cpuLoad(*asset);

// 					// Step 2: Queue GPU upload (runs on GPU thread)
// 					auto uploadFuture = m_UploadManager.QueueUpload(
// 						[asset, gpuUpload](VkCommandBuffer cmd) { gpuUpload(*asset, cmd); }, "Upload_" + filepath,
// 						0 // priority
// 					);

// 					// Wait for GPU upload to complete
// 					uploadFuture.wait();

// 					return asset;

// 				} catch (const std::exception &e) {
// 					AQUILA_LOG_ERROR("Failed to load asset '{}': {}", filepath, e.what());
// 					throw;
// 				}
// 			});
// 	}

// 	// Load multiple assets in parallel
// 	std::vector<Core::JobHandle<Ref<AssetType>>> LoadBatch(const std::vector<std::string> &filepaths,
// 														   CPULoadFunc cpuLoad, GPUUploadFunc gpuUpload,
// 														   Core::JobPriority priority = Core::JobPriority::Normal) {
// 		std::vector<Core::JobHandle<Ref<AssetType>>> handles;
// 		handles.reserve(filepaths.size());

// 		for (const auto &filepath : filepaths) {
// 			handles.push_back(LoadAsync(filepath, cpuLoad, gpuUpload, priority));
// 		}

// 		return handles;
// 	}

// 	// Wait for all pending loads to complete
// 	void FlushAll() {
// 		Core::JobSystem::Get().WaitForAll();
// 		m_UploadManager.FlushAll();
// 	}

//   private:
// 	Graphics::Device &m_Device;
// 	Graphics::GPUUploadManager &m_UploadManager;
// };

// } // namespace Aquila::Assets

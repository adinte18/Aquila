#ifndef AQUILA_MESH_CACHE_H
#define AQUILA_MESH_CACHE_H

#include "Aquila/Graphics/Resources/Mesh.h"

namespace Aquila::Graphics::Resources {

class MeshCache {
  public:
	static MeshCache *Get() {
		static MeshCache instance;
		return &instance;
	}

	Ref<Mesh> Load(const std::string &filepath, Device &device, Utils::DeletionManager *queue, uint32 currentFrame) {
		std::lock_guard<std::mutex> lock(m_Mutex);

		auto it = m_Cache.find(filepath);
		if (it != m_Cache.end()) {
			AQUILA_LOG_DEBUG("Mesh cache hit: {}", filepath);
			return it->second;
		}

		AQUILA_LOG_INFO("Mesh cache miss, loading: {}", filepath);
		auto mesh = CreateRef<Mesh>(device, queue, filepath);

		try {
			mesh->Load(filepath);
			m_Cache[filepath] = mesh;
			return mesh;
		} catch (const std::exception &e) {
			AQUILA_LOG_ERROR("Failed to load mesh {}: {}", filepath, e.what());
			return nullptr;
		}
	}

	std::future<Ref<Mesh>> LoadAsync(const std::string &filepath, Device &device, Utils::DeletionManager *queue) {
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			auto it = m_Cache.find(filepath);
			if (it != m_Cache.end()) {
				AQUILA_LOG_DEBUG("Mesh cache hit (async): {}", filepath);

				std::promise<Ref<Mesh>> promise;
				promise.set_value(it->second);
				return promise.get_future();
			}
		}

		return std::async(std::launch::async, [this, filepath, &device, queue]() {
			auto mesh = CreateRef<Mesh>(device, queue, filepath);

			try {
				mesh->Load(filepath);

				std::lock_guard<std::mutex> lock(m_Mutex);
				m_Cache[filepath] = mesh;
				AQUILA_LOG_INFO("Async loaded mesh: {}", filepath);
				return mesh;
			} catch (const std::exception &e) {
				AQUILA_LOG_ERROR("Failed to async load mesh {}: {}", filepath, e.what());
				return Ref<Mesh>(nullptr);
			}
		});
	}

	Ref<Mesh> Get(const std::string &filepath) {
		std::lock_guard<std::mutex> lock(m_Mutex);
		auto it = m_Cache.find(filepath);
		return (it != m_Cache.end()) ? it->second : nullptr;
	}

	bool IsCached(const std::string &filepath) const {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Cache.contains(filepath);
	}

	void Remove(const std::string &filepath) {
		std::lock_guard<std::mutex> lock(m_Mutex);
		auto it = m_Cache.find(filepath);
		if (it != m_Cache.end()) {
			AQUILA_LOG_INFO("Removing mesh from cache: {}", filepath);
			m_Cache.erase(it);
		}
	}

	void Clear() {
		std::lock_guard<std::mutex> lock(m_Mutex);
		size_t count = m_Cache.size();
		m_Cache.clear();
		AQUILA_LOG_INFO("Cleared mesh cache ({} entries)", count);
	}

	size_t GetCacheSize() const {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Cache.size();
	}

	void PreloadMeshes(const std::vector<std::string> &filepaths, Device &device, Utils::DeletionManager *queue) {
		for (const auto &filepath : filepaths) {
			LoadAsync(filepath, device, queue);
		}
	}

  private:
	MeshCache() = default;
	~MeshCache() = default;

	MeshCache(const MeshCache &) = delete;
	MeshCache &operator=(const MeshCache &) = delete;

	std::unordered_map<std::string, Ref<Mesh>> m_Cache;
	mutable std::mutex m_Mutex;
};

} // namespace Aquila::Graphics::Resources

#endif

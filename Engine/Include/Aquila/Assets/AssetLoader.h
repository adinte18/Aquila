

#ifndef AQUILA_ASSET_LOADER_H
#define AQUILA_ASSET_LOADER_H

#include "Aquila/Core/AquilaCore.h"

namespace Aquila::Assets {

template <typename T> struct AssetLoadRequest {
	std::string filepath;
	std::promise<Ref<T>> promise;
	std::function<Ref<T>()> loadFunc;
	int priority = 0;
};

template <typename T> class AssetLoader {
  public:
	AssetLoader(size_t numThreads = 4) : m_Running(true) {
		for (size_t i = 0; i < numThreads; ++i) {
			m_Workers.emplace_back([this]() { WorkerThread(); });
		}

		AQUILA_LOG_INFO("AssetLoader<{}> started with {} threads", typeid(T).name(), numThreads);
	}

	~AssetLoader() { Shutdown(); }

	std::future<Ref<T>> LoadAsync(const std::string &filepath, std::function<Ref<T>()> loadFunc, int priority = 0) {
		std::unique_lock<std::mutex> lock(m_QueueMutex);

		AssetLoadRequest<T> request;
		request.filepath = filepath;
		request.loadFunc = std::move(loadFunc);
		request.priority = priority;

		auto future = request.promise.get_future();

		m_LoadQueue.push(std::move(request));
		lock.unlock();

		m_QueueCV.notify_one();

		return future;
	}

	bool IsLoading(const std::string &filepath) const {
		std::lock_guard<std::mutex> lock(m_QueueMutex);

		return false;
	}

	size_t GetQueueSize() const {
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		return m_LoadQueue.size();
	}

	void Shutdown() {
		{
			std::lock_guard<std::mutex> lock(m_QueueMutex);
			m_Running = false;
		}
		m_QueueCV.notify_all();

		for (auto &worker : m_Workers) {
			if (worker.joinable()) {
				worker.join();
			}
		}

		m_Workers.clear();
	}

  private:
	void WorkerThread() {
		while (m_Running) {
			AssetLoadRequest<T> request;

			{
				std::unique_lock<std::mutex> lock(m_QueueMutex);
				m_QueueCV.wait(lock, [this]() { return !m_LoadQueue.empty() || !m_Running; });

				if (!m_Running && m_LoadQueue.empty()) {
					return;
				}

				if (!m_LoadQueue.empty()) {
					request = std::move(const_cast<AssetLoadRequest<T> &>(m_LoadQueue.top()));
					m_LoadQueue.pop();
				}
			}

			if (request.loadFunc) {
				try {
					auto asset = request.loadFunc();
					request.promise.set_value(asset);
				} catch (const std::exception &e) {
					AQUILA_LOG_ERROR("Asset load failed: {}", e.what());
					request.promise.set_exception(std::current_exception());
				}
			}
		}
	}

	struct RequestComparator {
		bool operator()(const AssetLoadRequest<T> &a, const AssetLoadRequest<T> &b) const {
			return a.priority < b.priority;
		}
	};

	std::vector<std::thread> m_Workers;
	std::priority_queue<AssetLoadRequest<T>, std::vector<AssetLoadRequest<T>>, RequestComparator> m_LoadQueue;
	mutable std::mutex m_QueueMutex;
	std::condition_variable m_QueueCV;
	bool m_Running;
};

template <typename T> class AssetCache {
  public:
	AssetCache(size_t loaderThreads = 4) : m_Loader(loaderThreads) {}

	~AssetCache() = default;

	Ref<T> Load(const std::string &filepath, std::function<Ref<T>()> loadFunc, int priority = 0) {
		{
			std::lock_guard<std::mutex> lock(m_CacheMutex);
			auto it = m_Cache.find(filepath);
			if (it != m_Cache.end()) {
				AQUILA_LOG_DEBUG("Cache hit: {}", filepath);
				return it->second;
			}
		}

		AQUILA_LOG_INFO("Cache miss, loading: {}", filepath);
		auto asset = loadFunc();

		if (asset) {
			std::lock_guard<std::mutex> lock(m_CacheMutex);
			m_Cache[filepath] = asset;
		}

		return asset;
	}

	std::future<Ref<T>> LoadAsync(const std::string &filepath, std::function<Ref<T>()> loadFunc, int priority = 0) {
		{
			std::lock_guard<std::mutex> lock(m_CacheMutex);
			auto it = m_Cache.find(filepath);
			if (it != m_Cache.end()) {
				std::promise<Ref<T>> promise;
				promise.set_value(it->second);
				return promise.get_future();
			}
		}

		return m_Loader.LoadAsync(
			filepath,
			[this, filepath, loadFunc]() {
				auto asset = loadFunc();
				if (asset) {
					std::lock_guard<std::mutex> lock(m_CacheMutex);
					m_Cache[filepath] = asset;
				}
				return asset;
			},
			priority);
	}

	Ref<T> Get(const std::string &filepath) {
		std::lock_guard<std::mutex> lock(m_CacheMutex);
		auto it = m_Cache.find(filepath);
		return (it != m_Cache.end()) ? it->second : nullptr;
	}

	bool IsCached(const std::string &filepath) const {
		std::lock_guard<std::mutex> lock(m_CacheMutex);
		return m_Cache.contains(filepath);
	}

	void Remove(const std::string &filepath) {
		std::lock_guard<std::mutex> lock(m_CacheMutex);
		m_Cache.erase(filepath);
	}

	void Clear() {
		std::lock_guard<std::mutex> lock(m_CacheMutex);
		m_Cache.clear();
	}

	size_t Size() const {
		std::lock_guard<std::mutex> lock(m_CacheMutex);
		return m_Cache.size();
	}

  private:
	AssetLoader<T> m_Loader;
	std::unordered_map<std::string, Ref<T>> m_Cache;
	mutable std::mutex m_CacheMutex;
};

} // namespace Aquila::Assets

#endif

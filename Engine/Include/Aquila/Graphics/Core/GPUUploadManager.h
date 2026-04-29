#ifndef AQUILA_GPU_UPLOADER_H
#define AQUILA_GPU_UPLOADER_H

#include "Aquila/GFX/GfxContext.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Graphics {

struct GPUUploadCommand {
	std::function<void(GFX::GfxCommandList &)> uploadFunc;
	std::promise<void> completion;
	std::string debugName;
	uint32 priority = 0;

	bool operator<(const GPUUploadCommand &other) const { return priority < other.priority; }
};

class GPUUploadManager {
  public:
	explicit GPUUploadManager(GFX::GfxContext &ctx) : m_Context(ctx) {}
	~GPUUploadManager() { Shutdown(); }

	void Initialize() {
		if (m_Initialized) {
			AQUILA_LOG_WARNING("GPUUploadManager already initialized");
			return;
		}
		m_Running = true;
		m_UploadThread = std::thread(&GPUUploadManager::UploadThreadFunc, this);
		m_Initialized = true;
		AQUILA_LOG_INFO("GPUUploadManager initialized");
	}

	void Shutdown() {
		if (!m_Initialized) {
			return;
		}
		AQUILA_LOG_INFO("Shutting down GPUUploadManager...");
		m_Running = false;
		m_QueueCV.notify_all();
		if (m_UploadThread.joinable()) {
			m_UploadThread.join();
		}
		m_Initialized = false;
		AQUILA_LOG_INFO("GPUUploadManager shut down");
	}

	std::future<void> QueueUpload(std::function<void(GFX::GfxCommandList &)> uploadFunc,
								  const std::string &debugName = "GPUUpload", uint32 priority = 0) {
		GPUUploadCommand cmd;
		cmd.uploadFunc = std::move(uploadFunc);
		cmd.debugName = debugName;
		cmd.priority = priority;
		auto future = cmd.completion.get_future();
		{
			std::lock_guard<std::mutex> lock(m_QueueMutex);
			m_UploadQueue.push(std::move(cmd));
		}
		m_QueueCV.notify_one();
		return future;
	}

	void FlushAll() {
		std::unique_lock<std::mutex> lock(m_QueueMutex);
		m_FlushCV.wait(lock, [this] { return m_UploadQueue.empty() && !m_IsProcessing; });
	}

	size_t GetQueueSize() const {
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		return m_UploadQueue.size();
	}

  private:
	void ExecuteUpload(GPUUploadCommand &cmd) {
		m_Context.ExecuteImmediate(RHI::CommandListType::Transfer,
								   [&](GFX::GfxCommandList &cmdList) { cmd.uploadFunc(cmdList); });
	}

	void UploadThreadFunc() {
		while (m_Running) {
			GPUUploadCommand cmd;
			{
				std::unique_lock<std::mutex> lock(m_QueueMutex);
				m_QueueCV.wait(lock, [this] { return !m_UploadQueue.empty() || !m_Running; });

				if (!m_Running && m_UploadQueue.empty())
					break;
				if (m_UploadQueue.empty())
					continue;

				cmd = std::move(const_cast<GPUUploadCommand &>(m_UploadQueue.top()));
				m_UploadQueue.pop();
				m_IsProcessing = true;
			}

			try {
				ExecuteUpload(cmd);
				cmd.completion.set_value();
			} catch (const std::exception &e) {
				AQUILA_LOG_ERROR("GPU upload '{}' failed: {}", cmd.debugName, e.what());
				cmd.completion.set_exception(std::current_exception());
			}

			{
				std::lock_guard<std::mutex> lock(m_QueueMutex);
				m_IsProcessing = false;
			}
			m_FlushCV.notify_all();
		}

		// Drain remaining on shutdown
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		while (!m_UploadQueue.empty()) {
			auto remaining = std::move(const_cast<GPUUploadCommand &>(m_UploadQueue.top()));
			m_UploadQueue.pop();
			try {
				remaining.completion.set_exception(
					std::make_exception_ptr(std::runtime_error("GPUUploadManager shutting down")));
			} catch (...) {
			}
		}
		AQUILA_LOG_INFO("Upload thread exited cleanly");
	}

	GFX::GfxContext &m_Context;

	bool m_Initialized = false;
	bool m_Running = false;
	bool m_IsProcessing = false;

	std::thread m_UploadThread;

	mutable std::mutex m_QueueMutex;
	std::condition_variable m_QueueCV;
	std::condition_variable m_FlushCV;
	std::priority_queue<GPUUploadCommand> m_UploadQueue;
};

} // namespace Aquila::Graphics
#endif

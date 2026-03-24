#pragma once

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Foundation {

struct Job {
	std::function<void()> task;
	Priority priority = Priority::Medium;
	std::string debugName;

	bool operator<(const Job &other) const { return priority < other.priority; }
};

template <typename T> class JobHandle {
  public:
	JobHandle() = default;
	explicit JobHandle(std::shared_future<T> future) : m_Future(std::move(future)) {}

	[[nodiscard]] bool IsComplete() const {
		return m_Future.valid() && m_Future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}

	void Wait() const {
		if (m_Future.valid()) {
			m_Future.wait();
		}
	}

	T Get() { return m_Future.get(); }

	const std::shared_future<T> &GetFuture() const { return m_Future; }

  private:
	std::shared_future<T> m_Future;
};

class JobQueue {
  public:
	void Push(Job &&job) {
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Jobs.push(std::move(job));
		m_Condition.notify_one();
	}

	bool TryPop(Job &job) {
		std::unique_lock<std::mutex> lock(m_Mutex);
		if (m_Jobs.empty()) {
			return false;
		}
		job = std::move(const_cast<Job &>(m_Jobs.top()));
		m_Jobs.pop();
		return true;
	}

	bool WaitAndPop(Job &job, std::atomic<bool> &shouldRun) {
		std::unique_lock<std::mutex> lock(m_Mutex);

		m_Condition.wait(
			lock, [this, &shouldRun]() { return !m_Jobs.empty() || !shouldRun.load(std::memory_order_acquire); });

		if (!shouldRun.load(std::memory_order_acquire) && m_Jobs.empty()) {
			return false;
		}

		if (!m_Jobs.empty()) {
			job = std::move(const_cast<Job &>(m_Jobs.top()));
			m_Jobs.pop();
			return true;
		}

		return false;
	}

	void NotifyAll() { m_Condition.notify_all(); }

	size_t Size() const {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Jobs.size();
	}

	bool Empty() const {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Jobs.empty();
	}

  private:
	mutable std::mutex m_Mutex;
	std::condition_variable m_Condition;
	std::priority_queue<Job> m_Jobs;
};

class JobSystem {
  public:
	static JobSystem &Get() {
		static JobSystem instance;
		return instance;
	}

	~JobSystem() { Shutdown(); }

	void Initialize(uint32 threadCount = 0) {
		if (m_Initialized.load(std::memory_order_acquire)) {
			AQUILA_LOG_WARNING("JobSystem already initialized");
			return;
		}

		if (threadCount == 0) {
			threadCount = std::max(1U, std::thread::hardware_concurrency() - 1);
		}

		m_ThreadCount = threadCount;
		m_Running.store(true, std::memory_order_release);

		for (uint32 i = 0; i < threadCount; ++i) {
			m_Workers.emplace_back([this, i]() {
#ifdef _WIN32
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#else
				nice(5);
#endif
				WorkerThread(i);
			});
		}

		m_Initialized.store(true, std::memory_order_release);
		AQUILA_LOG_INFO("JobSystem initialized with {} worker threads", threadCount);
	}

	void Shutdown() {
		if (!m_Initialized.load(std::memory_order_acquire)) {
			return;
		}

		m_Running.store(false, std::memory_order_seq_cst);
		m_JobQueue.NotifyAll();

		for (auto &worker : m_Workers) {
			if (worker.joinable()) {
				worker.join();
			}
		}

		m_Workers.clear();
		m_Initialized.store(false, std::memory_order_release);
		AQUILA_LOG_INFO("JobSystem shut down");
	}

	template <typename Func, typename... Args>
	auto Schedule(Priority priority, const std::string &debugName, Func &&func, Args &&...args)
		-> JobHandle<std::invoke_result_t<Func, Args...>> {
		using ReturnType = std::invoke_result_t<Func, Args...>;

		auto task = std::make_shared<std::packaged_task<ReturnType()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

		auto future = task->get_future().share();

		Job job;
		job.priority = priority;
		job.debugName = debugName;
		job.task = [task]() { (*task)(); };

		m_JobQueue.Push(std::move(job));
		m_ActiveJobCount.fetch_add(1, std::memory_order_relaxed);

		return JobHandle<ReturnType>(future);
	}

	template <typename Func, typename... Args>
	auto ScheduleNormal(const std::string &debugName, Func &&func, Args &&...args) {
		return Schedule(Priority::Medium, debugName, std::forward<Func>(func), std::forward<Args>(args)...);
	}

	template <typename Func, typename... Args>
	auto ScheduleHigh(const std::string &debugName, Func &&func, Args &&...args) {
		return Schedule(Priority::High, debugName, std::forward<Func>(func), std::forward<Args>(args)...);
	}

	void WaitForAll() {
		while (m_ActiveJobCount.load(std::memory_order_relaxed) > 0 || !m_JobQueue.Empty()) {
			std::this_thread::yield();
		}
	}

	size_t GetPendingJobCount() const { return m_JobQueue.Size(); }
	size_t GetActiveJobCount() const { return m_ActiveJobCount.load(std::memory_order_relaxed); }
	uint32 GetThreadCount() const { return m_ThreadCount; }

  private:
	JobSystem() = default;
	JobSystem(const JobSystem &) = delete;
	JobSystem &operator=(const JobSystem &) = delete;

	void WorkerThread(uint32 threadId) {
		while (true) {
			Job job;

			if (!m_JobQueue.WaitAndPop(job, m_Running)) {
				break;
			}

			try {
				job.task();
			} catch (const std::exception &e) {
				AQUILA_LOG_ERROR("Job '{}' failed on thread {}: {}", job.debugName, threadId, e.what());
			}

			m_ActiveJobCount.fetch_sub(1, std::memory_order_relaxed);
		}
	}

	std::atomic<bool> m_Initialized{ false };
	std::atomic<bool> m_Running{ false };
	uint32 m_ThreadCount = 0;

	JobQueue m_JobQueue;
	std::vector<std::thread> m_Workers;
	std::atomic<size_t> m_ActiveJobCount{ 0 };
};

} // namespace Aquila::Foundation

#pragma once

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include <functional>
#include <string>
#include <future>
#include <chrono>
#include <mutex>
#include <utility>
#include <atomic>
#include <cstddef>
#include <queue>
#include <algorithm>
#include <thread>
#include <processthreadsapi.h>
#include <winbase.h>
#include <type_traits>
#include <exception>
#include <vector>

namespace Aquila::Foundation {

struct Job {
	std::function<void()> task;
	Priority priority = Priority::Medium;
	std::string debug_name;

	bool operator<(const Job &other) const { return priority < other.priority; }
};

template <typename T> class JobHandle {
  public:
	JobHandle() = default;
	explicit JobHandle(std::shared_future<T> future) : m_future(std::move(future)) {}

	[[nodiscard]] bool is_complete() const {
		return m_future.valid() && m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}

	void wait() const {
		if (m_future.valid()) {
			m_future.wait();
		}
	}

	T get() { return m_future.get(); }

	const std::shared_future<T> &get_future() const { return m_future; }

  private:
	std::shared_future<T> m_future;
};

class JobQueue {
  public:
	void push(Job &&job) {
		std::scoped_lock const lock(m_mutex);
		m_jobs.push(std::move(job));
		m_condition.notify_one();
	}

	bool try_pop(Job &job) {
		std::unique_lock<std::mutex> const lock(m_mutex);
		if (m_jobs.empty()) {
			return false;
		}
		job = std::move(const_cast<Job &>(m_jobs.top()));
		m_jobs.pop();
		return true;
	}

	bool wait_and_pop(Job &job, std::atomic<bool> &should_run) {
		std::unique_lock<std::mutex> lock(m_mutex);

		m_condition.wait(
			lock, [this, &should_run]() { return !m_jobs.empty() || !should_run.load(std::memory_order_acquire); });

		if (!should_run.load(std::memory_order_acquire) && m_jobs.empty()) {
			return false;
		}

		if (!m_jobs.empty()) {
			job = std::move(const_cast<Job &>(m_jobs.top()));
			m_jobs.pop();
			return true;
		}

		return false;
	}

	void notify_all() { m_condition.notify_all(); }

	size_t size() const {
		std::scoped_lock const lock(m_mutex);
		return m_jobs.size();
	}

	bool empty() const {
		std::scoped_lock const lock(m_mutex);
		return m_jobs.empty();
	}

  private:
	mutable std::mutex m_mutex;
	std::condition_variable m_condition;
	std::priority_queue<Job> m_jobs;
};

class JobSystem {
  public:
	static JobSystem &get() {
		static JobSystem instance;
		return instance;
	}

	~JobSystem() { shutdown(); }

	void initialize(Uint32 thread_count = 0) {
		if (m_initialized.load(std::memory_order_acquire)) {
			AQUILA_LOG_WARNING("JobSystem already initialized");
			return;
		}

		if (thread_count == 0) {
			thread_count = std::max(1U, std::thread::hardware_concurrency() - 1);
		}

		m_thread_count = thread_count;
		m_running.store(true, std::memory_order_release);

		for (Uint32 i = 0; i < thread_count; ++i) {
			m_workers.emplace_back([this, i]() {
#ifdef _WIN32
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#else
				nice(5);
#endif
				worker_thread(i);
			});
		}

		m_initialized.store(true, std::memory_order_release);
		AQUILA_LOG_INFO("JobSystem initialized with {} worker threads", thread_count);
	}

	void shutdown() {
		if (!m_initialized.load(std::memory_order_acquire)) {
			return;
		}

		m_running.store(false, std::memory_order_seq_cst);
		m_job_queue.notify_all();

		for (auto &worker : m_workers) {
			if (worker.joinable()) {
				worker.join();
			}
		}

		m_workers.clear();
		m_initialized.store(false, std::memory_order_release);
		AQUILA_LOG_INFO("JobSystem shut down");
	}

	template <typename Func, typename... Args>
	auto schedule(Priority priority, const std::string &debug_name, Func &&func, Args &&...args)
		-> JobHandle<std::invoke_result_t<Func, Args...>> {
		using ReturnType = std::invoke_result_t<Func, Args...>;

		auto task = std::make_shared<std::packaged_task<ReturnType()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

		auto future = task->get_future().share();

		Job job;
		job.priority = priority;
		job.debug_name = debug_name;
		job.task = [task]() { (*task)(); };

		m_job_queue.push(std::move(job));
		m_active_job_count.fetch_add(1, std::memory_order_relaxed);

		return JobHandle<ReturnType>(future);
	}

	template <typename Func, typename... Args>
	auto schedule_normal(const std::string &debug_name, Func &&func, Args &&...args) {
		return Schedule(Priority::Medium, debug_name, std::forward<Func>(func), std::forward<Args>(args)...);
	}

	template <typename Func, typename... Args>
	auto schedule_high(const std::string &debug_name, Func &&func, Args &&...args) {
		return Schedule(Priority::High, debug_name, std::forward<Func>(func), std::forward<Args>(args)...);
	}

	void wait_for_all() {
		while (m_active_job_count.load(std::memory_order_relaxed) > 0 || !m_job_queue.empty()) {
			std::this_thread::yield();
		}
	}

	size_t get_pending_job_count() const { return m_job_queue.size(); }
	size_t get_active_job_count() const { return m_active_job_count.load(std::memory_order_relaxed); }
	Uint32 get_thread_count() const { return m_thread_count; }

  private:
	JobSystem() = default;
	JobSystem(const JobSystem &) = delete;
	JobSystem &operator=(const JobSystem &) = delete;

	void worker_thread(Uint32 thread_id) {
		while (true) {
			Job job;

			if (!m_job_queue.wait_and_pop(job, m_running)) {
				break;
			}

			try {
				job.task();
			} catch (const std::exception &e) {
				AQUILA_LOG_ERROR("Job '{}' failed on thread {}: {}", job.debug_name, thread_id, e.what());
			}

			m_active_job_count.fetch_sub(1, std::memory_order_relaxed);
		}
	}

	std::atomic<bool> m_initialized{ false };
	std::atomic<bool> m_running{ false };
	Uint32 m_thread_count = 0;

	JobQueue m_job_queue;
	std::vector<std::thread> m_workers;
	std::atomic<size_t> m_active_job_count{ 0 };
};

} // namespace Aquila::Foundation

#ifndef AQUILA_PROFILER_H
#define AQUILA_PROFILER_H

#include "Aquila/Utilities/Timer.h"
#include <algorithm>
#include <array>
#include <mutex>

namespace Aquila::Utils {

struct ProfilerEntry {
	std::string name;
	f64 startTime = 0.0;
	f64 duration = 0.0;
	uint32 depth = 0;
	uint32 callCount = 0;

	std::thread::id threadId;

	uint32 color = 0;

	f64 avgDuration = 0.0;
	f64 minDuration = DBL_MAX;
	f64 maxDuration = 0.0;
	f64 totalDuration = 0.0;
	uint32 frameCount = 0;

	std::array<f32, 60> recentDurations = {};
	uint32 historyIndex = 0;

	ProfilerEntry() { recentDurations.fill(0.0F); }
};

struct FrameStats {
	f64 frameDuration = 0.0;
	f64 cpuTime = 0.0;
	f64 gpuTime = 0.0;
	uint32 frameNumber = 0;
	TimePoint timestamp;
};

class Profiler {
  public:
	static Profiler &Get() {
		static Profiler instance;
		return instance;
	}

	void BeginFrame() {
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_FrameStart = Now();
		m_CurrentDepth = 0;
		m_CurrentFrameEntries.clear();
		m_FrameNumber++;
	}

	void EndFrame() {
		std::lock_guard<std::mutex> lock(m_Mutex);

		m_FrameDuration = ElapsedMilliseconds(m_FrameStart, Now());
		m_FrameCount++;

		m_FPS = 1000.0 / m_FrameDuration;

		m_FrameTimeHistory[m_FrameTimeHistoryIndex] = static_cast<f32>(m_FrameDuration);
		m_FrameTimeHistoryIndex = (m_FrameTimeHistoryIndex + 1) % m_FrameTimeHistory.size();

		f64 cpuTime = 0.0;
		for (const auto &entry : m_CurrentFrameEntries) {
			if (entry.depth == 0) {
				cpuTime += entry.duration;
			}
		}

		FrameStats frameStats;
		frameStats.frameDuration = m_FrameDuration;
		frameStats.cpuTime = cpuTime;
		frameStats.frameNumber = m_FrameNumber;
		frameStats.timestamp = Now();

		m_FrameStatsHistory.push_back(frameStats);
		if (m_FrameStatsHistory.size() > m_MaxHistoryFrames) {
			m_FrameStatsHistory.erase(m_FrameStatsHistory.begin());
		}

		for (auto &entry : m_CurrentFrameEntries) {
			auto &stats = m_Stats[entry.name];
			stats.name = entry.name;
			stats.totalDuration += entry.duration;
			stats.minDuration = std::min(stats.minDuration, entry.duration);
			stats.maxDuration = std::max(stats.maxDuration, entry.duration);
			stats.frameCount++;
			stats.avgDuration = stats.totalDuration / stats.frameCount;
			stats.callCount += entry.callCount;
			stats.depth = entry.depth;
			stats.color = entry.color;
			stats.threadId = entry.threadId;

			stats.recentDurations[stats.historyIndex] = static_cast<f32>(entry.duration);
			stats.historyIndex = (stats.historyIndex + 1) % stats.recentDurations.size();
		}

		m_FrameHistory.push_back(m_CurrentFrameEntries);
		if (m_FrameHistory.size() > m_MaxHistoryFrames) {
			m_FrameHistory.erase(m_FrameHistory.begin());
		}

		DetectBottlenecks();
	}

	void BeginSection(const std::string &name) {
		std::lock_guard<std::mutex> lock(m_Mutex);

		ProfilerEntry entry;
		entry.name = name;
		entry.startTime = ElapsedMilliseconds(m_FrameStart, Now());
		entry.depth = m_CurrentDepth;
		entry.callCount = 1;
		entry.threadId = std::this_thread::get_id();
		entry.color = HashString(name);

		m_SectionStack.push_back(entry);
		m_CurrentDepth++;
	}

	void EndSection() {
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_SectionStack.empty()) {
			return;
		}

		auto entry = m_SectionStack.back();
		m_SectionStack.pop_back();
		m_CurrentDepth--;

		entry.duration = ElapsedMilliseconds(m_FrameStart, Now()) - entry.startTime;
		m_CurrentFrameEntries.push_back(entry);
	}

	void PrintFrameSummary() const {
		std::lock_guard<std::mutex> lock(m_Mutex);

		AQUILA_LOG_INFO("=== Frame Profiler Summary ===");
		AQUILA_LOG_INFO("Frame time: {:.3f} ms ({:.1f} FPS)", m_FrameDuration, m_FPS);
		AQUILA_LOG_INFO("Total frames: {}", m_FrameCount);
		AQUILA_LOG_INFO("");

		std::vector<ProfilerEntry> sorted;
		sorted.reserve(m_Stats.size());
		for (const auto &[name, entry] : m_Stats) {
			sorted.push_back(entry);
		}
		std::ranges::sort(sorted, [](const auto &a, const auto &b) { return a.avgDuration > b.avgDuration; });

		AQUILA_LOG_INFO("{:<40} {:>10} {:>10} {:>10} {:>10} {:>10}", "Section", "Avg (ms)", "Min (ms)", "Max (ms)",
						"% Frame", "Calls/F");
		AQUILA_LOG_INFO("{:-<90}", "");

		for (const auto &entry : sorted) {
			f64 percentage = (entry.avgDuration / m_FrameDuration) * 100.0;
			f64 callsPerFrame = static_cast<f64>(entry.callCount) / entry.frameCount;
			AQUILA_LOG_INFO("{:<40} {:>10.3f} {:>10.3f} {:>10.3f} {:>9.1f}% {:>10.1f}", entry.name, entry.avgDuration,
							entry.minDuration, entry.maxDuration, percentage, callsPerFrame);
		}
		AQUILA_LOG_INFO("");

		if (!m_Bottlenecks.empty()) {
			AQUILA_LOG_WARNING("=== Detected Bottlenecks ===");
			for (const auto &bottleneck : m_Bottlenecks) {
				AQUILA_LOG_WARNING("  - {} ({:.1f}% of frame time)", bottleneck,
								   (m_Stats.at(bottleneck).avgDuration / m_FrameDuration) * 100.0);
			}
			AQUILA_LOG_INFO("");
		}
	}

	void PrintLastFrame() const {
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_CurrentFrameEntries.empty()) {
			return;
		}

		AQUILA_LOG_INFO("=== Last Frame Breakdown ===");
		AQUILA_LOG_INFO("Frame time: {:.3f} ms ({:.1f} FPS)", m_FrameDuration, m_FPS);
		AQUILA_LOG_INFO("");

		for (const auto &entry : m_CurrentFrameEntries) {
			std::string indent(entry.depth * 2, ' ');
			f64 percentage = (entry.duration / m_FrameDuration) * 100.0;
			AQUILA_LOG_INFO("{}{:<35} {:>8.3f} ms ({:>5.1f}%)", indent, entry.name, entry.duration, percentage);
		}
		AQUILA_LOG_INFO("");
	}

	void Reset() {
		std::lock_guard<std::mutex> lock(m_Mutex);

		m_Stats.clear();
		m_FrameHistory.clear();
		m_FrameStatsHistory.clear();
		m_CurrentFrameEntries.clear();
		m_SectionStack.clear();
		m_Bottlenecks.clear();
		m_FrameCount = 0;
		m_FrameNumber = 0;
		m_FrameTimeHistoryIndex = 0;
		m_FrameTimeHistory.fill(0.0f);
	}

	f64 GetFrameDuration() const {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_FrameDuration;
	}

	f64 GetFPS() const {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_FPS;
	}

	uint32 GetFrameCount() const {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_FrameCount;
	}

	uint32 GetFrameNumber() const {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_FrameNumber;
	}

	const std::vector<ProfilerEntry> &GetCurrentFrameEntries() const { return m_CurrentFrameEntries; }

	const std::unordered_map<std::string, ProfilerEntry> &GetStats() const { return m_Stats; }

	const std::vector<std::vector<ProfilerEntry>> &GetFrameHistory() const { return m_FrameHistory; }

	const std::vector<FrameStats> &GetFrameStatsHistory() const { return m_FrameStatsHistory; }

	const std::array<f32, 120> &GetFrameTimeHistory() const { return m_FrameTimeHistory; }

	const std::vector<std::string> &GetBottlenecks() const { return m_Bottlenecks; }

	bool GetSectionStats(const std::string &name, ProfilerEntry &out) const {
		std::lock_guard<std::mutex> lock(m_Mutex);
		auto it = m_Stats.find(name);
		if (it != m_Stats.end()) {
			out = it->second;
			return true;
		}
		return false;
	}

	bool IsEnabled() const { return m_Enabled; }
	void SetEnabled(bool enabled) { m_Enabled = enabled; }

  private:
	Profiler() { m_FrameTimeHistory.fill(0.0f); }

	uint32 HashString(const std::string &str) const {
		uint32 hash = 0x811c9dc5;
		for (char c : str) {
			hash ^= static_cast<uint32>(c);
			hash *= 0x01000193;
		}
		return hash;
	}

	void DetectBottlenecks() {
		m_Bottlenecks.clear();

		constexpr f64 BOTTLENECK_THRESHOLD = 0.20;

		for (const auto &[name, entry] : m_Stats) {
			if (entry.avgDuration / m_FrameDuration > BOTTLENECK_THRESHOLD) {
				m_Bottlenecks.push_back(name);
			}
		}
	}

	mutable std::mutex m_Mutex;
	bool m_Enabled = true;

	TimePoint m_FrameStart;
	f64 m_FrameDuration = 0.0;
	f64 m_FPS = 0.0;
	uint32 m_CurrentDepth = 0;
	uint32 m_FrameCount = 0;
	uint32 m_FrameNumber = 0;

	std::vector<ProfilerEntry> m_SectionStack;
	std::vector<ProfilerEntry> m_CurrentFrameEntries;
	std::unordered_map<std::string, ProfilerEntry> m_Stats;

	std::vector<std::vector<ProfilerEntry>> m_FrameHistory;
	std::vector<FrameStats> m_FrameStatsHistory;
	std::vector<std::string> m_Bottlenecks;

	std::array<f32, 120> m_FrameTimeHistory;
	size_t m_FrameTimeHistoryIndex = 0;

	static constexpr size_t m_MaxHistoryFrames = 60;
};

class ProfileSection {
  public:
	explicit ProfileSection(const std::string &name) : m_Name(name) {
		if (Profiler::Get().IsEnabled()) {
			Profiler::Get().BeginSection(m_Name);
		}
	}

	~ProfileSection() {
		if (Profiler::Get().IsEnabled()) {
			Profiler::Get().EndSection();
		}
	}

  private:
	std::string m_Name;
};

} // namespace Aquila::Utils

#define PROFILE_FRAME_BEGIN() Aquila::Utils::Profiler::Get().BeginFrame()
#define PROFILE_FRAME_END() Aquila::Utils::Profiler::Get().EndFrame()
#define PROFILE_SCOPE(name) Aquila::Utils::ProfileSection _profile_##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)

#endif

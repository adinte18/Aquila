

#include "Aquila/Foundation/Profiler.h"
#include <algorithm>
#include <mutex>

namespace Aquila::Foundation {

void Profiler::BeginFrame() {
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
	m_FrameStart = Now();
	m_CurrentDepth = 0;
	m_CurrentFrameEntries.clear();
	m_FrameNumber++;
}

void Profiler::EndFrame() {
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	m_FrameDuration = ElapsedMilliseconds(m_FrameStart, Now());

	// Sort by startTime so parents appear before their children throughout the rest of EndFrame,
	// PrintLastFrame, and the stored frame history.
	std::sort(m_CurrentFrameEntries.begin(), m_CurrentFrameEntries.end(),
			  [](const ProfilerEntry &a, const ProfilerEntry &b) { return a.startTime < b.startTime; });
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

void Profiler::BeginSection(const std::string &name) {
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

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

void Profiler::EndSection() {
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	if (m_SectionStack.empty()) {
		return;
	}

	auto entry = m_SectionStack.back();
	m_SectionStack.pop_back();
	m_CurrentDepth--;

	entry.duration = ElapsedMilliseconds(m_FrameStart, Now()) - entry.startTime;
	m_CurrentFrameEntries.push_back(entry);
}

void Profiler::PrintFrameSummary() const {
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

	AQUILA_LOG_INFO("=== Frame Profiler Summary ===");
	AQUILA_LOG_INFO("Frame time: {:.3f} ms ({:.1f} FPS)", m_FrameDuration, m_FPS);
	AQUILA_LOG_INFO("Total frames: {}", m_FrameCount);
	AQUILA_LOG_INFO("");

	if (m_FrameHistory.empty()) {
		return;
	}

	AQUILA_LOG_INFO("{:<50} {:>10} {:>10} {:>10} {:>9}", "Section", "Avg (ms)", "Min (ms)", "Max (ms)", "% Frame");
	AQUILA_LOG_INFO("{:-<93}", "");

	// Walk the last frame in startTime order (sorted in EndFrame) so the tree prints correctly.
	// Look up aggregated stats by name for the avg/min/max columns.
	for (const auto &frameEntry : m_FrameHistory.back()) {
		auto it = m_Stats.find(frameEntry.name);
		if (it == m_Stats.end()) {
			continue;
		}
		const ProfilerEntry &stats = it->second;
		std::string label(frameEntry.depth * 2, ' ');
		label += frameEntry.name;
		f64 percentage = m_FrameDuration > 0.0 ? (stats.avgDuration / m_FrameDuration) * 100.0 : 0.0;
		AQUILA_LOG_INFO("{:<50} {:>10.3f} {:>10.3f} {:>10.3f} {:>8.1f}%", label, stats.avgDuration, stats.minDuration,
						stats.maxDuration, percentage);
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

void Profiler::PrintLastFrame() const {
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

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

void Profiler::Reset() {
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);

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

f64 Profiler::GetFrameDuration() const {
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
	return m_FrameDuration;
}
f64 Profiler::GetFPS() const {
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
	return m_FPS;
}
uint32 Profiler::GetFrameCount() const {
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
	return m_FrameCount;
}
uint32 Profiler::GetFrameNumber() const {
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
	return m_FrameNumber;
}
bool Profiler::IsEnabled() const {
	return m_Enabled;
}
void Profiler::SetEnabled(bool enabled) {
	m_Enabled = enabled;
}

const std::vector<ProfilerEntry> &Profiler::GetCurrentFrameEntries() const {
	return m_CurrentFrameEntries;
}
const std::unordered_map<std::string, ProfilerEntry> &Profiler::GetStats() const {
	return m_Stats;
}
const std::vector<std::vector<ProfilerEntry>> &Profiler::GetFrameHistory() const {
	return m_FrameHistory;
}
const std::vector<FrameStats> &Profiler::GetFrameStatsHistory() const {
	return m_FrameStatsHistory;
}
const std::array<f32, 120> &Profiler::GetFrameTimeHistory() const {
	return m_FrameTimeHistory;
}
const std::vector<std::string> &Profiler::GetBottlenecks() const {
	return m_Bottlenecks;
}

bool Profiler::GetSectionStats(const std::string &name, ProfilerEntry &out) const {
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
	auto it = m_Stats.find(name);
	if (it != m_Stats.end()) {
		out = it->second;
		return true;
	}
	return false;
}

uint32 Profiler::HashString(const std::string &str) const {
	uint32 hash = 0x811c9dc5;
	for (char c : str) {
		hash ^= static_cast<uint32>(c);
		hash *= 0x01000193;
	}
	return hash;
}

void Profiler::DetectBottlenecks() {
	m_Bottlenecks.clear();
	constexpr f64 BOTTLENECK_THRESHOLD = 0.20;
	for (const auto &[name, entry] : m_Stats) {
		if (entry.avgDuration / m_FrameDuration > BOTTLENECK_THRESHOLD) {
			m_Bottlenecks.push_back(name);
		}
	}
}

ProfileSection::ProfileSection(const std::string &name) : m_Name(name) {
	if (Profiler::Get()->IsEnabled()) {
		Profiler::Get()->BeginSection(m_Name);
	}
}

ProfileSection::~ProfileSection() {
	if (Profiler::Get()->IsEnabled()) {
		Profiler::Get()->EndSection();
	}
}

} // namespace Aquila::Foundation

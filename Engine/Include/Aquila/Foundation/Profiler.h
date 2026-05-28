#ifndef AQUILA_PROFILER_H
#define AQUILA_PROFILER_H

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Singleton.h"
#include "Aquila/Foundation/Timer.h"
#include "Aquila/Foundation/Log.h"

namespace Aquila::Foundation {

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

class Profiler : public Singleton<Profiler> {
	friend class Singleton<Profiler>;

  public:
	void BeginFrame();
	void EndFrame();
	void BeginSection(const std::string &name);
	void EndSection();
	void PrintFrameSummary() const;
	void PrintLastFrame() const;
	void Reset();

	f64 GetFrameDuration() const;
	f64 GetFPS() const;
	uint32 GetFrameCount() const;
	uint32 GetFrameNumber() const;
	bool IsEnabled() const;
	void SetEnabled(bool enabled);

	bool GetSectionStats(const std::string &name, ProfilerEntry &out) const;

	const std::vector<ProfilerEntry> &GetCurrentFrameEntries() const;
	const std::unordered_map<std::string, ProfilerEntry> &GetStats() const;
	const std::vector<std::vector<ProfilerEntry>> &GetFrameHistory() const;
	const std::vector<FrameStats> &GetFrameStatsHistory() const;
	const std::array<f32, 120> &GetFrameTimeHistory() const;
	const std::vector<std::string> &GetBottlenecks() const;

  private:
	Profiler() { m_FrameTimeHistory.fill(0.0F); }

	uint32 HashString(const std::string &str) const;
	void DetectBottlenecks();

	mutable std::recursive_mutex m_Mutex;
	bool m_Enabled = true;

	TimePoint m_FrameStart;
	f64 m_FrameDuration = 0.0;
	f64 m_FPS = 0.0;
	uint32 m_CurrentDepth = 0;
	uint32 m_FrameCount = 0;
	uint32 m_FrameNumber = 0;
	size_t m_FrameTimeHistoryIndex = 0;

	std::vector<ProfilerEntry> m_SectionStack;
	std::vector<ProfilerEntry> m_CurrentFrameEntries;
	std::unordered_map<std::string, ProfilerEntry> m_Stats;
	std::vector<std::vector<ProfilerEntry>> m_FrameHistory;
	std::vector<FrameStats> m_FrameStatsHistory;
	std::vector<std::string> m_Bottlenecks;
	std::array<f32, 120> m_FrameTimeHistory{};

	static constexpr size_t m_MaxHistoryFrames = 60;
};

class ProfileSection {
  public:
	explicit ProfileSection(const std::string &name);
	~ProfileSection();

  private:
	std::string m_Name;
};

} // namespace Aquila::Foundation

#define PROFILE_FRAME_BEGIN() Aquila::Foundation::Profiler::Get()->BeginFrame()
#define PROFILE_FRAME_END() Aquila::Foundation::Profiler::Get()->EndFrame()
#define PROFILE_SCOPE(name) Aquila::Foundation::ProfileSection _profile_##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
#define PROFILE_PRINT_SUMMARY_EVERY_N_FRAMES(n)                                 \
	do {                                                                        \
		if (Aquila::Foundation::Profiler::Get()->GetFrameNumber() % (n) == 0) { \
			Aquila::Foundation::Profiler::Get()->PrintFrameSummary();           \
		}                                                                       \
	} while (0)
#endif

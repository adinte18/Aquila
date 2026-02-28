#pragma once

#include "Aquila/Core/Layer.h"
#include "Aquila/Utilities/Profiler.h"
#include "imgui.h"

namespace Editor::UI {

class ProfilerWindow : public Aquila::Core::Layer {
  public:
	ProfilerWindow();
	~ProfilerWindow() override = default;

	void OnImGuiRender() override;

  private:
	void DrawMenuBar(Aquila::Utils::Profiler &profiler);
	void DrawPerformanceOverview(Aquila::Utils::Profiler &profiler);
	void DrawCurrentFrameTab(Aquila::Utils::Profiler &profiler);
	void DrawStatisticsTab(Aquila::Utils::Profiler &profiler);
	void DrawFlameGraphTab(Aquila::Utils::Profiler &profiler);
	void DrawFrameHistoryTab(Aquila::Utils::Profiler &profiler);
	void DrawBottlenecksTab(Aquila::Utils::Profiler &profiler);

	void DrawEntry(const Aquila::Utils::ProfilerEntry &entry, double frameDuration);
	ImVec4 GetColorForHash(uint32_t hash) const;

	void ExportToCSV(Aquila::Utils::Profiler &profiler);
	void ExportToJSON(Aquila::Utils::Profiler &profiler);

  private:
	bool m_ShowPercentages = true;
	bool m_ShowCallCounts = true;
	bool m_ShowSparklines = true;
	bool m_AutoScroll = false;

	f32 m_TargetFPS = 60.0f;
	f32 m_TargetFrameTime = 16.67f;
};

} // namespace Editor::UI

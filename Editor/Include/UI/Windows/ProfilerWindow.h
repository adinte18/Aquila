#pragma once

#include "Aquila/Core/Layer.h"
#include "Aquila/Foundation/Profiler.h"
#include "imgui.h"

namespace Editor::UI {

class ProfilerWindow : public Aquila::Core::Layer {
  public:
	ProfilerWindow();
	~ProfilerWindow() override = default;

	void OnImGuiRender() override;

  private:
	void DrawMenuBar(Aquila::Foundation::Profiler &profiler);
	void DrawPerformanceOverview(Aquila::Foundation::Profiler &profiler);
	void DrawCurrentFrameTab(Aquila::Foundation::Profiler &profiler);
	void DrawStatisticsTab(Aquila::Foundation::Profiler &profiler);
	void DrawFlameGraphTab(Aquila::Foundation::Profiler &profiler);
	void DrawFrameHistoryTab(Aquila::Foundation::Profiler &profiler);
	void DrawBottlenecksTab(Aquila::Foundation::Profiler &profiler);

	void DrawEntry(const Aquila::Foundation::ProfilerEntry &entry, double frameDuration);
	ImVec4 GetColorForHash(uint32_t hash) const;

	void ExportToCSV(Aquila::Foundation::Profiler &profiler);
	void ExportToJSON(Aquila::Foundation::Profiler &profiler);

  private:
	bool m_ShowPercentages = true;
	bool m_ShowCallCounts = true;
	bool m_ShowSparklines = true;
	bool m_AutoScroll = false;

	f32 m_TargetFPS = 60.0f;
	f32 m_TargetFrameTime = 16.67f;
};

} // namespace Editor::UI

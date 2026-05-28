#include "UI/Windows/ProfilerWindow.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "Aquila/Core/Defines.h"
#include "lucide.h"

namespace Editor::UI {

ProfilerWindow::ProfilerWindow() {
	m_TargetFrameTime = 1000.0f / 60.0f;
}

void ProfilerWindow::OnImGuiRender() {
	using namespace Aquila::Foundation;
	auto &profiler = Profiler::Get();

	ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
	ImGui::Begin(ICON_LC_CHART_PIE " Profiler", nullptr, ImGuiWindowFlags_MenuBar);

	DrawMenuBar(profiler);

	ImGui::BeginChild("ProfilerContent", ImVec2(0, 0), false);

	DrawPerformanceOverview(profiler);

	ImGui::Separator();

	if (ImGui::BeginTabBar("ProfilerTabs")) {
		if (ImGui::BeginTabItem(ICON_LC_CLOCK " Current Frame")) {
			DrawCurrentFrameTab(profiler);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem(ICON_LC_CHART_BAR " Statistics")) {
			DrawStatisticsTab(profiler);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem(ICON_LC_FLAME " Flame Graph")) {
			DrawFlameGraphTab(profiler);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem(ICON_LC_GALLERY_HORIZONTAL_END " Frame History")) {
			DrawFrameHistoryTab(profiler);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem(ICON_LC_CLOCK_ALERT " Bottlenecks")) {
			DrawBottlenecksTab(profiler);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::EndChild();
	ImGui::End();
}

void ProfilerWindow::DrawMenuBar(Aquila::Foundation::Profiler &profiler) {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("Actions")) {
			if (ImGui::MenuItem(ICON_LC_REFRESH_CW " Reset Stats", "Ctrl+R")) {
				profiler.Reset();
			}

			ImGui::Separator();

			if (ImGui::MenuItem(ICON_LC_SAVE " Export CSV")) {
				ExportToCSV(profiler);
			}

			if (ImGui::MenuItem(ICON_LC_DOWNLOAD " Export JSON")) {
				ExportToJSON(profiler);
			}

			ImGui::Separator();

			bool enabled = profiler.IsEnabled();
			if (ImGui::MenuItem(ICON_LC_POWER " Enable Profiling", nullptr, &enabled)) {
				profiler.SetEnabled(enabled);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")) {
			ImGui::Checkbox("Show Percentages", &m_ShowPercentages);
			ImGui::Checkbox("Show Call Counts", &m_ShowCallCounts);
			ImGui::Checkbox("Show Sparklines", &m_ShowSparklines);
			ImGui::Checkbox("Auto-scroll to Bottom", &m_AutoScroll);

			ImGui::Separator();

			ImGui::SliderFloat("Target FPS", &m_TargetFPS, 30.0f, 144.0f, "%.0f");
			m_TargetFrameTime = 1000.0f / m_TargetFPS;

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}

void ProfilerWindow::DrawPerformanceOverview(Aquila::Foundation::Profiler &profiler) {
	double frameDuration = profiler.GetFrameDuration();
	double fps = profiler.GetFPS();

	ImVec4 fpsColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
	if (fps < 60.0) {
		fpsColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
	}
	if (fps < 30.0) {
		fpsColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	}

	ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
	ImGui::TextColored(fpsColor, "%.1f FPS", fps);
	ImGui::PopFont();

	ImGui::SameLine();
	ImGui::Text("  |  Frame: %.3f ms", frameDuration);
	ImGui::SameLine();
	ImGui::Text("  |  Frames: %u", profiler.GetFrameCount());

	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, fpsColor);
	f32 fraction = static_cast<f32>(frameDuration / m_TargetFrameTime);
	ImGui::ProgressBar(fraction, ImVec2(-1, 0), "");
	ImGui::PopStyleColor();

	const auto &frameHistory = profiler.GetFrameTimeHistory();
	ImGui::PlotLines("##FrameTime", frameHistory.data(), frameHistory.size(), 0, nullptr, 0.0f,
					 m_TargetFrameTime * 2.0f, ImVec2(-1, 60));

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	ImVec2 graphMin = ImGui::GetItemRectMin();
	ImVec2 graphMax = ImGui::GetItemRectMax();
	f32 targetY = graphMin.y + (graphMax.y - graphMin.y) * (1.0f - (m_TargetFrameTime / (m_TargetFrameTime * 2.0f)));
	drawList->AddLine(ImVec2(graphMin.x, targetY), ImVec2(graphMax.x, targetY), IM_COL32(255, 255, 0, 128), 1.0f);
}

void ProfilerWindow::DrawCurrentFrameTab(Aquila::Foundation::Profiler &profiler) {
	const auto &entries = profiler.GetCurrentFrameEntries();
	double frameDuration = profiler.GetFrameDuration();

	if (entries.empty()) {
		ImGui::TextDisabled("No profiling data available");
		return;
	}

	ImGui::Text("Last frame breakdown:");
	ImGui::Separator();

	if (ImGui::BeginTable("CurrentFrame", m_ShowCallCounts ? 4 : 3,
						  ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
		ImGui::TableSetupColumn("Section", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Time (ms)", ImGuiTableColumnFlags_WidthFixed, 100);
		if (m_ShowCallCounts) {
			ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 60);
		}
		ImGui::TableSetupColumn("Graph", ImGuiTableColumnFlags_WidthFixed, 200);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		for (const auto &entry : entries) {
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			std::string indent(entry.depth * 2, ' ');
			ImGui::Text("%s%s", indent.c_str(), entry.name.c_str());

			ImGui::TableNextColumn();
			double percent = (entry.duration / frameDuration) * 100.0;
			if (m_ShowPercentages) {
				ImGui::Text("%.3f (%.1f%%)", entry.duration, percent);
			} else {
				ImGui::Text("%.3f", entry.duration);
			}

			if (m_ShowCallCounts) {
				ImGui::TableNextColumn();
				ImGui::Text("%u", entry.callCount);
			}

			ImGui::TableNextColumn();
			ImVec4 color = GetColorForHash(entry.color);
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
			ImGui::ProgressBar(static_cast<f32>(entry.duration / frameDuration), ImVec2(-1, 0), "");
			ImGui::PopStyleColor();
		}

		ImGui::EndTable();
	}
}

void ProfilerWindow::DrawStatisticsTab(Aquila::Foundation::Profiler &profiler) {
	const auto &stats = profiler.GetStats();

	if (stats.empty()) {
		ImGui::TextDisabled("No statistics available");
		return;
	}

	static int sortMode = 0;
	ImGui::Text("Sort by:");
	ImGui::SameLine();
	ImGui::RadioButton("Avg", &sortMode, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Max", &sortMode, 1);
	ImGui::SameLine();
	ImGui::RadioButton("Calls", &sortMode, 2);

	ImGui::Separator();

	std::vector<Aquila::Foundation::ProfilerEntry> sorted;
	for (const auto &[name, entry] : stats) {
		sorted.push_back(entry);
	}

	switch (sortMode) {
	case 0:
		std::sort(sorted.begin(), sorted.end(),
				  [](const auto &a, const auto &b) { return a.avgDuration > b.avgDuration; });
		break;
	case 1:
		std::sort(sorted.begin(), sorted.end(),
				  [](const auto &a, const auto &b) { return a.maxDuration > b.maxDuration; });
		break;
	case 2:
		std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return a.callCount > b.callCount; });
		break;
	}

	int columns = 5;
	if (m_ShowSparklines) {
		columns++;
	}
	if (m_ShowCallCounts) {
		columns++;
	}

	if (ImGui::BeginTable("Stats", columns,
						  ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
							  ImGuiTableFlags_Sortable)) {
		ImGui::TableSetupColumn("Section", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("Min (ms)", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("Max (ms)", ImGuiTableColumnFlags_WidthFixed, 80);
		ImGui::TableSetupColumn("% Frame", ImGuiTableColumnFlags_WidthFixed, 70);
		if (m_ShowCallCounts) {
			ImGui::TableSetupColumn("Calls/F", ImGuiTableColumnFlags_WidthFixed, 70);
		}
		if (m_ShowSparklines) {
			ImGui::TableSetupColumn("Trend", ImGuiTableColumnFlags_WidthFixed, 120);
		}
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		double frameDuration = profiler.GetFrameDuration();

		for (const auto &entry : sorted) {
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::Text("%s", entry.name.c_str());

			ImGui::TableNextColumn();
			ImGui::Text("%.3f", entry.avgDuration);

			ImGui::TableNextColumn();
			ImGui::Text("%.3f", entry.minDuration);

			ImGui::TableNextColumn();
			ImGui::Text("%.3f", entry.maxDuration);

			ImGui::TableNextColumn();
			double percent = (entry.avgDuration / frameDuration) * 100.0;
			ImGui::Text("%.1f%%", percent);

			if (m_ShowCallCounts) {
				ImGui::TableNextColumn();
				double callsPerFrame = static_cast<double>(entry.callCount) / entry.frameCount;
				ImGui::Text("%.1f", callsPerFrame);
			}

			if (m_ShowSparklines) {
				ImGui::TableNextColumn();
				ImVec4 color = GetColorForHash(entry.color);
				ImGui::PushStyleColor(ImGuiCol_PlotLines, color);
				ImGui::PlotLines("##sparkline", entry.recentDurations.data(), entry.recentDurations.size(), 0, nullptr,
								 0.0f, entry.maxDuration * 1.2f, ImVec2(120, 30));
				ImGui::PopStyleColor();
			}
		}

		ImGui::EndTable();
	}
}

void ProfilerWindow::DrawFlameGraphTab(Aquila::Foundation::Profiler &profiler) {
	const auto &entries = profiler.GetCurrentFrameEntries();
	double frameDuration = profiler.GetFrameDuration();

	if (entries.empty()) {
		ImGui::TextDisabled("No profiling data available");
		return;
	}

	ImGui::Text("Flame graph visualization:");
	ImGui::Separator();

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize = ImGui::GetContentRegionAvail();
	canvasSize.y = 400.0f;

	drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
							IM_COL32(30, 30, 30, 255));

	const f32 barHeight = 20.0f;
	const f32 padding = 2.0f;

	for (const auto &entry : entries) {
		f32 x = canvasPos.x + (entry.startTime / frameDuration) * canvasSize.x;
		f32 width = (entry.duration / frameDuration) * canvasSize.x;
		f32 y = canvasPos.y + entry.depth * (barHeight + padding);

		ImVec4 colorVec = GetColorForHash(entry.color);
		ImU32 color = ImGui::ColorConvertFloat4ToU32(colorVec);

		drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + barHeight), color, 2.0f);

		drawList->AddRect(ImVec2(x, y), ImVec2(x + width, y + barHeight), IM_COL32(0, 0, 0, 255), 2.0f);

		if (width > 50.0f) {
			drawList->AddText(ImVec2(x + 4, y + 2), IM_COL32(255, 255, 255, 255), entry.name.c_str());
		}

		if (ImGui::IsMouseHoveringRect(ImVec2(x, y), ImVec2(x + width, y + barHeight))) {
			ImGui::BeginTooltip();
			ImGui::Text("%s", entry.name.c_str());
			ImGui::Text("Time: %.3f ms (%.1f%%)", entry.duration, (entry.duration / frameDuration) * 100.0);
			ImGui::Text("Depth: %u", entry.depth);
			ImGui::EndTooltip();
		}
	}

	ImGui::Dummy(canvasSize);
}

void ProfilerWindow::DrawFrameHistoryTab(Aquila::Foundation::Profiler &profiler) {
	const auto &history = profiler.GetFrameStatsHistory();

	if (history.empty()) {
		ImGui::TextDisabled("No frame history available");
		return;
	}

	std::vector<f32> frameTimes;
	std::vector<f32> cpuTimes;

	frameTimes.reserve(history.size());
	cpuTimes.reserve(history.size());

	for (const auto &frame : history) {
		frameTimes.push_back(static_cast<f32>(frame.frameDuration));
		cpuTimes.push_back(static_cast<f32>(frame.cpuTime));
	}

	f32 avgFrameTime = 0.0f;
	f32 minFrameTime = FLT_MAX;
	f32 maxFrameTime = 0.0f;

	for (f32 time : frameTimes) {
		avgFrameTime += time;
		minFrameTime = std::min(minFrameTime, time);
		maxFrameTime = std::max(maxFrameTime, time);
	}
	avgFrameTime /= frameTimes.size();

	ImGui::Text("Statistics over last %zu frames:", history.size());
	ImGui::Text("  Average: %.3f ms (%.1f FPS)", avgFrameTime, 1000.0f / avgFrameTime);
	ImGui::Text("  Min: %.3f ms (%.1f FPS)", minFrameTime, 1000.0f / minFrameTime);
	ImGui::Text("  Max: %.3f ms (%.1f FPS)", maxFrameTime, 1000.0f / maxFrameTime);

	ImGui::Separator();

	ImGui::Text("Frame Time:");
	ImGui::PlotLines("##FrameTimePlot", frameTimes.data(), frameTimes.size(), 0, nullptr, 0.0f, maxFrameTime * 1.2f,
					 ImVec2(-1, 150));

	ImGui::Text("CPU Time:");
	ImGui::PlotLines("##CPUTimePlot", cpuTimes.data(), cpuTimes.size(), 0, nullptr, 0.0f, maxFrameTime * 1.2f,
					 ImVec2(-1, 150));
}

void ProfilerWindow::DrawBottlenecksTab(Aquila::Foundation::Profiler &profiler) {
	const auto &bottlenecks = profiler.GetBottlenecks();
	double frameDuration = profiler.GetFrameDuration();

	if (bottlenecks.empty()) {
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ICON_LC_CIRCLE_CHECK " No bottlenecks detected!");
		ImGui::TextDisabled("(Sections taking >20%% of frame time are considered bottlenecks)");
		return;
	}

	ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ICON_LC_CIRCLE_ALERT " %zu bottleneck(s) detected!",
					   bottlenecks.size());
	ImGui::Separator();

	for (const auto &name : bottlenecks) {
		Aquila::Foundation::ProfilerEntry entry;
		if (profiler.GetSectionStats(name, entry)) {
			double percent = (entry.avgDuration / frameDuration) * 100.0;

			ImGui::PushID(name.c_str());

			if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Indent();

				ImGui::Text("Average Time: %.3f ms", entry.avgDuration);
				ImGui::Text("Frame Impact: %.1f%%", percent);
				ImGui::Text("Min/Max: %.3f / %.3f ms", entry.minDuration, entry.maxDuration);

				double callsPerFrame = static_cast<double>(entry.callCount) / entry.frameCount;
				ImGui::Text("Calls per Frame: %.1f", callsPerFrame);

				ImVec4 color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
				ImGui::ProgressBar(static_cast<f32>(percent / 100.0), ImVec2(-1, 0), "");
				ImGui::PopStyleColor();

				ImGui::PlotLines("##trend", entry.recentDurations.data(), entry.recentDurations.size(), 0, nullptr,
								 0.0f, entry.maxDuration * 1.2f, ImVec2(-1, 60));

				ImGui::Unindent();
			}

			ImGui::PopID();
		}
	}
}

void ProfilerWindow::DrawEntry(const Aquila::Foundation::ProfilerEntry &entry, double frameDuration) {
	std::string indent(entry.depth * 2, ' ');
	double percent = (entry.duration / frameDuration) * 100.0;

	ImGui::Text("%s%s: %.3f ms (%.1f%%)", indent.c_str(), entry.name.c_str(), entry.duration, percent);
	ImGui::SameLine();

	ImVec4 color = GetColorForHash(entry.color);
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
	ImGui::ProgressBar(static_cast<f32>(entry.duration / frameDuration), ImVec2(200, 0), "");
	ImGui::PopStyleColor();
}

ImVec4 ProfilerWindow::GetColorForHash(uint32 hash) const {
	f32 hue = (hash % 360) / 360.0f;
	f32 sat = 0.7f;
	f32 val = 0.9f;

	f32 r, g, b;
	ImGui::ColorConvertHSVtoRGB(hue, sat, val, r, g, b);
	return ImVec4(r, g, b, 1.0f);
}

void ProfilerWindow::ExportToCSV(Aquila::Foundation::Profiler &profiler) {
	const auto &stats = profiler.GetStats();

	std::ofstream file("profiler_export.csv");
	if (!file.is_open()) {
		AQUILA_LOG_ERROR("Failed to open profiler_export.csv for writing");
		return;
	}

	file << "Section,Avg(ms),Min(ms),Max(ms),PercentFrame,CallsPerFrame,TotalCalls\n";

	for (const auto &[name, entry] : stats) {
		double percent = (entry.avgDuration / profiler.GetFrameDuration()) * 100.0;
		double callsPerFrame = static_cast<double>(entry.callCount) / entry.frameCount;

		file << name << "," << entry.avgDuration << "," << entry.minDuration << "," << entry.maxDuration << ","
			 << percent << "," << callsPerFrame << "," << entry.callCount << "\n";
	}

	file.close();
	AQUILA_LOG_INFO("Profiler data exported to profiler_export.csv");
}

void ProfilerWindow::ExportToJSON(Aquila::Foundation::Profiler &profiler) {
	const auto &stats = profiler.GetStats();

	std::ofstream file("profiler_export.json");
	if (!file.is_open()) {
		AQUILA_LOG_ERROR("Failed to open profiler_export.json for writing");
		return;
	}

	file << "{\n";
	file << "  \"frameTime\": " << profiler.GetFrameDuration() << ",\n";
	file << "  \"fps\": " << profiler.GetFPS() << ",\n";
	file << "  \"frameCount\": " << profiler.GetFrameCount() << ",\n";
	file << "  \"sections\": [\n";

	bool first = true;
	for (const auto &[name, entry] : stats) {
		if (!first) {
			file << ",\n";
		}
		first = false;

		double percent = (entry.avgDuration / profiler.GetFrameDuration()) * 100.0;
		double callsPerFrame = static_cast<double>(entry.callCount) / entry.frameCount;

		file << "    {\n";
		file << "      \"name\": \"" << name << "\",\n";
		file << "      \"avgDuration\": " << entry.avgDuration << ",\n";
		file << "      \"minDuration\": " << entry.minDuration << ",\n";
		file << "      \"maxDuration\": " << entry.maxDuration << ",\n";
		file << "      \"percentFrame\": " << percent << ",\n";
		file << "      \"callsPerFrame\": " << callsPerFrame << ",\n";
		file << "      \"totalCalls\": " << entry.callCount << "\n";
		file << "    }";
	}

	file << "\n  ]\n";
	file << "}\n";

	file.close();
	AQUILA_LOG_INFO("Profiler data exported to profiler_export.json");
}

} // namespace Editor::UI

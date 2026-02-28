#include "UI/Windows/AboutWindow.h"
#include "Core/EditorConfig.h"
#include "Aquila/Core/Defines.h"
#include "UI/Managers/FontManager.h"
#include "lucide.h"
#include <imgui.h>
#include <vulkan/vulkan.h>

namespace Editor::UI {

AboutWindow::AboutWindow() : Layer("AboutWindow") {
	AQUILA_LOG_INFO("AboutWindow created");
}

void AboutWindow::OnImGuiRender() {
	if (!m_IsOpen) {
		return;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);

	if (ImGui::Begin(ICON_LC_INFO " About Aquila Editor", &m_IsOpen)) {
		// Header with logo/title
		ImGui::PushFont(nullptr); // Use default or larger font
		ImGui::TextUnformatted(Config::EDITOR_NAME);
		ImGui::PopFont();

		ImGui::Text("Version %s", Config::EDITOR_VERSION);
		ImGui::Text("Build Date: %s", Config::EDITOR_BUILD_DATE);
		ImGui::Separator();

		// Tab bar
		if (ImGui::BeginTabBar("AboutTabs")) {
			if (ImGui::BeginTabItem(ICON_LC_INFO " Information")) {
				ImGui::Spacing();

				ImGui::PushFont(nullptr, 5.0F);
				ImGui::TextWrapped(
					"Aquila is a game engine editor built with Vulkan, "
					"designed for 3D rendering and MAYBE game development in the future. This project is mainly a "
					"hobby project "
					"where the point is to learn about application development. I am very interested in the low level "
					"parts of Vulkan. This engine is extremely far from being optimal or fast or whatever epithet or "
					"description you want to give it. Mainly built for fun and kept alive for the jokes. ");
				ImGui::PopFont();
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Text("Features:");
				ImGui::BulletText("Vulkan-based rendering");
				ImGui::BulletText("Scene management and entity-component system");
				ImGui::BulletText("Material editor with real-time preview");
				ImGui::BulletText("Centralized asset browser and management");
				ImGui::BulletText("Customizable(-ish) themes and UI");

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Text("Links:");
				if (ImGui::Button(ICON_LC_GLOBE " Website")) {
					// TODO: Open browser to website
					AQUILA_LOG_INFO("Opening website...");
				}
				ImGui::SameLine();
				if (ImGui::Button(ICON_LC_GITHUB " GitHub")) {
#if defined(AQUILA_PLATFORM_WINDOWS)
					ShellExecute(nullptr, "open", "https://github.com/adinte18/Aquila/", nullptr, nullptr,
								 SW_SHOWNORMAL);
#elif defined(AQUILA_PLATFORM_LINUX)
					system("xdg-open https://github.com/adinte18/Aquila/");
#elif defined(AQUILA_PLATFORM_MACOS)
					system("open https://github.com/adinte18/Aquila/");
#endif
				}
				ImGui::SameLine();
				if (ImGui::Button(ICON_LC_FILE_TEXT " Documentation")) {
#if defined(AQUILA_PLATFORM_WINDOWS)
					ShellExecute(nullptr, "open", "https://github.com/adinte18/Aquila/wiki", nullptr, nullptr,
								 SW_SHOWNORMAL);
#elif defined(AQUILA_PLATFORM_LINUX)
					system("xdg-open https://github.com/adinte18/Aquila/wiki");
#elif defined(AQUILA_PLATFORM_MACOS)
					system("open https://github.com/adinte18/Aquila/wiki");
#endif
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(ICON_LC_MONITOR " System Info")) {
				RenderSystemInfo();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(ICON_LC_USERS " Credits")) {
				RenderCredits();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Footer
		f32 buttonWidth = 100.0f;
		f32 offsetX = (ImGui::GetContentRegionAvail().x - buttonWidth) * 0.5f;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

		if (ImGui::Button(ICON_LC_X " Close", ImVec2(buttonWidth, 0))) {
			m_IsOpen = false;
		}
	}
	ImGui::End();
}

void AboutWindow::RenderSystemInfo() {
	ImGui::Spacing();

	if (ImGui::BeginTable("SystemInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
		ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 200.0f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();

		// Platform
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Platform");
		ImGui::TableNextColumn();
#ifdef AQUILA_PLATFORM_WINDOWS
		ImGui::Text("Windows");
#elif defined(AQUILA_PLATFORM_LINUX)
		ImGui::Text("Linux");
#elif defined(AQUILA_PLATFORM_MACOS)
		ImGui::Text("macOS");
#else
		ImGui::Text("Unknown");
#endif

		// Architecture
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Architecture");
		ImGui::TableNextColumn();
#ifdef AQUILA_64BIT
		ImGui::Text("64-bit");
#else
		ImGui::Text("32-bit");
#endif

		// Compiler
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Compiler");
		ImGui::TableNextColumn();
#ifdef _MSC_VER
		ImGui::Text("MSVC %d", _MSC_VER);
#elif defined(__clang__)
		ImGui::Text("Clang %d.%d.%d", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__)
		ImGui::Text("GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#else
		ImGui::Text("Unknown");
#endif

		// Build Type
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Build Type");
		ImGui::TableNextColumn();
#ifdef NDEBUG
		ImGui::Text("Release");
#else
		ImGui::Text("Debug");
#endif

		// Vulkan Version
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("Vulkan Version");
		ImGui::TableNextColumn();
		ImGui::Text("%d.%d.%d", VK_VERSION_MAJOR(VK_API_VERSION_1_3), VK_VERSION_MINOR(VK_API_VERSION_1_3),
					VK_VERSION_PATCH(VK_API_VERSION_1_3));

		// ImGui Version
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("ImGui Version");
		ImGui::TableNextColumn();
		ImGui::Text("%s", ImGui::GetVersion());

		ImGui::EndTable();
	}
}

void AboutWindow::RenderCredits() {
	ImGui::Spacing();

	ImGui::TextUnformatted("Development Team:");
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::BulletText("Lead (and the only) Developer:  Alex");

	ImGui::Spacing();
}

} // namespace Editor::UI

#include "UI/ThemeManager.h"

namespace Editor::UI {
    ThemeManager& ThemeManager::Get() {
        static ThemeManager instance;
        return instance;
    }

    void ThemeManager::ApplyAquilaTheme() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Background colors
        colors[ImGuiCol_WindowBg]           = HEXAtoIV4("#1b1b1f", 1.0f);  // Dark background
        colors[ImGuiCol_ChildBg]            = HEXAtoIV4("#23272e", 1.0f);
        colors[ImGuiCol_PopupBg]            = HEXAtoIV4("#181a1f", 0.98f);

        colors[ImGuiCol_Header]             = HEXAtoIV4("#2f3136", 1.0f);
        colors[ImGuiCol_HeaderHovered]      = HEXAtoIV4("#393c43", 1.0f);
        colors[ImGuiCol_HeaderActive]       = HEXAtoIV4("#41454d", 1.0f);

        colors[ImGuiCol_Border]             = HEXAtoIV4("#2c2f33", 1.0f);
        colors[ImGuiCol_BorderShadow]       = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);

        colors[ImGuiCol_FrameBg]            = HEXAtoIV4("#2f3136", 1.0f);
        colors[ImGuiCol_FrameBgHovered]     = HEXAtoIV4("#393c43", 1.0f);
        colors[ImGuiCol_FrameBgActive]      = HEXAtoIV4("#41454d", 1.0f);

        colors[ImGuiCol_Button]             = HEXAtoIV4("#4b5058", 1.0f);
        colors[ImGuiCol_ButtonHovered]      = HEXAtoIV4("#565c66", 1.0f);
        colors[ImGuiCol_ButtonActive]       = HEXAtoIV4("#2e333a", 1.0f);

        colors[ImGuiCol_ScrollbarBg]        = HEXAtoIV4("#1e2126", 1.0f);
        colors[ImGuiCol_ScrollbarGrab]      = HEXAtoIV4("#2f3136", 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered] = HEXAtoIV4("#393c43", 1.0f);
        colors[ImGuiCol_ScrollbarGrabActive] = HEXAtoIV4("#41454d", 1.0f);

        colors[ImGuiCol_Tab]                = HEXAtoIV4("#2f3136", 1.0f);
        colors[ImGuiCol_TabHovered]         = HEXAtoIV4("#393c43", 1.0f);
        colors[ImGuiCol_TabActive]          = HEXAtoIV4("#41454d", 1.0f);
        colors[ImGuiCol_TabUnfocused]       = HEXAtoIV4("#2f3136", 1.0f);
        colors[ImGuiCol_TabUnfocusedActive] = HEXAtoIV4("#393c43", 1.0f);

        colors[ImGuiCol_TitleBg]            = HEXAtoIV4("#1b1b1f", 1.0f);
        colors[ImGuiCol_TitleBgActive]      = HEXAtoIV4("#23272e", 1.0f);
        colors[ImGuiCol_TitleBgCollapsed]   = HEXAtoIV4("#1b1b1f", 1.0f);

        colors[ImGuiCol_ResizeGrip]         = HEXAtoIV4("#2f3136", 1.0f);
        colors[ImGuiCol_ResizeGripHovered]  = HEXAtoIV4("#393c43", 1.0f);
        colors[ImGuiCol_ResizeGripActive]   = HEXAtoIV4("#41454d", 1.0f);

        colors[ImGuiCol_Separator]          = HEXAtoIV4("#2c2f33", 1.0f);
        colors[ImGuiCol_SeparatorHovered]   = HEXAtoIV4("#393c43", 1.0f);
        colors[ImGuiCol_SeparatorActive]    = HEXAtoIV4("#41454d", 1.0f);

        colors[ImGuiCol_Text]               = HEXAtoIV4("#eeeeee", 1.0f);
        colors[ImGuiCol_TextDisabled]       = HEXAtoIV4("#6c757d", 1.0f);

        style.WindowPadding = ImVec2(10, 10);
        style.WindowRounding = 0.f;
        style.TabRounding = 0.f;
        style.GrabRounding = 6.0f;
        style.ItemSpacing = ImVec2(10, 5);
        style.FramePadding.y = 2.0f;
        style.GrabMinSize = 10.0f;
        style.FrameRounding = 1.0f;
        style.WindowMenuButtonPosition = ImGuiDir_None;
    }

    void ThemeManager::ApplyDarkTheme() {
        // TODO : implement dark theme
    }

    void ThemeManager::ApplyLightTheme() {
        // TODO : implement light theme
    }

    ImVec4 ThemeManager::HEXAtoIV4(const char* hex, float alpha) {
        int r, g, b;

        if (hex[0] == '#') hex++;

        #ifdef AQUILA_PLATFORM_WINDOWS
            sscanf_s(hex, "%02x%02x%02x", &r, &g, &b);
        #elif defined(AQUILA_PLATFORM_LINUX)
            sscanf(hex, "%02x%02x%02x", &r, &g, &b);
        #endif


        return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, alpha);
    }
} // namespace Editor
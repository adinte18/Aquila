#include "UI/FontManager.h"
#include "UI/UIConfig.h"

namespace Editor::UI {
    FontManager& FontManager::Get() {
        static FontManager instance;
        return instance;
    }
    
    void FontManager::LoadFonts() {
        ImGuiIO& io = ImGui::GetIO();
        
        LoadFont(io, 12.0f, m_Fonts.Font12);
        LoadFont(io, 14.0f, m_Fonts.Font14);
        LoadFont(io, 16.0f, m_Fonts.Font16);
        LoadFont(io, 18.0f, m_Fonts.Font18);
        LoadFont(io, 20.0f, m_Fonts.Font20);
        LoadFont(io, 24.0f, m_Fonts.Font24);
        LoadFont(io, 28.0f, m_Fonts.Font28);
        LoadFont(io, 32.0f, m_Fonts.Font32);
        LoadFont(io, 40.0f, m_Fonts.Font40);
        LoadFont(io, 48.0f, m_Fonts.Font48);
        LoadFont(io, 64.0f, m_Fonts.Font64);
        LoadFont(io, 72.0f, m_Fonts.Font72);
        
        m_CurrentFont = m_Fonts.Font16;
    }
    
    void FontManager::LoadFont(ImGuiIO& io, float size, ImFont*& outFont) {
        ImFontConfig config;
        config.OversampleH = 3;
        config.OversampleV = 1;
        config.PixelSnapH = true;

        outFont = io.Fonts->AddFontFromFileTTF(Config::MainFontPath, size, &config);
        
        static const ImWchar icon_ranges[] = { ICON_MIN_LC, ICON_MAX_16_LC, 0 };
        ImFontConfig icon_config;
        icon_config.MergeMode = true;
        icon_config.PixelSnapH = true;
        icon_config.GlyphMinAdvanceX = size;
        icon_config.GlyphOffset.y = size * 0.25f;

        io.Fonts->AddFontFromFileTTF(Config::IconsFontPath.c_str(), size, &icon_config, icon_ranges);
    }
}
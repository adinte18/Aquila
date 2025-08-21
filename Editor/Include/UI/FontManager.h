// UI/UIFonts.h
#ifndef UI_FONTS_H
#define UI_FONTS_H

#include "imgui.h"

namespace Editor::UI {
    struct Fonts {
        ImFont* Font12 = nullptr;
        ImFont* Font14 = nullptr;
        ImFont* Font16 = nullptr;
        ImFont* Font18 = nullptr;
        ImFont* Font20 = nullptr;
        ImFont* Font24 = nullptr;
        ImFont* Font28 = nullptr;
        ImFont* Font32 = nullptr;
        ImFont* Font40 = nullptr;
        ImFont* Font48 = nullptr;
        ImFont* Font64 = nullptr;
        ImFont* Font72 = nullptr;
    };

    class FontManager {
    public:
        static FontManager& Get();
        
        void LoadFonts();
        void LoadFont(ImGuiIO& io, float size, ImFont*& outFont);
        
        Fonts& GetFonts() { return m_Fonts; }
        ImFont* GetCurrentFont() const { return m_CurrentFont; }
        void SetCurrentFont(ImFont* font) { m_CurrentFont = font; }

    private:
        FontManager() = default;
        Fonts m_Fonts{};
        ImFont* m_CurrentFont = nullptr;
    };
}

#endif // UI_FONTS_H

#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include "UI/UIConfig.h"

namespace Editor::UI {
    class ThemeManager {
    public:
        static ThemeManager& Get();

        void ApplyAquilaTheme();
        void ApplyDarkTheme();
        void ApplyLightTheme();
        
    private:
        ImVec4 HEXAtoIV4(const char* hex, float alpha);
    };
}

#endif // THEME_MANAGER_H
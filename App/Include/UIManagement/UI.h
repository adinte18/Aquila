//
// Created by alexa on 19/10/2024.
//

#ifndef UI_H
#define UI_H

#include <Engine/Device.h>

#include "Elements/ContentBrowserElement.h"
#include "Elements/HierarchyElement.h"
#include "Elements/ViewportElement.h"
#include "Elements/MenubarElement.h"
#include "Elements/PropertiesElement.h"

#include "Scene/Scene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo/ImGuizmo.h"

#include <nativefiledialog/src/nfd_common.h>
#include <lucide.h>



namespace Editor {

    struct UIConfig {
        static inline ImGuiDockNodeFlags DockspaceFlags = ImGuiDockNodeFlags_None;

        static inline ImGuiWindowFlags WindowFlags = 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        static inline ImGuiConfigFlags ConfigFlags = 
            ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad 
            | ImGuiConfigFlags_DockingEnable
            | ImGuiConfigFlags_NoMouseCursorChange;

        static inline float WindowRounding = 0.0f;
        static inline float WindowBorderSize = 0.0f;

        static inline const char* DockspaceName = "Aquila Dockspace";
        static inline ImGuiID DockspaceID = 0;

        static inline float BaseFontSize = 16.0f;

        #if defined(AQUILA_PLATFORM_WINDOWS)
            static inline const char* MainFontPath = R"(C:/Windows/Fonts/segoeui.ttf)";
            static inline const char* IconsFontPath = R"(C:\Programming\Aquila\App\Include\lucide.ttf)";
        #elif defined(AQUILA_PLATFORM_LINUX)
            static inline const char* MainFontPath = R"(/home/adinte/Desktop/Aquila/Aquila/Resources/Engine Ressources/Roboto-VariableFont_wdth,wght.ttf)";
            static inline const char* IconsFontPath = R"(/home/adinte/Desktop/Aquila/Aquila/App/Include/lucide.ttf)";
        #endif

        static inline constexpr ImWchar IconsRanges[3] = { ICON_MIN_LC, ICON_MAX_16_LC, 0 };
    };

    class UIManager {
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


        Engine::Device& m_Device;
        Engine::Window& m_Window;

        VkRenderPass m_RenderPass;
        VkDescriptorPool m_UiDescriptorPool{};
        static VkDescriptorSet m_FinalImage;

        //UI elements
        UIManagement::MenubarElement m_Menubar{}; 
        UIManagement::ContentElement m_ContentBrowser{};
        UIManagement::HierarchyElement m_Hierarchy{};
        UIManagement::ViewportElement m_Viewport{};
        UIManagement::PropertiesElement m_Properties{};

        
        void InitializeImGui() const;

        void LoadFont(ImGuiIO& io, Fonts& fonts, float size, ImFont*& outFont) const;
        void LoadFonts() const;
        void SetAquilaTheme() const;

        ImVec4 HEXAtoIV4(const char* hex, float alpha = 1.0f) const;

    public:
        static Fonts s_Fonts;
        static ImFont* s_CurrentFont;
        static entt::entity s_SelectedEntity;

        UIManager(Engine::Device& device, Engine::Window& window, VkRenderPass renderPass)
            : m_Device(device), m_Window(window), m_RenderPass(renderPass) {
            OnStart();
        }

        ~UIManager() {
            OnEnd();
        }

        void SetupDockspace();

        void GetFinalImage(VkDescriptorSet finalImage);

        static VkDescriptorSet FetchRenderedImage() { return m_FinalImage; }

        void OnStart();
        void OnUpdate(VkCommandBuffer commandBuffer, float deltaTime, Engine::AquilaScene* scene);
        void OnEnd() const;
    };
}


#endif //UI_H

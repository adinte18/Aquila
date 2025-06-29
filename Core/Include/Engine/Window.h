//
// Created by adinte on 7/5/24.
//

#ifndef VK_APP_Window_H
#define VK_APP_Window_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace Engine{
    class Window {
    private:
        static void FramebufferResizedCallback(GLFWwindow *window, int width, int height);

        int m_Width;
        int m_Height;
        bool m_FramebufferResized = false;

        std::string m_WindowName;
        GLFWwindow *m_Window;

    public:
        Window(int width, int height, const std::string& title);
        ~Window();

        [[nodiscard]] bool IsWindowResized() const { return m_FramebufferResized; }
        [[nodiscard]] bool ShouldClose() const;

        void ResetResizedFlag() { m_FramebufferResized = false; }
        void CreateWindowSurface(VkInstance instance, VkSurfaceKHR *surface) const;

        [[nodiscard]] VkExtent2D GetExtent() const {
            return {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)};
        }

        void SetInputMode(int inputMode) const;
        void GetCursorPosition(double& xpos, double& ypos) const;
        void SetCursorPosition(double xpos, double ypos) const;

        [[nodiscard]] GLFWwindow *GetWindow() const {return m_Window;}

        void PollEvents();

        void CleanUp();
    };
}

#endif //VK_APP_Window_H

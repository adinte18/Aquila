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
        static void vk_FBResizedCallback(GLFWwindow *window, int width, int height);

        int width;
        int height;
        bool framebufferResized = false;

        std::string windowName;
        GLFWwindow *window;

    public:
        Window(int width, int height, std::string title);

        ~Window();

        [[nodiscard]] bool WindowResized() const { return framebufferResized; }
        void vk_ResetResizedFlag() { framebufferResized = false; }

        [[nodiscard]] bool ShouldClose() const;

        void vk_CreateWindowSurface(VkInstance instance, VkSurfaceKHR *surface) const;
        [[nodiscard]] VkExtent2D getExtent() const {
            return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        }

        [[nodiscard]] GLFWwindow *glfw_GetWindow() const {return window;}

        void PollEvents();

        void CleanUp();
    };
}

#endif //VK_APP_Window_H

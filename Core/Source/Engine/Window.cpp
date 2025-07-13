#include "Engine/Window.h"

#include <stdexcept>

Engine::Window::Window(const int width, const int height, const std::string& title)
: m_Width{width}, m_Height{height}, m_WindowName{title} {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    m_Window = glfwCreateWindow(mode->width, mode->height, "Aquila", nullptr, nullptr);

    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, FramebufferResizedCallback);
    glfwMaximizeWindow(m_Window);

    if (m_Window == nullptr) {
        throw std::runtime_error("Failed to create GLFW window");
    }
}

void Engine::Window::SetInputMode(int inputMode) const {
    if (m_Window) {
        glfwSetInputMode(m_Window, GLFW_CURSOR, inputMode);
    }
}

void Engine::Window::GetCursorPosition(double& xpos, double& ypos) const {
    glfwGetCursorPos(m_Window, &xpos, &ypos);
}

void Engine::Window::SetCursorPosition(double xpos, double ypos) const {
    glfwSetCursorPos(m_Window, xpos, ypos);
}

Engine::Window::~Window() {
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Engine::Window::CreateWindowSurface(VkInstance instance, VkSurfaceKHR *surface) const {
    if (glfwCreateWindowSurface(instance, m_Window, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void Engine::Window::PollEvents() {
    glfwPollEvents();
}

void Engine::Window::CleanUp() {

}

bool Engine::Window::ShouldClose() const {
    return glfwWindowShouldClose(m_Window);
}

void Engine::Window::FramebufferResizedCallback(GLFWwindow *window, int width, int height) {
    const auto vkWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    vkWindow->m_FramebufferResized = true;
    vkWindow->m_Width = width;
    vkWindow->m_Height = height;
}


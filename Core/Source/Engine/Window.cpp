#include "Engine/Window.h"
#include <stdexcept>
#include <utility>

Engine::Window::Window(const int width, const int height, std::string title)
: width{width}, height{height}, windowName{std::move(title)} {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    window = glfwCreateWindow(mode->width, mode->height, "Aquila", nullptr, nullptr);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, vk_FBResizedCallback);
    glfwMaximizeWindow(window);

    if (window == nullptr) {
        throw std::runtime_error("Failed to create GLFW window");
    }
}

Engine::Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Engine::Window::vk_CreateWindowSurface(VkInstance instance, VkSurfaceKHR *surface) const {
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void Engine::Window::PollEvents() {
    glfwPollEvents();
}

void Engine::Window::CleanUp() {

}

bool Engine::Window::ShouldClose() const {
    return glfwWindowShouldClose(window);
}

void Engine::Window::vk_FBResizedCallback(GLFWwindow *window, int width, int height) {
    auto vkWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    vkWindow->framebufferResized = true;
    vkWindow->width = width;
    vkWindow->height = height;
}


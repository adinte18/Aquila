cmake_minimum_required(VERSION 3.10)
project(App)

# Set the path for external libraries
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/Core/Include/External)

# Add ImGui sources
set(IMGUI_SOURCES
        ${EXTERNAL_DIR}/imgui/imgui.cpp
        ${EXTERNAL_DIR}/imgui/imgui_draw.cpp
        ${EXTERNAL_DIR}/imgui/imgui_widgets.cpp
        ${EXTERNAL_DIR}/imgui/imgui_tables.cpp
        ${EXTERNAL_DIR}/imgui/imgui_demo.cpp

        # Include only the necessary backends
        ${EXTERNAL_DIR}/imgui/backends/imgui_impl_glfw.cpp
        ${EXTERNAL_DIR}/imgui/backends/imgui_impl_vulkan.cpp

        # Include ImGuizmo
        ${EXTERNAL_DIR}/ImGuizmo/ImGuizmo.cpp
)

# Add Native File Dialog source
set(NFD_SOURCES
        ${EXTERNAL_DIR}/nativefiledialog/src/nfd_common.c
        ${EXTERNAL_DIR}/nativefiledialog/src/nfd_win.cpp  # If you're on Windows
        # For other platforms (Linux/macOS)
        # ${EXTERNAL_DIR}/nativefiledialog/src/nfd_gtk.c  # For Linux
        # ${EXTERNAL_DIR}/nativefiledialog/src/nfd_cocoa.m # For macOS
)

# Recursively gather all source files for the application
file(GLOB_RECURSE APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
)

# Define the executable target for the application
add_executable(App ${APP_SOURCES} ${IMGUI_SOURCES} ${NFD_SOURCES})

# Set include directories for the application
target_include_directories(App PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/                                # Path to App headers
        ${EXTERNAL_DIR}/                                                    # Path to external libraries
        ${EXTERNAL_DIR}/imgui/                                              # Path to ImGui
        ${EXTERNAL_DIR}/imgui/backends/                                     # Path to ImGui backends
        ${EXTERNAL_DIR}/nativefiledialog/src/include                        # Path to Native File Dialog
        ${EXTERNAL_DIR}/ImGuizmo/                                           # Path to ImGuizmo
)

add_dependencies(App validate_shaders)

# Link against Core (assuming Core is another target in your project)
target_link_libraries(App PRIVATE Core)

# Link against necessary system libraries for NFD (if required)
if (WIN32)
    target_link_libraries(App PRIVATE ole32)  # NFD needs ole32 on Windows
endif()
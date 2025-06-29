cmake_minimum_required(VERSION 3.10)
project(App)

set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/Core/Vendor)

set(IMGUI_SOURCES
    ${VENDOR_DIR}/imgui/imgui.cpp
    ${VENDOR_DIR}/imgui/imgui_draw.cpp
    ${VENDOR_DIR}/imgui/imgui_widgets.cpp
    ${VENDOR_DIR}/imgui/imgui_tables.cpp
    ${VENDOR_DIR}/imgui/imgui_demo.cpp
    ${VENDOR_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${VENDOR_DIR}/imgui/backends/imgui_impl_vulkan.cpp
    ${VENDOR_DIR}/ImGuizmo/ImGuizmo.cpp
)

set(NFD_SOURCES
    ${VENDOR_DIR}/nativefiledialog/src/nfd_common.c
    ${VENDOR_DIR}/nativefiledialog/src/nfd_win.cpp
    Include/lucide.h
    Include/UIManagement/
    # Uncomment for other platforms:
    # ${VENDOR_DIR}/nativefiledialog/src/nfd_gtk.c
    # ${VENDOR_DIR}/nativefiledialog/src/nfd_cocoa.m
)

file(GLOB_RECURSE APP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
)

add_executable(App
    ${APP_SOURCES}
    ${IMGUI_SOURCES}
    ${NFD_SOURCES}
    ${CMAKE_SOURCE_DIR}/app.rc
)

target_include_directories(App PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/UIManagement
    ${VENDOR_DIR}
    ${VENDOR_DIR}/imgui
    ${VENDOR_DIR}/imgui/backends
    ${VENDOR_DIR}/nativefiledialog/src/include
    ${VENDOR_DIR}/ImGuizmo
)

add_dependencies(App validate_shaders)

set(ICONS_PATH "${CMAKE_SOURCE_DIR}/Resources/Icons")
add_compile_definitions(ICONS_PATH="${ICONS_PATH}")

set(ASSET_PATH "${CMAKE_SOURCE_DIR}/Assets")
add_compile_definitions(ASSET_PATH="${ASSET_PATH}")

target_link_libraries(App PRIVATE Core)

if (WIN32)
    target_link_libraries(App PRIVATE ole32)
endif()

cmake_minimum_required(VERSION 3.10)
project(Editor)

set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/Core/Vendor)

# ==== IMGUI ====
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
# ========================


# ==== NFD ====
set(NFD_SOURCES
    ${VENDOR_DIR}/nativefiledialog/src/nfd_common.c
)

if(WIN32)
    list(APPEND NFD_SOURCES ${VENDOR_DIR}/nativefiledialog/src/nfd_win.cpp)
elseif(APPLE)
    list(APPEND NFD_SOURCES ${VENDOR_DIR}/nativefiledialog/src/nfd_cocoa.m)
elseif(UNIX)
    list(APPEND NFD_SOURCES ${VENDOR_DIR}/nativefiledialog/src/nfd_gtk.c)
endif()
# ========================


# ==== EDITOR SOURCES ====
file(GLOB_RECURSE EDITOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
)
# ========================


# ==== LIBRARY ====
add_library(Editor STATIC
    ${EDITOR_SOURCES}
    ${IMGUI_SOURCES}
    ${NFD_SOURCES}
)
# ========================


# ==== INCLUDE DIRECTORIES ====
target_include_directories(Editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Include/UIManagement
    ${VENDOR_DIR}
    ${VENDOR_DIR}/imgui
    ${VENDOR_DIR}/imgui/backends
    ${VENDOR_DIR}/nativefiledialog/src/include
    ${VENDOR_DIR}/ImGuizmo
)

target_include_directories(Editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)
# ========================


# ==== DEFINITIONS ====
set(ICONS_PATH "${CMAKE_SOURCE_DIR}/Resources/Icons")
target_compile_definitions(Editor PUBLIC ICONS_PATH="${ICONS_PATH}")

set(ASSET_PATH "${CMAKE_SOURCE_DIR}/Assets")
target_compile_definitions(Editor PUBLIC ASSET_PATH="${ASSET_PATH}")

set(ENGINE_RESSOURCES_PATH "${CMAKE_SOURCE_DIR}/Resources/Engine Ressources")
target_compile_definitions(Editor PUBLIC ENGINE_RESSOURCES_PATH="${ENGINE_RESSOURCES_PATH}")
# ========================


# ==== LINK LIBRARIES ====
target_link_libraries(Editor PUBLIC Core)

if (WIN32)
    target_link_libraries(Editor PRIVATE ole32)
elseif(APPLE)
    target_link_libraries(Editor PRIVATE "-framework Cocoa")
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    target_include_directories(Editor PRIVATE ${GTK3_INCLUDE_DIRS})
    target_link_libraries(Editor PRIVATE ${GTK3_LIBRARIES})
endif()
# ========================

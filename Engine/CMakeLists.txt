cmake_minimum_required(VERSION 3.16)
project(AquilaCore)

set(CORE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(VENDOR_DIR ${CORE_ROOT_DIR}/Vendor)
set(CORE_INCLUDE_DIR ${CORE_ROOT_DIR}/Include)
set(CORE_SOURCE_DIR ${CORE_ROOT_DIR}/Source)

find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found.")
endif ()
message(STATUS "Vulkan SDK found at: $ENV{VULKAN_SDK}")
message(STATUS "Vulkan includes: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan libraries: ${VULKAN_LIBRARY}")

set(SLANG_INCLUDE_DIR "$ENV{VULKAN_SDK}/Include")
set(SLANG_LIB_DIR "$ENV{VULKAN_SDK}/Lib")

find_library(SLANG_LIBRARY
    NAMES slang
    PATHS ${SLANG_LIB_DIR}
    REQUIRED
)

message(STATUS "Slang library: ${SLANG_LIBRARY}")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_INCLUDE_DIRS ${VENDOR_DIR}/glfw/include)
add_subdirectory(${VENDOR_DIR}/glfw)

set(IMGUI_DIR ${VENDOR_DIR}/imgui)
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

set(IMGUIZMO_DIR ${VENDOR_DIR}/ImGuizmo)
set(IMGUIZMO_SOURCES ${IMGUIZMO_DIR}/ImGuizmo.cpp)

set_source_files_properties(${IMGUIZMO_SOURCES} PROPERTIES
        COMPILE_DEFINITIONS "IMGUI_DEFINE_MATH_OPERATORS"
)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${VENDOR_DIR}/assimp)

set(ASSIMP_INCLUDE_DIRS
        ${VENDOR_DIR}/assimp/include
        ${CMAKE_BINARY_DIR}/Engine/Vendor/assimp/include
)
set(ASSIMP_LIBRARY_DIRS
        ${CMAKE_BINARY_DIR}/Engine/Vendor/assimp/code
)
include_directories(${ASSIMP_INCLUDE_DIRS})
link_directories(${ASSIMP_LIBRARY_DIRS})

set(GLM_INCLUDE_DIRS ${VENDOR_DIR}/glm)

set(STB_INCLUDE_DIR ${VENDOR_DIR}/stb)

set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SPIRV_HEADERS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(${VENDOR_DIR}/SPIRV-Headers)
set(SPIRV_HEADERS_INCLUDE_DIRS ${VENDOR_DIR}/SPIRV-Headers/include)

set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SPIRV-Headers_SOURCE_DIR ${VENDOR_DIR}/SPIRV-Headers)
add_subdirectory(${VENDOR_DIR}/SPIRV-Tools)
set(SPIRV_TOOLS_INCLUDE_DIRS ${VENDOR_DIR}/SPIRV-Tools/include)

file(GLOB_RECURSE ENGINE_SOURCES ${CORE_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE ENGINE_HEADERS ${CORE_INCLUDE_DIR}/*.h ${CORE_INCLUDE_DIR}/*.hpp)

add_library(Core STATIC
        ${ENGINE_SOURCES}
        ${ENGINE_HEADERS}
        ${IMGUI_SOURCES}
        ${IMGUIZMO_SOURCES}
)

set_target_properties(Core PROPERTIES
        OUTPUT_NAME "AquilaCore"
        DEBUG_POSTFIX "d"
)

set(PCH_HEADER "${CORE_INCLUDE_DIR}/AquilaPCH.h")
if (EXISTS ${PCH_HEADER})
    target_precompile_headers(Core PRIVATE ${PCH_HEADER})
    message(STATUS "Precompiled header enabled: ${PCH_HEADER}")
else ()
    message(WARNING "Precompiled header not found: ${PCH_HEADER}")
endif ()

#  Include Directories
target_include_directories(Core
        PUBLIC
        # Third-party libraries
        ${GLM_INCLUDE_DIRS}
        ${Vulkan_INCLUDE_DIRS}
        ${VENDOR_DIR}
        ${GLFW_INCLUDE_DIRS}
        ${ASSIMP_INCLUDE_DIRS}
        ${SPIRV_TOOLS_INCLUDE_DIRS}
        ${SPIRV_HEADERS_INCLUDE_DIRS}
        ${STB_INCLUDE_DIR}

        # ImGui PUBLIC so Editor inherits
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends

        # ImGuizmo PUBLIC so Editor inherits
        ${IMGUIZMO_DIR}

        ${CORE_INCLUDE_DIR}
        PRIVATE
        ${CORE_SOURCE_DIR}
)

#  Compile Definitions
set(SHADERS_PATH "${AQUILA_RESOURCES_PATH}/Shaders")
set(IMMUTABLE_SHADERS_PATH "${AQUILA_ENGINE_IMMUTABLE_SHADER_PATH}")
set(TEXTURES_PATH "${AQUILA_RESOURCES_PATH}/Textures")
set(ICONS_PATH "${AQUILA_RESOURCES_PATH}/Icons")
set(ENGINE_RESOURCES_PATH "${AQUILA_RESOURCES_PATH}/Engine")

target_compile_definitions(Core
        PUBLIC
        ASSET_PATH="${AQUILA_ASSET_PATH}"
        RESOURCES_PATH="${AQUILA_RESOURCES_PATH}"
        ICONS_PATH="${ICONS_PATH}"
        ENGINE_RESOURCES_PATH="${ENGINE_RESOURCES_PATH}"
        FONTS_PATH="${AQUILA_RESOURCES_PATH}/Fonts"
        PRIVATE
        SHADERS_PATH="${SHADERS_PATH}"
        IMMUTABLE_SHADERS_PATH="${IMMUTABLE_SHADERS_PATH}"
        TEXTURES_PATH="${TEXTURES_PATH}"
)

target_link_libraries(Core
        PUBLIC
        Vulkan::Vulkan
        ${SLANG_LIBRARY}
        glfw
        PRIVATE
        assimp
        SPIRV-Tools
)

if (MSVC)
    target_compile_options(Core PRIVATE
            /W4
            /bigobj
            /MP
            /permissive-
            $<$<CONFIG:Debug>:/Zi /Od>
            $<$<CONFIG:Release>:/O2>
            /wd4996
    )
    set_target_properties(Core PROPERTIES COMPILE_FLAGS "/Zm200")

elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Core PRIVATE
            -Wall -Wextra -Wpedantic
            -Wno-unused-parameter
            -Wno-missing-field-initializers
            -Wno-deprecated-literal-operator
            -fcolor-diagnostics
            -ftemplate-backtrace-limit=0
            $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer -fno-inline>
            $<$<CONFIG:Release>:-O3 -march=native -g1>
    )

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(Core PRIVATE
            -Wall -Wextra -Wpedantic
            -Wno-unused-parameter
            $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer>
            $<$<CONFIG:Release>:-O3 -g1>
    )
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(Core PRIVATE -g3 -fno-omit-frame-pointer)
    endif ()
    target_compile_definitions(Core PRIVATE _DEBUG)
endif ()

#  IDE Organization
source_group("ThirdParty/ImGui" FILES ${IMGUI_SOURCES})
source_group("ThirdParty/ImGuizmo" FILES ${IMGUIZMO_SOURCES})

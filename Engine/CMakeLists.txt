cmake_minimum_required(VERSION 3.16)
project(AquilaCore)

# ===== Paths =====
set(CORE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(VENDOR_DIR ${CORE_ROOT_DIR}/Vendor)
set(CORE_INCLUDE_DIR ${CORE_ROOT_DIR}/Include)
set(CORE_SOURCE_DIR ${CORE_ROOT_DIR}/Source)

# ===== Vulkan =====
find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please install Vulkan SDK from https://vulkan.lunarg.com/")
endif ()
message(STATUS "Vulkan SDK found at: $ENV{VULKAN_SDK}")
message(STATUS "Vulkan includes: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan libraries: ${Vulkan_LIBRARIES}")

# ===== GLFW =====
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_INCLUDE_DIRS ${VENDOR_DIR}/glfw/include)
add_subdirectory(${VENDOR_DIR}/glfw)

# ===== ImGui =====
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

# ===== ImGuizmo =====
set(IMGUIZMO_DIR ${VENDOR_DIR}/ImGuizmo)
set(IMGUIZMO_SOURCES ${IMGUIZMO_DIR}/ImGuizmo.cpp)

# ImGuizmo needs IMGUI_DEFINE_MATH_OPERATORS before imgui.h
set_source_files_properties(${IMGUIZMO_SOURCES} PROPERTIES
        COMPILE_DEFINITIONS "IMGUI_DEFINE_MATH_OPERATORS"
)

# ===== Assimp =====
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

# ===== GLM =====
set(GLM_INCLUDE_DIRS ${VENDOR_DIR}/glm)

# ===== SPIRV-Headers =====
set(SPIRV_HEADERS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SPIRV_HEADERS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(${VENDOR_DIR}/SPIRV-Headers)
set(SPIRV_HEADERS_INCLUDE_DIRS ${VENDOR_DIR}/SPIRV-Headers/include)

# ===== SPIRV-Tools =====
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SPIRV-Headers_SOURCE_DIR ${VENDOR_DIR}/SPIRV-Headers)
add_subdirectory(${VENDOR_DIR}/SPIRV-Tools)
set(SPIRV_TOOLS_INCLUDE_DIRS ${VENDOR_DIR}/SPIRV-Tools/include)

# ===== glslang =====
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)
add_subdirectory(${VENDOR_DIR}/glslang)
set(GLSLANG_INCLUDE_DIRS ${VENDOR_DIR}/glslang)

# ===== shaderc =====
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_INSTALL ON CACHE BOOL "" FORCE)
set(SHADERC_GLSLANG_DIR ${VENDOR_DIR}/glslang CACHE STRING "" FORCE)
set(SHADERC_SPIRV_TOOLS_DIR ${VENDOR_DIR}/SPIRV-Tools CACHE STRING "" FORCE)
add_subdirectory(${VENDOR_DIR}/shaderc)
set(SHADERC_INCLUDE_DIRS ${VENDOR_DIR}/shaderc/libshaderc/include)

# ===== Core Sources =====
file(GLOB_RECURSE ENGINE_SOURCES ${CORE_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE ENGINE_HEADERS ${CORE_INCLUDE_DIR}/*.h ${CORE_INCLUDE_DIR}/*.hpp)

# ===== Create Core Library =====
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

# ===== Precompiled Headers =====
set(PCH_HEADER "${CORE_INCLUDE_DIR}/AquilaPCH.h")
if (EXISTS ${PCH_HEADER})
    target_precompile_headers(Core PRIVATE ${PCH_HEADER})
    message(STATUS "Precompiled header enabled: ${PCH_HEADER}")
else ()
    message(WARNING "Precompiled header not found: ${PCH_HEADER}")
endif ()

# ===== Include Directories =====
target_include_directories(Core
        PUBLIC
        # Third-party libraries (no prefix needed for these)
        ${GLM_INCLUDE_DIRS}
        ${Vulkan_INCLUDE_DIRS}
        ${VENDOR_DIR}
        ${GLFW_INCLUDE_DIRS}
        ${ASSIMP_INCLUDE_DIRS}
        ${SHADERC_INCLUDE_DIRS}
        ${GLSLANG_INCLUDE_DIRS}
        ${SPIRV_TOOLS_INCLUDE_DIRS}
        ${SPIRV_HEADERS_INCLUDE_DIRS}

        # ImGui - PUBLIC so Editor inherits
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends

        # ImGuizmo - PUBLIC so Editor inherits
        ${IMGUIZMO_DIR}

        ${CORE_INCLUDE_DIR}
        PRIVATE
        ${CORE_SOURCE_DIR}
)

# ===== Compile Definitions =====
set(SHADERS_PATH "${AQUILA_RESOURCES_PATH}/Shaders")
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
        TEXTURES_PATH="${TEXTURES_PATH}"
)

# ===== Link Libraries =====
target_link_libraries(Core
        PUBLIC
        ${Vulkan_LIBRARIES}
        glfw
        PRIVATE
        assimp
        shaderc
        glslang
        SPIRV
        SPIRV-Tools
)

# ===== Compiler-Specific Options =====
if (MSVC)
    target_compile_options(Core PRIVATE
            /W4
            /bigobj
            /MP
            /permissive-
            # Full debug info for debugging
            $<$<CONFIG:Debug>:/Zi /Od>
            $<$<CONFIG:Release>:/O2>
            # Disable specific warnings from vendor code
            /wd4996  # Deprecated functions
    )
    set_target_properties(Core PROPERTIES COMPILE_FLAGS "/Zm200")

elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Core PRIVATE
            -Wall -Wextra -Wpedantic
            -Wno-unused-parameter
            -Wno-missing-field-initializers

            # SILENCE VENDOR WARNINGS
            -Wno-deprecated-literal-operator  # Fixes entt and nlohmann json warnings

            -fcolor-diagnostics
            -ftemplate-backtrace-limit=0

            # Debug: FULL debug info for debugger
            $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer -fno-inline>

            # Release: optimize but keep some debug info
            $<$<CONFIG:Release>:-O3 -march=native -g1>
    )

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(Core PRIVATE
            -Wall -Wextra -Wpedantic
            -Wno-unused-parameter

            # Debug: FULL debug info
            $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer>

            $<$<CONFIG:Release>:-O3 -g1>
    )
endif ()

# ===== Better Debug Info (All Configs) =====
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Force full debug symbols
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(Core PRIVATE -g3 -fno-omit-frame-pointer)
    endif ()

    # Disable optimizations completely in debug
    target_compile_definitions(Core PRIVATE _DEBUG)
endif ()

# ===== IDE Organization =====
source_group("ThirdParty/ImGui" FILES ${IMGUI_SOURCES})
source_group("ThirdParty/ImGuizmo" FILES ${IMGUIZMO_SOURCES})

# ===== Summary =====
list(LENGTH ENGINE_SOURCES SOURCE_COUNT)
list(LENGTH ENGINE_HEADERS HEADER_COUNT)
list(LENGTH IMGUI_SOURCES IMGUI_COUNT)
message(STATUS "")
message(STATUS "==================== Core Configuration ======================")
message(STATUS "  Library:          Core (AquilaCore)")
message(STATUS "  Type:             STATIC")
message(STATUS "  Source Files:     ${SOURCE_COUNT} files")
message(STATUS "  Header Files:     ${HEADER_COUNT} files")
message(STATUS "  Vulkan Version:   ${Vulkan_VERSION}")
message(STATUS "  Compiler:         ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Debug Symbols:    ${CMAKE_BUILD_TYPE}")
message(STATUS "==============================================================")
message(STATUS "")
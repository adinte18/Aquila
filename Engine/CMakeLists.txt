# Engine/CMakeLists.txt

set(ENGINE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(VENDOR_DIR ${ENGINE_ROOT_DIR}/Vendor)
set(ENGINE_INCLUDE_DIR ${ENGINE_ROOT_DIR}/Include)

set(VULKAN_SDK $ENV{VULKAN_SDK} CACHE PATH "Vulkan SDK path")

find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND OR NOT EXISTS ${VULKAN_SDK})
    message(FATAL_ERROR "Vulkan SDK not found.")
endif ()

message(STATUS "Vulkan SDK found at: ${VULKAN_SDK}")
message(STATUS "Vulkan includes: ${Vulkan_INCLUDE_DIRS}")

set(SLANG_INCLUDE_DIR "${VULKAN_SDK}/include")

find_library(SLANG_LIBRARY
    NAMES libslang slang
    PATHS "${VULKAN_SDK}/lib"
    # This is needed otherwise CMake may find the library for the S-Lang interpreted language.
    NO_DEFAULT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_SYSTEM_PATH
    REQUIRED
)
message(STATUS "Slang library: ${SLANG_LIBRARY}")

set(GLM_INCLUDE_DIRS ${VENDOR_DIR}/glm)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC OFF CACHE BOOL "" FORCE)
add_subdirectory(${VENDOR_DIR}/glfw)

set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_FBX_IMPORTER ON)
set(ASSIMP_BUILD_GLTF_IMPORTER ON)
set(ASSIMP_BUILD_OBJ_IMPORTER ON)
set(ASSIMP_BUILD_COLLADA_IMPORTER ON)
set(ASSIMP_BUILD_DOCS OFF)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)

if(UNIX)
    set(ASSIMP_BUILD_MINIZIP ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(${VENDOR_DIR}/assimp)

add_subdirectory(${VENDOR_DIR}/VulkanMemoryAllocator)

add_compile_definitions(AQUILA_SHADERS_DIR="${ENGINE_ROOT_DIR}/Shaders/")

add_subdirectory(Source/Aquila/Foundation)
add_subdirectory(Source/Aquila/Platform)
add_subdirectory(Source/Aquila/Application)
add_subdirectory(Source/Aquila/RHI)
add_subdirectory(Source/Aquila/GFX)
add_subdirectory(Source/Aquila/Graphics)
add_subdirectory(Source/Aquila/Scene)
add_subdirectory(Source/Aquila/Rendering)
add_subdirectory(Source/Aquila/UI)

enable_testing()
add_subdirectory(Tests)

add_executable(AquilaEngine
    ${ENGINE_ROOT_DIR}/main.cpp
)
target_precompile_headers(AquilaEngine REUSE_FROM Application)
target_link_libraries(AquilaEngine PRIVATE
    Application
    GFX
    Graphics
    Scene
    Rendering
    UI
    glfw
    Vulkan::Headers
)

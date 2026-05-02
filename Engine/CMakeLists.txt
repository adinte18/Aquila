# Engine/CMakeLists.txt

set(ENGINE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(VENDOR_DIR ${ENGINE_ROOT_DIR}/Vendor)
set(ENGINE_INCLUDE_DIR ${ENGINE_ROOT_DIR}/Include)

find_package(Vulkan REQUIRED)
message(STATUS "Vulkan SDK found at: $ENV{VULKAN_SDK}")
message(STATUS "Vulkan includes: ${Vulkan_INCLUDE_DIRS}")

find_library(SLANG_LIBRARY
    NAMES slang
    PATHS "$ENV{VULKAN_SDK}/Lib"
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
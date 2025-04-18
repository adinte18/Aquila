cmake_minimum_required(VERSION 3.10)
project(Core)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building in Debug mode with DEBUG")
    add_definitions(-DDEBUG)
else()
    message(STATUS "Building in Release mode")
endif()

find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please install Vulkan SDK.")
else()
    message(STATUS "Vulkan SDK found at: $ENV{VULKAN_SDK}")
    set (VULKAN_SDK $ENV{VULKAN_SDK})
    message(STATUS "Vulkan found: ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "Vulkan libraries: ${Vulkan_LIBRARIES}")
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/Core/Include/External/glfw)
add_subdirectory(${CMAKE_SOURCE_DIR}/Core/Include/External/assimp)
add_subdirectory(${CMAKE_SOURCE_DIR}/Core/Include/External/SPIRV-Headers)
add_subdirectory(${CMAKE_SOURCE_DIR}/Core/Include/External/SPIRV-Tools)
add_subdirectory(${CMAKE_SOURCE_DIR}/Core/Include/External/glslang)


set(GLM_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/Core/Include/External/glm)

# Add your source files
file(GLOB_RECURSE ENGINE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Engine/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ECS/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/RenderingSystems/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Renderpasses/*.cpp
)

file(GLOB_RECURSE ENGINE_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/Engine/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/ECS/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/RenderingSystems/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/Renderpasses/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h
)

# Create the Core library
add_library(Core STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

# Include directories for the project
target_include_directories(Core PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/ECS
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/ECS/Components
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/Engine
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/RenderingSystems
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/Renderpasses
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/External/glfw/include
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/External/assimp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/External/SPIRV-Headers/include
)

# Define paths for shaders and textures
set(SHADERS_PATH "${CMAKE_SOURCE_DIR}/Resources/Shaders")
add_compile_definitions(SHADERS_PATH="${SHADERS_PATH}")

set(TEXTURES_PATH "${CMAKE_SOURCE_DIR}/Resources/Textures")
add_compile_definitions(TEXTURES_PATH="${TEXTURES_PATH}")

# Link libraries for the project
target_link_libraries(Core PRIVATE ${Vulkan_LIBRARIES} glslang glslang::glslang-default-resource-limits SPIRV SPIRV-Headers glfw assimp)

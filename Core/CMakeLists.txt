cmake_minimum_required(VERSION 3.10)
project(Core)

find_package(Vulkan REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/Core/Include/External/glfw)

set(GLM_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/Core/Include/External/glm)

file(GLOB_RECURSE ENGINE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Engine/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/ECS/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/RenderingSystems/*.cpp
)

add_library(Core STATIC ${ENGINE_SOURCES}
        Include/ECS/Components/Components.h
        Source/Engine/Shader.cpp
        Include/Engine/Camera.h
        Source/Engine/Camera.cpp
        Include/Engine/Primitives.h
        Source/RenderingSystems/DepthRenderingSystem.cpp
        Include/RenderingSystems/GridRenderingSystem.h)

target_include_directories(Core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/                            # Path to Core headers
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/ECS/Components              # Path to ECS headers
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/ECS                         # Path to ECS headers
        ${Vulkan_INCLUDE_DIRS}                                          # Vulkan include directories
        ${GLM_INCLUDE_DIRS}                                             # GLM include directories
        ${CMAKE_SOURCE_DIR}/Core/Include                                # Path to include directory
        ${CMAKE_SOURCE_DIR}/Core/Include/External/glfw/include
)

set(SHADERS_PATH "${CMAKE_SOURCE_DIR}/Resources/Shaders")
add_compile_definitions(SHADERS_PATH="${SHADERS_PATH}")

set(TEXTURES_PATH "${CMAKE_SOURCE_DIR}/Resources/Textures")
add_compile_definitions(TEXTURES_PATH="${TEXTURES_PATH}")


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building in Debug mode with DEBUG")
    add_definitions(-DDEBUG)
else()
    message(STATUS "Building in Release mode")
endif()

target_link_libraries(Core PRIVATE ${Vulkan_LIBRARIES} glfw)

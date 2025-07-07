cmake_minimum_required(VERSION 3.10)
project(Core)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()

#=========================== PATH SETUP ===========================
set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/Core/Vendor)
set(CORE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Core/Include)
set(CORE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/Core/Source)
#==================================================================


#=========================== VULKAN ===============================
find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please install Vulkan SDK.")
else()
    message(STATUS "Vulkan SDK found at: $ENV{VULKAN_SDK}")
    set(VULKAN_SDK $ENV{VULKAN_SDK})
    message(STATUS "Vulkan includes: ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "Vulkan libs: ${Vulkan_LIBRARIES}")
endif()
#==================================================================


#=========================== GLFW =================================
set(GLFW_INCLUDE_DIRS ${VENDOR_DIR}/glfw/include)
add_subdirectory(${VENDOR_DIR}/glfw)
#==================================================================


#=========================== ASSIMP ===============================
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
add_subdirectory(${VENDOR_DIR}/assimp)

set(ASSIMP_INCLUDE_DIRS
    ${VENDOR_DIR}/assimp/include
    ${CMAKE_BINARY_DIR}/Core/External/assimp/include
)
set(ASSIMP_LIBRARY_DIRS
    ${CMAKE_BINARY_DIR}/Core/External/assimp/code
)
include_directories(${ASSIMP_INCLUDE_DIRS})
link_directories(${ASSIMP_LIBRARY_DIRS})
#==================================================================


#=========================== GLM ==================================
set(GLM_INCLUDE_DIRS ${VENDOR_DIR}/glm)
#==================================================================


#=========================== CORE =================================
file(GLOB_RECURSE ENGINE_SOURCES
    ${CORE_SOURCE_DIR}/*.cpp
)

file(GLOB_RECURSE ENGINE_HEADERS
    ${CORE_INCLUDE_DIR}/*.h
)

add_library(Core STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(Core
    PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        ${CORE_INCLUDE_DIR}
        ${GLM_INCLUDE_DIRS}
        ${ASSIMP_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${VENDOR_DIR}
)

set(SHADERS_PATH "${CMAKE_SOURCE_DIR}/Resources/Shaders")
set(TEXTURES_PATH "${CMAKE_SOURCE_DIR}/Resources/Textures")
set(ASSET_PATH "${CMAKE_SOURCE_DIR}/Assets")

target_compile_definitions(Core PRIVATE
    SHADERS_PATH="${SHADERS_PATH}"
    TEXTURES_PATH="${TEXTURES_PATH}"
    ASSET_PATH="${ASSET_PATH}"
)

target_link_libraries(Core PRIVATE
    ${Vulkan_LIBRARIES}
    glfw
    assimp
)
#==================================================================
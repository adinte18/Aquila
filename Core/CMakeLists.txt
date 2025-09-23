cmake_minimum_required(VERSION 3.16)  # Increased for PCH support
project(Core)

# C++20 required for modern features and std::format
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    add_definitions(-DAQUILA_DEBUG)  # Your engine's debug flag
endif()

# Platform detection for your engine
if(WIN32)
    add_definitions(-DAQUILA_PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DAQUILA_PLATFORM_LINUX)
elseif(APPLE)
    add_definitions(-DAQUILA_PLATFORM_MACOS)
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
add_subdirectory(${VENDOR_DIR}/assimp)

set(ASSIMP_INCLUDE_DIRS
    ${VENDOR_DIR}/assimp/include
    ${CMAKE_BINARY_DIR}/Core/Vendor/assimp/include
)
set(ASSIMP_LIBRARY_DIRS
    ${CMAKE_BINARY_DIR}/Core/Vendor/assimp/code
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

set(PCH_HEADER "${CORE_INCLUDE_DIR}/AquilaPCH.h")
set(PCH_SOURCE "${CORE_SOURCE_DIR}/AquilaPCH.cpp")
add_library(Core STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})
#==================================================================

target_include_directories(Core
    PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        ${CORE_INCLUDE_DIR}
        ${GLM_INCLUDE_DIRS}
        ${ASSIMP_INCLUDE_DIRS}
        ${GLFW_INCLUDE_DIRS}
        ${VENDOR_DIR}
)

# This automatically injects AquilaPCH.h into EVERY .cpp file
# No manual #include needed in source files!
target_precompile_headers(Core PRIVATE ${PCH_HEADER})

message(STATUS "Precompiled header enabled: ${PCH_HEADER}")
message(STATUS "PCH will be automatically included in all source files")

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

# =============  COMPILER-SPECIFIC OPTIMIZATIONS =============
if(MSVC)
    target_compile_options(Core PRIVATE 
        /W4          # Warning level 4
        /bigobj      # Handle large object files (needed for PCH)
        /MP          # Multi-processor compilation
        $<$<CONFIG:Release>:/O2>  # Optimize for speed in release
    )
    
    # MSVC-specific PCH optimizations
    set_target_properties(Core PROPERTIES
        COMPILE_FLAGS "/Zm200"  # Increase compiler memory limit
    )
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(Core PRIVATE 
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-g>
    )
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(Core PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-g>
    )
endif()

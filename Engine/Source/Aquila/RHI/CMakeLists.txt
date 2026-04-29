set(MODULE_NAME RHI)
set(MODULE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Engine/Include)
set(MODULE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE MODULE_SOURCES ${MODULE_SOURCE_DIR}/*.cpp)

add_library(${MODULE_NAME} STATIC ${MODULE_SOURCES})

set_target_properties(${MODULE_NAME} PROPERTIES
    OUTPUT_NAME "AquilaRHI"
    DEBUG_POSTFIX "_debug"
)

target_compile_features(${MODULE_NAME} PUBLIC cxx_std_20)

target_include_directories(${MODULE_NAME}
    PUBLIC
    ${MODULE_INCLUDE_DIR}
    PRIVATE
    ${MODULE_SOURCE_DIR}
)

target_precompile_headers(${MODULE_NAME} PRIVATE
    ${MODULE_INCLUDE_DIR}/VulkanPCH.h
)

target_link_libraries(${MODULE_NAME}
    PUBLIC
    Foundation
    Platform
    Vulkan::Vulkan
    glfw
    GPUOpen::VulkanMemoryAllocator
    ${SLANG_LIBRARY}
)

# Treat VMA headers as system headers so Clang suppresses their warnings
# (VMA uses _Nullable/_Nonnull which are Clang extensions not valid on non-Apple targets).
get_target_property(_VMA_INCLUDES GPUOpen::VulkanMemoryAllocator INTERFACE_INCLUDE_DIRECTORIES)
if(_VMA_INCLUDES)
    target_include_directories(${MODULE_NAME} SYSTEM PUBLIC ${_VMA_INCLUDES})
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${MODULE_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-nullability-extension   # VMA uses _Nullable/_Nonnull (Apple Clang extension)
        -fcolor-diagnostics
        $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer -fno-inline>
        $<$<CONFIG:Release>:-O3>
    )
endif()

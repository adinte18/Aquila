set(MODULE_NAME Rendering)
set(MODULE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Engine/Include)
set(MODULE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Explicit list — old broken systems in Systems/ and Environment/ are excluded intentionally.
set(MODULE_SOURCES
    ${MODULE_SOURCE_DIR}/Camera.cpp
    ${MODULE_SOURCE_DIR}/RenderPipeline.cpp
    ${MODULE_SOURCE_DIR}/Systems/GeometrySystem.cpp
    ${MODULE_SOURCE_DIR}/Systems/DepthPrepassSystem.cpp
    ${MODULE_SOURCE_DIR}/Renderers/Renderer.cpp
    ${MODULE_SOURCE_DIR}/Renderers/Renderer2D.cpp
)

add_library(${MODULE_NAME} STATIC ${MODULE_SOURCES})

set_target_properties(${MODULE_NAME} PROPERTIES
    OUTPUT_NAME "AquilaRendering"
    DEBUG_POSTFIX "_debug"
)

target_compile_features(${MODULE_NAME} PUBLIC cxx_std_20)

target_include_directories(${MODULE_NAME}
    PUBLIC
    ${MODULE_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/Engine/Vendor
    PRIVATE
    ${MODULE_SOURCE_DIR}
)

target_precompile_headers(${MODULE_NAME} REUSE_FROM RHI)

target_link_libraries(${MODULE_NAME}
    PUBLIC
    Foundation
    RHI
    GFX
    Graphics
    Scene
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${MODULE_NAME} PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -fcolor-diagnostics
        $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer -fno-inline>
        $<$<CONFIG:Release>:-O3>
    )
endif()

set(MODULE_NAME Rendering)
set(MODULE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/Engine/Include)
set(MODULE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
    ${MODULE_SOURCE_DIR}/*.cpp
)
# Legacy system built against the old renderer architecture; not part of the current build.
list(FILTER MODULE_SOURCES EXCLUDE REGEX ".*SkyboxRenderingSystem\\.cpp$")

file(GLOB_RECURSE MODULE_HEADERS CONFIGURE_DEPENDS
    ${MODULE_INCLUDE_DIR}/Aquila/Rendering/*.h
)

add_library(${MODULE_NAME} STATIC ${MODULE_SOURCES} ${MODULE_HEADERS})

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
    PRIVATE
    hosek
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

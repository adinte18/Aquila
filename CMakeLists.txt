cmake_minimum_required(VERSION 3.10)
project(Aquila)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ===== Platform Detection =====
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")

if(WIN32)
    set(PLATFORM_DEFINE -DAQUILA_PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_DEFINE -DAQUILA_PLATFORM_LINUX)
elseif(APPLE)
    set(PLATFORM_DEFINE -DAQUILA_PLATFORM_MACOS)
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_DEFINE ${PLATFORM_DEFINE} -DAQUILA_PLATFORM_64BIT)
else()
    set(PLATFORM_DEFINE ${PLATFORM_DEFINE} -DAQUILA_PLATFORM_32BIT)
endif()

add_compile_definitions(${PLATFORM_DEFINE})

# =============================
add_subdirectory(Core)
add_subdirectory(Editor)
add_subdirectory(App)

file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/Resources/Shaders/*.frag" "${CMAKE_SOURCE_DIR}/Resources/Shaders/*.vert")
set(VALIDATE_COMMANDS)

set(SHADERS_PATH "${CMAKE_SOURCE_DIR}/Resources/Shaders")
set(TEXTURES_PATH "${CMAKE_SOURCE_DIR}/Resources/Textures")
set(ICONS_PATH "${CMAKE_SOURCE_DIR}/Resources/Icons")
set(ASSET_PATH "${CMAKE_SOURCE_DIR}/Assets")

target_compile_definitions(App PRIVATE 
    SHADERS_PATH="${SHADERS_PATH}" 
    TEXTURES_PATH="${TEXTURES_PATH}"
    ICONS_PATH="${ICONS_PATH}"
    ASSET_PATH="${ASSET_PATH}"
)


foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME_WE ${SHADER_FILE} NAME_WE)

    set(OUTPUT_FILE "${CMAKE_SOURCE_DIR}/Resources/Shaders/${SHADER_NAME_WE}.spv")

    message("Validating shader: ${SHADER_FILE} -> ${OUTPUT_FILE}")

    add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND glslangValidator -V ${SHADER_FILE} -o ${OUTPUT_FILE}
            DEPENDS ${SHADER_FILE}
            COMMENT "${SHADER_FILE} -> ${SHADER_NAME_WE}.spv"
    )

    list(APPEND VALIDATE_COMMANDS ${OUTPUT_FILE})
endforeach()

add_custom_target(validate_shaders ALL DEPENDS ${VALIDATE_COMMANDS})

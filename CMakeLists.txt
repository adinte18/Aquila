cmake_minimum_required(VERSION 3.10)
project(Aquila)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(Core)
add_subdirectory(App)

file(GLOB SHADER_FILES "${CMAKE_SOURCE_DIR}/Resources/Shaders/*.frag" "${CMAKE_SOURCE_DIR}/Resources/Shaders/*.vert")

set(VALIDATE_COMMANDS)

foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME_WE ${SHADER_FILE} NAME_WE)

    set(OUTPUT_FILE ${CMAKE_SOURCE_DIR}/shaders/${SHADER_NAME_WE}.spv)

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

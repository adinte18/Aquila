set(EDITOR_ROOT_DIR    ${CMAKE_CURRENT_SOURCE_DIR})
set(EDITOR_INCLUDE_DIR ${EDITOR_ROOT_DIR}/Include)
set(EDITOR_SOURCE_DIR  ${EDITOR_ROOT_DIR}/Source)

set(EDITOR_SOURCES
    ${EDITOR_SOURCE_DIR}/main.cpp
    ${EDITOR_SOURCE_DIR}/Core/EditorApplication.cpp
)

if (WIN32)
    list(APPEND EDITOR_SOURCES ${CMAKE_SOURCE_DIR}/app.rc)
endif()

add_executable(AquilaEditor ${EDITOR_SOURCES})

set_target_properties(AquilaEditor PROPERTIES
    OUTPUT_NAME "Aquila Studio"
    DEBUG_POSTFIX "(Debug)"
)

target_include_directories(AquilaEditor
    PRIVATE
    ${EDITOR_INCLUDE_DIR}
    ${EDITOR_SOURCE_DIR}
)

target_precompile_headers(AquilaEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/Include/GraphicsPCH.h
)

target_link_libraries(AquilaEditor
    PRIVATE
    Application
    GFX
    Graphics
    Scene
    Rendering
    UI
    glfw
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(AquilaEditor PRIVATE
        -Wall -Wextra
        -Wno-unused-parameter
        -Wno-deprecated-literal-operator
        -fcolor-diagnostics
        $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer -fno-inline>
        $<$<CONFIG:Release>:-O3 -march=native -g1>
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(AquilaEditor PRIVATE
        -Wall -Wextra
        -Wno-unused-parameter
        $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer>
        $<$<CONFIG:Release>:-O3 -g1>
    )
elseif (MSVC)
    target_compile_options(AquilaEditor PRIVATE
        /W3 /MP /bigobj
        $<$<CONFIG:Debug>:/Zi /Od>
        $<$<CONFIG:Release>:/O2>
    )
endif()

if (EXISTS ${CMAKE_SOURCE_DIR}/aquila-logo.ico)
    add_custom_command(TARGET AquilaEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/aquila-logo.ico
            $<TARGET_FILE_DIR:AquilaEditor>/aquila-logo.ico
        COMMENT "Copying Aquila logo"
    )
endif()

add_custom_command(TARGET AquilaEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${AQUILA_ASSET_PATH} $<TARGET_FILE_DIR:AquilaEditor>/Assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${AQUILA_RESOURCES_PATH} $<TARGET_FILE_DIR:AquilaEditor>/Resources
    COMMENT "Copying assets and resources"
    VERBATIM
)

if (WIN32 AND CMAKE_CONFIGURATION_TYPES)
    add_custom_command(TARGET AquilaEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:AquilaEditor>
            $<TARGET_FILE_DIR:AquilaEditor>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLLs"
    )
endif()

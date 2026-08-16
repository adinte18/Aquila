add_library(hosek STATIC ArHosekSkyModel.c)

target_include_directories(hosek PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(hosek PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
    target_compile_options(hosek PRIVATE -w)
endif()

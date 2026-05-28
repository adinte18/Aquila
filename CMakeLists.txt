cmake_minimum_required(VERSION 3.20)
project(Aquila LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif ()

option(AQUILA_BUILD_EDITOR "Build the Aquila editor" ON)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CACHE_PROGRAM "none")
find_program(SCCACHE_PROGRAM sccache)
find_program(CCACHE_PROGRAM ccache)
if (SCCACHE_PROGRAM)
    message(STATUS "Using sccache for faster rebuilds")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
    set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
    set(CACHE_PROGRAM "sccache")
elseif (CCACHE_PROGRAM)
    message(STATUS "Using ccache for faster rebuilds")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CACHE_PROGRAM "ccache")
else ()
    message(WARNING "No compiler cache found. Install sccache or ccache for faster rebuilds.")
endif ()

set(LINKER_NAME "default")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    find_program(LLD_LINKER lld)
    if (LLD_LINKER)
        message(STATUS "Using lld linker with Clang")
        add_link_options(-fuse-ld=lld)
        set(LINKER_NAME "lld")
    else ()
        message(WARNING "lld not found for Clang. Using default linker (slower). Install lld for faster linking.")
        set(LINKER_NAME "default (install lld for faster builds)")
    endif ()

elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if (UNIX AND NOT APPLE)
        find_program(MOLD_LINKER mold)
        if (MOLD_LINKER)
            message(STATUS "Using mold linker with GCC")
            add_link_options(-fuse-ld=mold)
            set(LINKER_NAME "mold")
        else ()
            find_program(LLD_LINKER lld)
            if (LLD_LINKER)
                message(STATUS "Using lld linker with GCC")
                add_link_options(-fuse-ld=lld)
                set(LINKER_NAME "lld")
            endif ()
        endif ()
    elseif (WIN32)
        find_program(LLD_LINKER ld.lld)
        if (LLD_LINKER)
            message(STATUS "Using lld linker (MinGW)")
            add_link_options(-fuse-ld=lld)
            set(LINKER_NAME "lld")
        else ()
            message(WARNING "lld not found. Using default ld (slow). Install lld with: pacman -S mingw-w64-x86_64-lld")
            set(LINKER_NAME "default (slow - install lld!)")
        endif ()
    endif ()

elseif (MSVC)
    add_compile_options(/MP16)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG:FULL /INCREMENTAL")
    else ()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL /DEBUG:FASTLINK")
    endif ()
    set(LINKER_NAME "MSVC")
endif ()

include(ProcessorCount)
ProcessorCount(NPROC)
if (NOT NPROC EQUAL 0)
    set(BUILD_PARALLELISM ${NPROC})
else ()
    set(BUILD_PARALLELISM 16)
endif ()

message(STATUS "Detected ${BUILD_PARALLELISM} logical cores")
message(STATUS "Parallel build enabled with ${BUILD_PARALLELISM} threads")

set(CMAKE_BUILD_PARALLEL_LEVEL ${BUILD_PARALLELISM})
set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} ${BUILD_PARALLELISM})

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
            -Xclang -fno-pch-timestamp
    )

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-O0 -g3 -fno-omit-frame-pointer -fno-inline)
    endif ()

    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-flto=thin)
        add_link_options(-flto=thin)
    endif ()
endif ()

if (WIN32)
    add_compile_definitions(AQUILA_PLATFORM_WINDOWS)
elseif (UNIX AND NOT APPLE)
    add_compile_definitions(AQUILA_PLATFORM_LINUX)
elseif (APPLE)
    add_compile_definitions(AQUILA_PLATFORM_MACOS)
else ()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif ()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_compile_definitions(AQUILA_PLATFORM_64BIT)
else ()
    add_compile_definitions(AQUILA_PLATFORM_32BIT)
endif ()

find_package(Git QUIET)

if (GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_BRANCH
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} status --porcelain
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_DIRTY
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (GIT_DIRTY)
        set(GIT_DIRTY_FLAG "-dirty")
    else()
        set(GIT_DIRTY_FLAG "")
    endif()

    if (GIT_TAG MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)(-.*)?$")
        set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})
        set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_2})
        set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_3})
        set(AQUILA_VERSION_SUFFIX  ${CMAKE_MATCH_4})
    else()
        message(WARNING "Could not parse git tag '${GIT_TAG}', falling back to 0.0.0")
        set(PROJECT_VERSION_MAJOR 0)
        set(PROJECT_VERSION_MINOR 0)
        set(PROJECT_VERSION_PATCH 0)
        set(AQUILA_VERSION_SUFFIX "")
    endif()

else()
    message(WARNING "Git not found, version will be 0.0.0")
    set(PROJECT_VERSION_MAJOR 0)
    set(PROJECT_VERSION_MINOR 0)
    set(PROJECT_VERSION_PATCH 0)
    set(AQUILA_VERSION_SUFFIX "")
    set(GIT_COMMIT_HASH "unknown")
    set(GIT_BRANCH "unknown")
    set(GIT_DIRTY_FLAG "")
endif()

set(AQUILA_VERSION_COMPOSED "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}${AQUILA_VERSION_SUFFIX}")

add_compile_definitions(
        AQUILA_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        AQUILA_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        AQUILA_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        AQUILA_VERSION_SUFFIX="${AQUILA_VERSION_SUFFIX}"
        AQUILA_VERSION_STRING="${AQUILA_VERSION_COMPOSED}"
        AQUILA_GIT_COMMIT="${GIT_COMMIT_HASH}${GIT_DIRTY_FLAG}"
        AQUILA_GIT_BRANCH="${GIT_BRANCH}"
)

message(STATUS "Aquila Version: ${AQUILA_VERSION_COMPOSED} (${GIT_BRANCH}@${GIT_COMMIT_HASH}${GIT_DIRTY_FLAG})")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(
            DEBUG
            AQUILA_DEBUG
            ENABLE_VALIDATION_LAYERS
            _DEBUG
    )
    message(STATUS "Validation layers: ENABLED")
    message(STATUS "Debug symbols: FULL")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_compile_definitions(
            AQUILA_RELEASE_DEBUG
            ENABLE_VALIDATION_LAYERS
    )
    message(STATUS "Validation layers: ENABLED (RelWithDebInfo)")
else ()
    message(STATUS "Validation layers: DISABLED")
endif ()

set(AQUILA_ROOT_DIR ${CMAKE_SOURCE_DIR})
set(AQUILA_ASSET_PATH "${CMAKE_SOURCE_DIR}/Assets" CACHE PATH "Asset directory path")
set(AQUILA_RESOURCES_PATH "${CMAKE_SOURCE_DIR}/Resources" CACHE PATH "Resources directory path")
set(AQUILA_ENGINE_IMMUTABLE_SHADER_PATH "${AQUILA_ROOT_DIR}/Engine/Shaders/")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
            -Wno-deprecated-literal-operator
    )
endif ()

mark_as_advanced(
        GLFW_BUILD_DOCS GLFW_BUILD_TESTS GLFW_BUILD_EXAMPLES GLFW_INSTALL
        ASSIMP_BUILD_TESTS ASSIMP_BUILD_ASSIMP_TOOLS ASSIMP_INSTALL
        SPIRV_SKIP_EXECUTABLES SPIRV_SKIP_TESTS
        ENABLE_SPVREMAPPER BUILD_EXTERNAL ENABLE_CTEST
)

add_compile_definitions(
    AQUILA_SHADERS_DIR="${CMAKE_SOURCE_DIR}/Engine/Shaders/"
    AQUILA_RESOURCES_DIR="${CMAKE_SOURCE_DIR}/Resources/"
)

add_subdirectory(Engine)

if (AQUILA_BUILD_EDITOR)
    add_subdirectory(Editor)
endif ()

if (AQUILA_BUILD_EDITOR)
    install(TARGETS AquilaEditor
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
    )
endif ()

install(DIRECTORY ${AQUILA_ASSET_PATH}
        DESTINATION share/aquila/assets
        PATTERN "*.tmp" EXCLUDE
)

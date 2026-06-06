if (NOT UNIX OR APPLE)
    return()
endif()

find_program(APT_CMD apt)
find_program(PACMAN_CMD pacman)

if (NOT APT_CMD AND NOT PACMAN_CMD)
    message(WARNING "No package manager found. Install dependencies manually.")
    return()
endif()

set(DEP_PROGRAMS "clang++" "ninja" "lld" "ccache" "git")
set(DEP_APT     "clang"   "ninja-build" "lld" "ccache" "git")
set(DEP_PACMAN  "clang"   "ninja"       "lld" "ccache" "git")
set(DEP_NAMES   "Clang compiler" "Ninja" "LLD" "ccache" "Git")

set(VULKAN_APT_PKGS    "libvulkan-dev" "vulkan-tools")
set(VULKAN_PACMAN_PKGS "vulkan-devel")

set(MISSING_NAMES   "")
set(MISSING_APT     "")
set(MISSING_PACMAN  "")

list(LENGTH DEP_PROGRAMS DEP_COUNT)
math(EXPR DEP_LAST "${DEP_COUNT} - 1")

foreach(I RANGE ${DEP_LAST})
    list(GET DEP_PROGRAMS ${I} PROG)
    list(GET DEP_APT      ${I} APT_PKG)
    list(GET DEP_PACMAN   ${I} PAC_PKG)
    list(GET DEP_NAMES    ${I} DNAME)

    find_program(_DEP_FOUND ${PROG})
    if (NOT _DEP_FOUND)
        list(APPEND MISSING_NAMES "  - ${DNAME}")
        list(APPEND MISSING_APT    ${APT_PKG})
        list(APPEND MISSING_PACMAN ${PAC_PKG})
    endif()
    unset(_DEP_FOUND CACHE)
endforeach()

# Check Vulkan (package manager path)
find_package(Vulkan QUIET)
if (NOT Vulkan_FOUND)
    list(APPEND MISSING_NAMES "  - Vulkan SDK")
    list(APPEND MISSING_APT    ${VULKAN_APT_PKGS})
    list(APPEND MISSING_PACMAN ${VULKAN_PACMAN_PKGS})
endif()

if (NOT DEFINED ENV{VULKAN_SDK} AND AQUILA_LUNARG_SDK_PATH)
    set(ENV{VULKAN_SDK} "${AQUILA_LUNARG_SDK_PATH}")
endif()

# Slang should exists in the LunarG tarball SDK per https://vulkan.lunarg.com/doc/sdk/1.4.350.1/linux/getting_started.html.
set(SLANG_FOUND FALSE)
if (DEFINED ENV{VULKAN_SDK})
    find_library(_SLANG_CHECK
        NAMES libslang slang
        PATHS "$ENV{VULKAN_SDK}/lib"
        NO_DEFAULT_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH
    )
    if (_SLANG_CHECK)
        set(SLANG_FOUND TRUE)
    endif()
    unset(_SLANG_CHECK CACHE)
endif()

if (NOT SLANG_FOUND)
    message(STATUS "  Slang requires the LunarG Vulkan SDK tarball.")

    if (DEFINED ENV{AQUILA_AUTO_INSTALL})
        set(SLANG_USER_CHOICE "y")
    else()
        set(SLANG_PROMPT_SCRIPT "${CMAKE_BINARY_DIR}/prompt_slang.sh")
        file(WRITE ${SLANG_PROMPT_SCRIPT}
            "#!/bin/bash\nread -p 'Auto-download LunarG Vulkan SDK (includes Slang)? [y/N]: ' choice\necho $choice\n"
        )
        execute_process(
            COMMAND bash ${SLANG_PROMPT_SCRIPT}
            INPUT_FILE /dev/tty
            OUTPUT_VARIABLE SLANG_USER_CHOICE
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()

    if (SLANG_USER_CHOICE MATCHES "^[Yy]$")
        # Fetch latest SDK version from LunarG
        message(STATUS "Fetching latest Vulkan SDK version...")
        execute_process(
            COMMAND bash -c "curl -s https://vulkan.lunarg.com/sdk/latest/linux.txt"
            OUTPUT_VARIABLE LUNARG_SDK_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE CURL_RESULT
        )
        if (NOT CURL_RESULT EQUAL 0 OR LUNARG_SDK_VERSION STREQUAL "")
            message(FATAL_ERROR "Failed to fetch latest SDK version. Check your internet connection.")
        endif()
        message(STATUS "Latest Vulkan SDK version: ${LUNARG_SDK_VERSION}")

        set(LUNARG_SDK_URL "https://sdk.lunarg.com/sdk/download/${LUNARG_SDK_VERSION}/linux/vulkansdk-linux-x86_64-${LUNARG_SDK_VERSION}.tar.xz")
        set(LUNARG_SDK_DIR "$ENV{HOME}/.lunarg-sdk")
        set(LUNARG_SDK_TARBALL "${LUNARG_SDK_DIR}/vulkansdk-linux-x86_64-${LUNARG_SDK_VERSION}.tar.xz")
        set(LUNARG_SDK_EXTRACTED "${LUNARG_SDK_DIR}/${LUNARG_SDK_VERSION}")

        file(MAKE_DIRECTORY ${LUNARG_SDK_DIR})

        if (NOT EXISTS ${LUNARG_SDK_EXTRACTED})
            message(STATUS "Downloading ${LUNARG_SDK_URL} ...")
            execute_process(
                COMMAND curl -L --progress-bar -o "${LUNARG_SDK_TARBALL}" "${LUNARG_SDK_URL}"
                RESULT_VARIABLE DOWNLOAD_RESULT
            )
            if (NOT DOWNLOAD_RESULT EQUAL 0)
                message(FATAL_ERROR "Download failed. Try manually:\n  curl -L -o ${LUNARG_SDK_TARBALL} ${LUNARG_SDK_URL}")
            endif()

            message(STATUS "Extracting SDK...")
            execute_process(
                COMMAND tar xf "${LUNARG_SDK_TARBALL}" -C "${LUNARG_SDK_DIR}"
                RESULT_VARIABLE EXTRACT_RESULT
            )
            if (NOT EXTRACT_RESULT EQUAL 0)
                message(FATAL_ERROR "Extraction failed.")
            endif()

            file(REMOVE "${LUNARG_SDK_TARBALL}")
        else()
            message(STATUS "SDK already extracted at ${LUNARG_SDK_EXTRACTED}, skipping download.")
        endif()

        # Set VULKAN_SDK for this CMake run
        set(ENV{VULKAN_SDK} "${LUNARG_SDK_EXTRACTED}/x86_64")
        set(VULKAN_SDK "${LUNARG_SDK_EXTRACTED}/x86_64" CACHE PATH "Vulkan SDK path" FORCE)

        # cache the path
        set(AQUILA_LUNARG_SDK_PATH "${LUNARG_SDK_EXTRACTED}/x86_64" CACHE PATH "Auto-installed LunarG SDK path" FORCE)

        # Persist to ~/.profile so future shells have it
        set(SETUP_LINE "source ${LUNARG_SDK_EXTRACTED}/setup-env.sh")
        execute_process(
            COMMAND bash -c "grep -qxF '${SETUP_LINE}' $ENV{HOME}/.profile || echo '${SETUP_LINE}' >> $ENV{HOME}/.profile"
        )
        message(STATUS "Added SDK to ~/.profile. Run 'source ~/.profile' after cmake completes.")

        # Re-check Slang now that VULKAN_SDK is set
        find_library(_SLANG_RECHECK
            NAMES libslang slang
            PATHS "${VULKAN_SDK}/lib"
            NO_DEFAULT_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH
        )
        if (_SLANG_RECHECK)
            set(SLANG_FOUND TRUE)
            message(STATUS "Slang found after SDK install: ${_SLANG_RECHECK}")
        else()
            message(FATAL_ERROR "SDK downloaded but Slang still not found at ${VULKAN_SDK}/lib. Check the extraction.")
        endif()
        unset(_SLANG_RECHECK CACHE)

    else()
        message(STATUS "  Manual install:")
        message(STATUS "    1. Download from https://vulkan.lunarg.com")
        message(STATUS "    2. Extract:  tar xf vulkansdk-linux-x86_64-<ver>.tar.xz -C ~/vulkan")
        message(STATUS "    3. Activate: source ~/vulkan/<ver>/setup-env.sh")
        message(STATUS "    4. Re-run cmake")
        message(FATAL_ERROR "Slang not found. See instructions above.")
    endif()
endif()

if (NOT MISSING_NAMES)
    return()
endif()

# Build install hint
if (APT_CMD)
    list(JOIN MISSING_APT " " PKGS_STR)
    set(INSTALL_CMD "sudo apt install -y ${PKGS_STR}")
else()
    list(JOIN MISSING_PACMAN " " PKGS_STR)
    set(INSTALL_CMD "sudo pacman -S --noconfirm ${PKGS_STR}")
endif()

message(STATUS "")
message(STATUS "Aquila: missing dependencies:")
foreach(M ${MISSING_NAMES})
    message(STATUS "${M}")
endforeach()
message(STATUS "")

if (NOT SLANG_FOUND)
    message(STATUS "  Slang requires the LunarG Vulkan SDK tarball:")
    message(STATUS "    1. Download from https://vulkan.lunarg.com")
    message(STATUS "    2. Extract:  tar xf vulkansdk-linux-x86_64-<ver>.tar.xz -C ~/vulkan")
    message(STATUS "    3. Activate: source ~/vulkan/<ver>/setup-env.sh")
    message(STATUS "    4. Re-run cmake")
    message(STATUS "")
endif()

# Only offer auto-install for package manager deps (not the tarball)
list(LENGTH MISSING_APT HAS_PKG_DEPS)
if (HAS_PKG_DEPS GREATER 0)
    message(STATUS "Install command for package deps: ${INSTALL_CMD}")
    message(STATUS "")

    if (DEFINED ENV{AQUILA_AUTO_INSTALL})
        set(USER_CHOICE "y")
    else()
        set(PROMPT_SCRIPT "${CMAKE_BINARY_DIR}/prompt_install.sh")
        file(WRITE ${PROMPT_SCRIPT} "#!/bin/bash\nread -p 'Install missing package dependencies? [y/N]: ' choice\necho $choice\n")
        execute_process(
            COMMAND bash ${PROMPT_SCRIPT}
            INPUT_FILE /dev/tty
            OUTPUT_VARIABLE USER_CHOICE
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()

    if (USER_CHOICE MATCHES "^[Yy]$")
        message(STATUS "Installing...")
        execute_process(
            COMMAND bash -c "${INSTALL_CMD}"
            RESULT_VARIABLE INSTALL_RESULT
        )
        if (NOT INSTALL_RESULT EQUAL 0)
            message(FATAL_ERROR "Installation failed. Run manually: ${INSTALL_CMD}")
        endif()
        message(WARNING "Dependencies installed. Please re-run cmake.")
    else()
        message(WARNING "Missing dependencies. Install manually: ${INSTALL_CMD}")
    endif()
endif()

if (NOT SLANG_FOUND)
    message(FATAL_ERROR "Slang not found. See instructions above.")
endif()

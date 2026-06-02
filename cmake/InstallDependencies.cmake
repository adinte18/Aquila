if (NOT UNIX OR APPLE)
    return()
endif()

find_program(APT_CMD apt)
find_program(PACMAN_CMD pacman)

if (NOT APT_CMD AND NOT PACMAN_CMD)
    message(WARNING "No package manager found. Install dependencies manually.")
    return()
endif()

# parallel lists indexed by position - cmake doesn't support list-of-structs
set(DEP_PROGRAMS "clang++" "ninja" "lld" "ccache" "git")
set(DEP_APT "clang" "ninja-build" "lld" "ccache" "git")
set(DEP_PACMAN "clang" "ninja" "lld" "ccache" "git")
set(DEP_NAMES "Clang compiler" "Ninja" "LLD" "ccache" "Git")

set(VULKAN_APT_PKGS "libvulkan-dev" "vulkan-tools")
set(VULKAN_PACMAN_PKGS "vulkan-devel")

set(MISSING_NAMES "")
set(MISSING_APT "")
set(MISSING_PACMAN "")

list(LENGTH DEP_PROGRAMS DEP_COUNT)
math(EXPR DEP_LAST "${DEP_COUNT} - 1")

foreach(I RANGE ${DEP_LAST})
    list(GET DEP_PROGRAMS ${I} PROG)
    list(GET DEP_APT ${I} APT_PKG)
    list(GET DEP_PACMAN ${I} PAC_PKG)
    list(GET DEP_NAMES ${I} DNAME)

    find_program(_DEP_FOUND ${PROG})
    if (NOT _DEP_FOUND)
        list(APPEND MISSING_NAMES "  - ${DNAME}")
        list(APPEND MISSING_APT ${APT_PKG})
        list(APPEND MISSING_PACMAN ${PAC_PKG})
    endif()
    unset(_DEP_FOUND CACHE)
endforeach()

find_package(Vulkan QUIET)
if (NOT Vulkan_FOUND)
    list(APPEND MISSING_NAMES "  - Vulkan SDK")
    list(APPEND MISSING_APT ${VULKAN_APT_PKGS})
    list(APPEND MISSING_PACMAN ${VULKAN_PACMAN_PKGS})
endif()

if (NOT MISSING_NAMES)
    return()
endif()

# build install command from whichever package manager was found
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
message(STATUS "Install command: ${INSTALL_CMD}")
message(STATUS "")

# allow non-interactive installs via env var
if (DEFINED ENV{AQUILA_AUTO_INSTALL})
    set(USER_CHOICE "y")
else()
    set(PROMPT_SCRIPT "${CMAKE_BINARY_DIR}/prompt_install.sh")
    file(WRITE ${PROMPT_SCRIPT} "#!/bin/bash\nread -p 'Install missing dependencies? [y/N]: ' choice\necho $choice\n")
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
    return()
else()
    message(WARNING "Missing dependencies. Install manually: ${INSTALL_CMD}")
    return()
endif()

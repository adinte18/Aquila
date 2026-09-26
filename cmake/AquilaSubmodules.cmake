find_package(Git QUIET)

if (NOT EXISTS "${CMAKE_SOURCE_DIR}/.git")
    return()
endif ()

if (NOT GIT_FOUND)
    message(FATAL_ERROR "Git is required to fetch Aquila's submodules. Install git, or clone with: git clone --recursive <url>")
endif ()

execute_process(
    COMMAND ${GIT_EXECUTABLE} submodule status --recursive
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE _aquila_submodule_status
    RESULT_VARIABLE _aquila_submodule_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (NOT _aquila_submodule_result EQUAL 0)
    message(FATAL_ERROR "Could not read the git submodule status of ${CMAKE_SOURCE_DIR}")
endif ()

string(REPLACE "\n" ";" _aquila_submodule_lines "${_aquila_submodule_status}")
set(_aquila_missing_submodules "")
set(_aquila_moved_submodules "")
foreach (_line IN LISTS _aquila_submodule_lines)
    string(SUBSTRING "${_line}" 0 1 _state)
    string(REGEX REPLACE "^.[0-9a-f]+ ([^ ]+).*$" "\\1" _path "${_line}")
    if (_state STREQUAL "-")
        list(APPEND _aquila_missing_submodules ${_path})
    elseif (_state STREQUAL "+")
        list(APPEND _aquila_moved_submodules ${_path})
    endif ()
endforeach ()

if (_aquila_missing_submodules)
    list(JOIN _aquila_missing_submodules ", " _missing_text)
    message(STATUS "Aquila: fetching submodules (${_missing_text})")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE _aquila_submodule_result
    )
    if (NOT _aquila_submodule_result EQUAL 0)
        message(FATAL_ERROR "git submodule update --init --recursive failed. Check your network connection and run it manually.")
    endif ()
endif ()

if (_aquila_moved_submodules)
    list(JOIN _aquila_moved_submodules ", " _moved_text)
    message(WARNING "Aquila: these submodules are not at the commit this checkout expects: ${_moved_text}. "
                    "Run 'git submodule update --recursive' unless you changed them on purpose.")
endif ()

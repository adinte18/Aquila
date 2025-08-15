cmake_minimum_required(VERSION 3.10)
project(App)

add_executable(App
    ${CMAKE_SOURCE_DIR}/App/Source/Application.cpp
)

target_link_libraries(App PRIVATE Core Editor)
add_executable(InputTesting
    InputTesting.cpp
)
target_precompile_headers(InputTesting REUSE_FROM Application)
target_link_libraries(InputTesting
    PRIVATE
    Application
    glfw
    Vulkan::Headers
)

add_executable(RenderPassSandbox
    RenderPassSandbox.cpp
)
target_precompile_headers(RenderPassSandbox REUSE_FROM Application)
target_link_libraries(RenderPassSandbox
    PRIVATE
    Application
    GFX
    Graphics
    glfw
    Vulkan::Headers
)
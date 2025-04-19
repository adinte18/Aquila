# Aquila - Vulkan PBR Rendering Engine

This project is a **Vulkan-based PBR (Physically-Based Rendering) engine** built entirely from scratch with the primary goal of learning and exploring modern graphics programming, Vulkan API usage, and real-time rendering techniques.

## ⚠️ Disclaimer

> **This project is for educational purposes only.**  
> It is not designed to compete with existing game engines, commercial or open-source, nor is that its goal. The primary intent is personal growth and experimentation in the field of graphics programming.


## About

This engine serves as a hands-on learning platform to:

- Understand the Vulkan graphics API in depth
- Implement modern PBR shading workflows
- Integrate image-based lighting (IBL)
- Explore efficient rendering pipelines and GPU resource management
- Experiment with common rendering features like shadow mapping, normal mapping, and post-processing

## Features

- **Physically-Based Rendering (PBR)** using metallic-roughness workflow
- **Runtime IBL (Image-Based Lighting)** computation from HDR environment maps
- **Material editing** at runtime, including base color, metallic, roughness, emissive, and AO control
- **Model loading** with Assimp (.obj, .gltf, etc.)
- Real-time viewport rendering
- Clean and modular C++ architecture focused on learning and extensibility
- Fallback texture system for missing material maps
---

## Dependencies

This project uses a collection of libraries and tools to support Vulkan rendering, model loading, UI, and utility functionality. Below is a list of key dependencies and their versions or commit hashes used at the time of development.

### Core Libraries

- [**GLFW**](https://github.com/glfw/glfw) – Windowing and input
- [**GLM**](https://github.com/g-truc/glm) – Mathematics for graphics
- [**Assimp**](https://github.com/assimp/assimp) – Model loading (.obj, .gltf, etc.)
- [**ImGui**](https://github.com/ocornut/imgui) – Immediate-mode GUI
- [**ImGuizmo**](https://github.com/CedricGuillemet/ImGuizmo) – Editor gizmo tools
- [**nativefiledialog**](https://github.com/mlabbe/nativefiledialog) – Native file picker dialogs

### Vulkan Toolchain

- [**glslang**](https://github.com/KhronosGroup/glslang) – GLSL to SPIR-V compilation
- [**SPIRV-Tools**](https://github.com/KhronosGroup/SPIRV-Tools) – SPIR-V binary tools
- [**SPIRV-Headers**](https://github.com/KhronosGroup/SPIRV-Headers) – Required SPIR-V header definitions
- [**shaderc**](https://github.com/google/shaderc) – Shader compiler frontend

### Header-only Utilities

- [`entt.h`](https://github.com/skypjack/entt) – Entity-Component-System (ECS) framework (WIP)
- [`json.hpp`](https://github.com/nlohmann/json) – JSON parsing and serialization
- [`tiny_gltf.h`](https://github.com/syoyo/tinygltf) – Lightweight GLTF loader (alternative)
- [`stb_image.h`](https://github.com/nothings/stb) – Image loading
- [`stb_image_resize2.h`](https://github.com/nothings/stb) – Image resizing
- [`stb_image_write.h`](https://github.com/nothings/stb) – Image writing
- [`stb_ds.h`](https://github.com/nothings/stb) – Dynamic arrays and hash tables
- [`stb_truetype.h`](https://github.com/nothings/stb) – Font rendering

---

> **Note:** All dependencies are included via submodules or directly in the source tree where applicable.

Thanks for checking it out! Feel free to explore, learn, and build upon it for your own Vulkan journey.

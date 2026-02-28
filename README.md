#Aquila - Vulkan PBR Rendering Engine

This project is my personal Vulkan-based engine built entirely from scratch with the primary goal of learning and exploring graphics programming and scalable (and maintanable) project architecture. It is still a work in progress, and its lacking comments and documentation (I am pretty inconsistent with this :/). It should come pretty soon though.

The initial idea was to keep it on Vulkan, but I am really veering towards integrating multiple graphics API support.

> **This project is for educational purposes only.**
> The current state of the engine is not designed to compete with existing game engines, commercial or open-source, nor is that its goal. The primary intent is personal growth and experimentation in the field of graphics programming.

## About

This engine serves as a hands-on learning platform to:

- Understand the Vulkan graphics API in depth
- Implement modern PBR shading workflows
- Explore efficient rendering pipelines and GPU resource management

## Features

The engine contains some cool features that I am proud of : 

- Physically-Based Rendering using metallic-roughness workflow
- Runtime IBL computation from HDR environment maps
- Material editing at runtime
- Model loading with Assimp, although I am trying to move away from it
- Clean(-ish) and modular C++ architecture

---

## Dependencies

This project uses a collection of libraries and tools to support Vulkan rendering, model loading, UI, and utility functionality. Below is a list of key dependencies and their versions or commit hashes used at the time of development.

### Core Libraries

- [**GLFW**](https://github.com/glfw/glfw) – Windowing and input
- [**GLM**](https://github.com/g-truc/glm) – Mathematics for graphics
- [**Assimp**](https://github.com/assimp/assimp) – Model loading
- [**ImGui**](https://github.com/ocornut/imgui) – Immediate-mode GUI
- [**ImGuizmo**](https://github.com/CedricGuillemet/ImGuizmo) – Editor gizmo tools
- [**SLang**](https://github.com/shader-slang/slang) - Shader language 
### Header-only Utilities

- [`entt.h`](https://github.com/skypjack/entt) – Entity-Component-System (ECS) framework (WIP)
- [`json.hpp`](https://github.com/nlohmann/json) – JSON parsing and serialization
- [`stb_image.h`](https://github.com/nothings/stb) – Image loading
- [`stb_image_resize2.h`](https://github.com/nothings/stb) – Image resizing
- [`stb_image_write.h`](https://github.com/nothings/stb) – Image writing
- [`stb_ds.h`](https://github.com/nothings/stb) – Dynamic arrays and hash tables
- [`stb_truetype.h`](https://github.com/nothings/stb) – Font rendering

---

> **Note:** All dependencies are included via submodules or directly in the source tree where applicable.

Thanks for checking it out! Feel free to explore, learn, and build upon it for your own Vulkan journey.

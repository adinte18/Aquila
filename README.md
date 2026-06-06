# Aquila - Vulkan PBR Rendering Engine

This project is my personal Vulkan-based engine built entirely from scratch with the primary goal of learning and exploring graphics programming and scalable (and maintanable) project architecture. It is still a work in progress, and its lacking comments and documentation (I am pretty inconsistent with this :/). It should come pretty soon though.

The initial idea was to keep it on Vulkan, but I am really veering towards integrating multiple graphics API support.

> **This project is for educational purposes only.**
> The current state of the engine is not designed to compete with existing game engines, commercial or open-source, nor is that its goal. The primary intent is personal growth and experimentation in the field of graphics programming.

## Build

### Configure Presets

#### Windows

| Preset | Build Type |
|--------|-----------|
| `windows-editor-debug` | Debug|
| `windows-editor-release` | Release |
| `windows-editor-relwithdebinfo` | RelWithDebInfo |
| `windows-engine-debug` | Debug |
| `windows-engine-release` | Release |
| `windows-engine-relwithdebinfo` | RelWithDebInfo |

#### Linux

| Preset | Build Type |
|--------|-----------|
| `linux-editor-debug` | Debug |
| `linux-editor-release` | Release |
| `linux-editor-relwithdebinfo` | RelWithDebInfo |
| `linux-engine-debug` | Debug |
| `linux-engine-release` | Release |
| `linux-engine-relwithdebinfo` | RelWithDebInfo |

## Usage

### Configure

```bash
cmake --preset <preset-name>
```

### Build

```bash
cmake --build --preset <preset-name>
```

### Example

```bash
cmake --preset windows-engine-debug
cmake --build --preset windows-engine-debug
```

## Base Configuration

All presets inherit from `base` which sets:

- **Generator:** Ninja
- **C Compiler:** clang
- **C++ Compiler:** clang++
- **Build directory:** `build/<preset-name>`

## Dependencies

This project uses a collection of libraries and tools to support Vulkan rendering, model loading, UI, and utility functionality. Below is a list of key dependencies and their versions or commit hashes used at the time of development.

### Core Libraries

- [**GLFW**](https://github.com/glfw/glfw) – Windowing and input
- [**GLM**](https://github.com/g-truc/glm) – Mathematics for graphics
- [**Assimp**](https://github.com/assimp/assimp) – Model loading
- [**stb**](https://github.com/nothings/stb) - Image processing

### Header-only Utilities

- [`entt.h`](https://github.com/skypjack/entt) – Entity-Component-System (ECS) framework (WIP)
- [`json.hpp`](https://github.com/nlohmann/json) – JSON parsing and serialization

---

> **Note:** All dependencies are included via submodules or directly in the source tree where applicable.

Thanks for checking it out! Feel free to explore, learn, and build upon it for your own Vulkan journey.

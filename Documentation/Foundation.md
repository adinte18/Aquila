# Foundation

**Location**: `Include/Aquila/Foundation/`

The Foundation layer provides primitive types, smart pointer aliases, utility macros, and global subsystem utilities. Every other layer depends on it.

---

## Primitive Types (`PrimitiveTypes.h`)

All engine code uses these aliases rather than raw C++ types:

```cpp
uint8, uint16, uint32, uint64
int8,  int16,  int32,  int64
f32, f64
usize, isize, uptr, iptr
```

### Smart Pointers

| Alias | Underlying | Ownership |
|-------|-----------|-----------|
| `Ref<T>` | `std::shared_ptr<T>` | Shared |
| `Unique<T>` | `std::unique_ptr<T>` | Exclusive |
| `WeakRef<T>` | `std::weak_ptr<T>` | Non-owning |

Factory helpers: `CreateRef<T>(args...)`, `CreateUnique<T>(args...)`.

### Other Aliases

| Alias | Underlying |
|-------|-----------|
| `Delegate<T>` | `std::function<T>` |
| `Option<T>` | `std::optional<T>` |
| `Variant<Ts...>` | `std::variant<Ts...>` |
| `Result<Ok, Err>` | Custom result type |

---

## Macros (`Macros.h`, `Defines.h`)

| Macro | Purpose |
|-------|---------|
| `AQUILA_NONCOPYABLE(T)` | Deletes copy constructor + assignment |
| `AQUILA_NONMOVEABLE(T)` | Deletes move constructor + assignment |
| `AQUILA_FORCE_INLINE` / `AQUILA_INLINE` | Compiler hint for inlining |
| `AQUILA_ASSERT(cond, msg)` | Debug assertion with message |
| `AQUILA_VULKAN_CHECK(expr)` | VkResult check that asserts on failure |
| `AQUILA_LOG_INFO/WARN/ERROR/…` | Structured logging via `Logger` |
| `BIT(n)` | `(1u << n)` — shorthand for bitmask enums |

---

## UUID (`UUID.h`)

128-bit identifier stored as two `uint64` values.

- `UUID::Generate()` — version 4 (random)
- `UUID::FromFilepath(path)` — version 5 (SHA-1 hash, deterministic from path)
- Hashable: custom `std::hash<UUID>` for use in `unordered_map`

---

## Singleton (`Singleton.h`)

Template singleton with explicit `Init(args...)` / `Shutdown()` lifecycle. Used for global subsystems (e.g., `ViewSystem`).

```cpp
// Initialize
MySystem::Init(arg1, arg2);

// Access
MySystem::Get()->DoSomething();

// Shutdown
MySystem::Shutdown();
```

---

## Logging (`Log.h`)

Structured, levelled logging with timestamps and source location. Outputs to configurable sinks. Format strings use `std::vformat`.

```cpp
AQUILA_LOG_INFO("Loaded mesh: {}", mesh.GetName());
AQUILA_LOG_WARN("Texture missing, using fallback");
AQUILA_LOG_ERROR("Shader compile failed: {}", err);
```

#ifndef AQUILA_TYPES_H
#define AQUILA_TYPES_H

#include "Aquila/Core/Defines.h"

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;
using f32 = float;
using f64 = double;
using byte = std::byte;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;
using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;
using mat3 = glm::mat3;
using mat4 = glm::mat4;
using quaternion = glm::quat;

enum class AccessMode : uint8 { Read = 0, Write = 1, ReadWrite = 2 };

template <typename T> using Ref = std::shared_ptr<T>;
template <typename T> using Unique = std::unique_ptr<T>;
template <typename T> using WeakRef = std::weak_ptr<T>;
template <typename T> using Delegate = std::function<T>;

template <typename T, typename... Args> constexpr Ref<T> CreateRef(Args &&...args) {
	return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args> constexpr Unique<T> CreateUnique(Args &&...args) {
	return std::make_unique<T>(std::forward<Args>(args)...);
}

enum class Priority : uint8 { VeryHigh = BIT(1), High = BIT(2), Medium = BIT(3), Low = BIT(4) };

#endif

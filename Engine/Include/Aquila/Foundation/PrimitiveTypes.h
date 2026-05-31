#ifndef AQUILA_TYPES_H
#define AQUILA_TYPES_H

#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/Math/MathTypes.h"

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

using usize = std::size_t;
using isize = std::ptrdiff_t;
using uptr = std::uintptr_t;
using iptr = std::intptr_t;

using Mutex = std::shared_mutex;
using MutexLock = std::unique_lock<Mutex>; // exclusive write lock
using SharedLock = std::shared_lock<Mutex>; // shared read lock

template <typename T> using Ref = std::shared_ptr<T>;
template <typename T> using Unique = std::unique_ptr<T>;
template <typename T> using WeakRef = std::weak_ptr<T>;
template <typename T> using Delegate = std::function<T>;

template <typename T> using Option = std::optional<T>;
template <typename... Ts> using Variant = std::variant<Ts...>;
template <typename Ok, typename Err> using Result = std::variant<Ok, Err>;

template <typename... Visitors> struct Overloaded : Visitors... {
	using Visitors::operator()...;
};
template <typename... Visitors> Overloaded(Visitors...) -> Overloaded<Visitors...>;

template <typename T, typename... Args> constexpr Ref<T> CreateRef(Args &&...args) {
	return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args> constexpr Unique<T> CreateUnique(Args &&...args) {
	return std::make_unique<T>(std::forward<Args>(args)...);
}

enum class AccessMode : uint8 { Read = 0, Write = 1, ReadWrite = 2 };
enum class OpenMode : uint8 {
	Text = 0, // default, text mode
	Binary = BIT(1), // "b" no newline translation
	Append = BIT(2), // "a" writes go to end
	Truncate = BIT(3), // "w" clears file on open
	Create = BIT(4), // create if not exists
};

AQUILA_FORCE_INLINE OpenMode operator|(OpenMode a, OpenMode b) {
	return static_cast<OpenMode>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

AQUILA_FORCE_INLINE OpenMode operator&(OpenMode a, OpenMode b) {
	return static_cast<OpenMode>(static_cast<uint8>(a) & static_cast<uint8>(b));
}

AQUILA_FORCE_INLINE bool HasFlag(OpenMode mode, OpenMode flag) {
	return (static_cast<uint8>(mode) & static_cast<uint8>(flag)) != 0;
}

enum class Priority : uint8 { VeryHigh = BIT(1), High = BIT(2), Medium = BIT(3), Low = BIT(4) };

#endif

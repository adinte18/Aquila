#ifndef AQUILA_DEFINES_H
#define AQUILA_DEFINES_H

#include "Aquila/Utilities/Log.h"

#define BIT(x) (1 << x)

#define AQUILA_NONCOPYABLE(ClassName)      \
	ClassName(const ClassName &) = delete; \
	ClassName &operator=(const ClassName &) = delete;

#define AQUILA_NONMOVEABLE(ClassName) \
	ClassName(ClassName &&) = delete; \
	ClassName &operator=(ClassName &&) = delete

#define AQUILA_UNIMPLEMENTED() AQUILA_ASSERT(false, "Not implemented: {}", __FUNCTION__)

// NOTE (Alex) :
// Note to self, FORCE_INLINE should be reserved for functions that are small, hot,
// and called frequently in tight loops.
// Slapping it on everything actually hurts performance by bloating instruction cache.
#ifdef _MSC_VER
#define AQUILA_FORCE_INLINE __forceinline
#define AQUILA_NEVER_INLINE __declspec(noinline)
#define AQUILA_RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
#define AQUILA_FORCE_INLINE __attribute__((always_inline)) inline
#define AQUILA_NEVER_INLINE __attribute__((noinline))
#define AQUILA_RESTRICT __restrict__
#else
#define AQUILA_FORCE_INLINE inline
#define AQUILA_NEVER_INLINE
#define AQUILA_RESTRICT
#endif

// NOTE (Alex) :
// This is unnecessary... It was cool when C was a thing, but im using a pretty modern C++, this can be deleted, and should be
// TODO : Delete this completely
#if defined(__GNUC__) || defined(__clang__)
#define AQUILA_LIKELY(x) __builtin_expect(!!(x), 1)
#define AQUILA_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define AQUILA_LIKELY(x) (x)
#define AQUILA_UNLIKELY(x) (x)
#endif

#define AQUILA_LOG_TRACE(...) Aquila::Utils::LogTrace(__VA_ARGS__)
#define AQUILA_LOG_DEBUG(...) Aquila::Utils::LogDebug(__VA_ARGS__)
#define AQUILA_LOG_INFO(...) Aquila::Utils::LogInfo(__VA_ARGS__)
#define AQUILA_LOG_WARNING(...) Aquila::Utils::LogWarning(__VA_ARGS__)
#define AQUILA_LOG_ERROR(...) Aquila::Utils::LogError(__VA_ARGS__)
#define AQUILA_LOG_CRITICAL(...) Aquila::Utils::LogCritical(__VA_ARGS__)

#define AQUILA_UNUSED(x) (void)(x)

#define AQUILA_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define AQUILA_OFFSETOF(type, member) offsetof(type, member)

#ifdef _MSC_VER
#define AQUILA_DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define AQUILA_DEBUG_BREAK() __builtin_trap()
#else
#define AQUILA_DEBUG_BREAK() ((void)0)
#endif

#ifdef _MSC_VER
#define AQUILA_DEPRECATED(msg) __declspec(deprecated(msg))
#elif defined(__GNUC__) || defined(__clang__)
#define AQUILA_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
#define AQUILA_DEPRECATED(msg)
#endif

#if defined(AQUILA_DEBUG)
#define AQUILA_ASSERT(condition, message)                                         \
	do {                                                                          \
		if (AQUILA_UNLIKELY(!(condition))) {                                      \
			Aquila::Utils::AssertFailed(#condition, message, __FILE__, __LINE__); \
		}                                                                         \
	} while (0)

#else
#define AQUILA_ASSERT(condition, message)                                         \
	do {                                                                          \
		if (!(condition)) {                                                       \
			Aquila::Utils::AssertFailed(#condition, message, __FILE__, __LINE__); \
		}                                                                         \
	} while (false)
#endif

#define AQUILA_VULKAN_CHECK(call)                                                                            \
	do {                                                                                                     \
		VkResult result = (call);                                                                            \
		if (AQUILA_UNLIKELY(result != VK_SUCCESS)) {                                                         \
			Aquila::Utils::LogError(std::string("[VULKAN] Call failed: ") + #call + " | File: " + __FILE__ + \
									" | Line: " + std::to_string(__LINE__));                                 \
			Aquila::Utils::AssertFailed(#call, "Vulkan call failed", __FILE__, __LINE__);                    \
		}                                                                                                    \
	} while (0)

#define EVENT_CLASS_TYPE(type)                     \
	static const char *GetStaticName() {           \
		return #type;                              \
	}                                              \
	virtual const char *GetName() const override { \
		return GetStaticName();                    \
	}

#define EVENT_CLASS_CATEGORY(category)                   \
	virtual EventCategory GetCategory() const override { \
		return category;                                 \
	}

#endif // AQUILA_DEFINES_H

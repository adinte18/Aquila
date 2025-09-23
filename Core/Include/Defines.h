#include "Utilities/Log.h"

#define BIT(x) (1 << x)
#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#define AQUILA_NONCOPYABLE(ClassName)                                          \
  ClassName(const ClassName &) = delete;                                       \
  ClassName &operator=(const ClassName &) = delete;

#define AQUILA_NONMOVEABLE(ClassName)                                          \
  ClassName(ClassName &&) = delete;                                            \
  ClassName &operator=(ClassName &&) = delete

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

#if defined(__GNUC__) || defined(__clang__)
#define AQUILA_LIKELY(x) __builtin_expect(!!(x), 1)
#define AQUILA_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define AQUILA_LIKELY(x) (x)
#define AQUILA_UNLIKELY(x) (x)
#endif

#define AQUILA_LOG_TRACE(...) Utility::LogTrace(__VA_ARGS__)
#define AQUILA_LOG_DEBUG(...) Utility::LogDebug(__VA_ARGS__)
#define AQUILA_LOG_INFO(...) Utility::LogInfo(__VA_ARGS__)
#define AQUILA_LOG_WARNING(...) Utility::LogWarning(__VA_ARGS__)
#define AQUILA_LOG_ERROR(...) Utility::LogError(__VA_ARGS__)
#define AQUILA_LOG_CRITICAL(...) Utility::LogCritical(__VA_ARGS__)

#if defined(AQUILA_DEBUG)
#define AQUILA_ASSERT(condition, message)                                      \
  do {                                                                         \
    if (AQUILA_UNLIKELY(!(condition))) {                                       \
      Utility::AssertFailed(#condition, message, __FILE__, __LINE__);          \
    }                                                                          \
  } while (0)

#else
#define AQUILA_ASSERT(condition, message)                                      \
  do {                                                                         \
    if (!(condition)) {                                                        \
      Utility::AssertFailed(#condition, message, __FILE__, __LINE__);          \
    }                                                                          \
  } while (false)
#endif

#define AQUILA_ASSERT_SIMPLE(condition, message)                               \
  do {                                                                         \
    if (!(condition)) {                                                        \
      Utility::AssertFailed(#condition, message, __FILE__, __LINE__);          \
    }                                                                          \
  } while (false)

#define AQUILA_VULKAN_CHECK(call)                                              \
  do {                                                                         \
    VkResult result = (call);                                                  \
    if (AQUILA_UNLIKELY(result != VK_SUCCESS)) {                               \
      Utility::LogError(std::string("[VULKAN] Call failed: ") + #call +        \
                        " | File: " + __FILE__ +                               \
                        " | Line: " + std::to_string(__LINE__));               \
      Utility::AssertFailed(#call, "Vulkan call failed", __FILE__, __LINE__);  \
    }                                                                          \
  } while (0)
#ifndef AQUILA_DEFINES_H
#define AQUILA_DEFINES_H

#include "Utilities/Log.h"

#ifdef AQUILA_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <direct.h>
#include <intrin.h>
#include <sysinfoapi.h>
#include <windows.h>

#elif defined(AQUILA_PLATFORM_LINUX)
#include <sys/sysinfo.h>
#include <thread>
#include <unistd.h>

#elif defined(AQUILA_PLATFORM_MACOS)
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <unistd.h>

#endif

/* Enable asserts by default - @todo i should maybe change this or condition
 * it*/
#define AQUILA_ENABLE_ASSERTS

#define AQUILA_OUT(x) (std::cout << x << std::endl)

#define BIT(x) (1 << x)

#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#ifdef _DEBUG
#define AQUILA_DEBUG
#endif

#if defined(AQUILA_DEBUG)
#define AQUILA_ASSERT(condition, message)                                      \
  do {                                                                         \
    if (!(condition)) {                                                        \
      Aquila::AssertFailed(#condition, message, __FILE__, __LINE__);           \
    }                                                                          \
  } while (0)
#define AQUILA_LOG_DEBUG(message) Aquila::Log(message)
#else
#define AQUILA_ASSERT(condition, message) ((void)0)
#define AQUILA_LOG_DEBUG(message) ((void)0)
#endif

#define AQUILA_NONCOPYABLE(ClassName)                                          \
  ClassName(const ClassName &) = delete;                                       \
  ClassName &operator=(const ClassName &) = delete;

#define AQUILA_NONMOVEABLE(ClassName)                                          \
  ClassName(ClassName &&) = delete;                                            \
  ClassName &operator=(ClassName &&) = delete

#define AQUILA_VULKAN_CHECK(call)                                              \
  do {                                                                         \
    VkResult result = (call);                                                  \
    if (result != VK_SUCCESS) {                                                \
      Aquila::LogError(std::string("[VULKAN] Call failed: ") + #call +         \
                       " | File: " + __FILE__ +                                \
                       " | Line: " + std::to_string(__LINE__));                \
      Aquila::AssertFailed(#call, "Vulkan call failed", __FILE__, __LINE__);   \
    }                                                                          \
  } while (0)
#endif
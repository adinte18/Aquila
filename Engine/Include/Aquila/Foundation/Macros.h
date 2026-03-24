#ifndef AQUILA_MACROS_H
#define AQUILA_MACROS_H

#include "Aquila/Foundation/Log.h"

#define AQUILA_LOG_TRACE(...) Aquila::Foundation::LogTrace(__VA_ARGS__)
#define AQUILA_LOG_DEBUG(...) Aquila::Foundation::LogDebug(__VA_ARGS__)
#define AQUILA_LOG_INFO(...) Aquila::Foundation::LogInfo(__VA_ARGS__)
#define AQUILA_LOG_WARNING(...) Aquila::Foundation::LogWarning(__VA_ARGS__)
#define AQUILA_LOG_ERROR(...) Aquila::Foundation::LogError(__VA_ARGS__)
#define AQUILA_LOG_CRITICAL(...) Aquila::Foundation::LogCritical(__VA_ARGS__)

#if defined(AQUILA_DEBUG)
#define AQUILA_ASSERT(condition, message)                                              \
	do {                                                                               \
		if (AQUILA_UNLIKELY(!(condition))) {                                           \
			Aquila::Foundation::AssertFailed(#condition, message, __FILE__, __LINE__); \
		}                                                                              \
	} while (0)

#else
#define AQUILA_ASSERT(condition, message)                                              \
	do {                                                                               \
		if (!(condition)) {                                                            \
			Aquila::Foundation::AssertFailed(#condition, message, __FILE__, __LINE__); \
		}                                                                              \
	} while (false)
#endif

#define AQUILA_VULKAN_CHECK(call)                                                                                 \
	do {                                                                                                          \
		VkResult result = (call);                                                                                 \
		if (AQUILA_UNLIKELY(result != VK_SUCCESS)) {                                                              \
			Aquila::Foundation::LogError(std::string("[VULKAN] Call failed: ") + #call + " | File: " + __FILE__ + \
										 " | Line: " + std::to_string(__LINE__));                                 \
			Aquila::Foundation::AssertFailed(#call, "Vulkan call failed", __FILE__, __LINE__);                    \
		}                                                                                                         \
	} while (0)

#define AQUILA_UNIMPLEMENTED() AQUILA_LOG_WARNING("Not implemented: {}", __FUNCTION__)

#endif

#ifndef AQUILA_DEFINES_H
#define AQUILA_DEFINES_H

#ifdef AQUILA_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <intrin.h>
    #include <sysinfoapi.h>
    #include <direct.h>
#elif defined(AQUILA_PLATFORM_LINUX)
    #include <unistd.h>
    #include <sys/sysinfo.h>
    #include <thread>
#elif defined(AQUILA_PLATFORM_MACOS)
    #include <unistd.h>
    #include <sys/sysctl.h>
    #include <mach/mach.h>
#endif

/* Enable asserts by default - @todo i should maybe change this or condition it*/
#define AQUILA_ENABLE_ASSERTS

#define AQUILA_OUT(x) (std::cout << x << std::endl)

#define BIT(x) (1 << x)

#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#ifdef _DEBUG
    #define AQUILA_DEBUG
#endif

#if defined(AQUILA_DEBUG)
    #define AQUILA_ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                Core::Platform::Debug::AssertFailed(#condition, message, __FILE__, __LINE__); \
            } \
        } while(0)
    #define AQUILA_LOG_DEBUG(message) Core::Platform::Debug::Log(message)
#else
    #define AQUILA_ASSERT(condition, message) ((void)0)
    #define AQUILA_LOG_DEBUG(message) ((void)0)
#endif


#endif
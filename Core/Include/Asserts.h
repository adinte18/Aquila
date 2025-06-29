#ifndef AQUILA_ASSERTS_H
#define AQUILA_ASSERTS_H

#include <iostream>
#include <cstdlib>  // for abort()

// Cross-platform debug break
#if defined(_MSC_VER)
    #define AQUILA_DEBUG_BREAK() __debugbreak()
#elif defined(_WIN32)
    #include <intrin.h>
    #define AQUILA_DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
    #include <csignal>
    #define AQUILA_DEBUG_BREAK() raise(SIGTRAP)
#else
    #include <cstdlib>
    #define AQUILA_DEBUG_BREAK() std::abort()
#endif

// Runtime assertion for engine/internal code
#define AQUILA_CORE_ASSERT(x) \
    do { \
        if (!(x)) { \
            std::cerr << "Core Assertion Failed: " << #x << std::endl; \
            AQUILA_DEBUG_BREAK(); \
        } \
    } while(0)

#define AQUILA_STATIC_ASSERT(x, msg) static_assert((x), msg)


#endif // AQUILA_ASSERTS_H

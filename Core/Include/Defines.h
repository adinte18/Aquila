#ifndef AQUILA_DEFINES_H
#define AQUILA_DEFINES_H

/* Enable asserts by default - @todo i should maybe change this or condition it*/
#define AQUILA_ENABLE_ASSERTS

#ifdef _WIN32
    #define AQUILA_PLATFORM_WINDOWS
#else
    #error Aquila only supports Windows (for now)
#endif

#define AQUILA_OUT(x) (std::cout << x << std::endl)

#define BIT(x) (1 << x)

#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#endif
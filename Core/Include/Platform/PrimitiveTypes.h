#ifndef CORE_PLATFORM_TYPES_H
#define CORE_PLATFORM_TYPES_H

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <functional>
#include <string>
#include "SDL3/SDL.h"
#include "Defines.h"


using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using float32 = float;
using float64 = double;

using size_t = std::size_t;
using ptrdiff_t = std::ptrdiff_t;

template<typename T>
using Unique = std::unique_ptr<T>;

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T>
using WeakRef = std::weak_ptr<T>;

template<typename T>
using Delegate = std::function<T>;

template<typename T, typename... Args>
Unique<T> CreateUnique(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
Ref<T> CreateRef(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

enum class Result : uint32 {
    Success = 0,
    Failure = 1,
    InvalidParameter = 2,
    OutOfMemory = 3,
    NotImplemented = 4,
    NotSupported = 5,
    Timeout = 6
};

struct ThreadHandle {
    SDL_Thread* sdlThread;
};

struct MutexHandle {
    SDL_Mutex* sdlMutex;
};


#endif // CORE_PLATFORM_TYPES_H
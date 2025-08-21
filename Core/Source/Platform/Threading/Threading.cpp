#include "Platform/Threading/Threading.h"
#include "Platform/Platform.h"

namespace Threading {

    ThreadHandle* CreateThread(int (*function)(void*), const char* name, void* data) {
        ThreadHandle* handle = new ThreadHandle();
        handle->sdlThread = SDL_CreateThread(function, name, data);
        if (!handle->sdlThread) {
            Debug::LogError("Failed to create thread: " + std::string(SDL_GetError()));
            delete handle;
            return nullptr;
        }
        return handle;
    }

    void DetachThread(ThreadHandle* thread) {
        if (thread && thread->sdlThread) {
            SDL_DetachThread(thread->sdlThread);
            delete thread;
        }
    }

    void WaitThread(ThreadHandle* thread, int* status) {
        if (thread && thread->sdlThread) {
            SDL_WaitThread(thread->sdlThread, status);
            #ifdef AQUILA_DEBUG
                Debug::Log("Thread " + std::string(SDL_GetThreadName(thread->sdlThread)) + " finished with status: " + std::to_string(*status));
            #endif
            delete thread;
        }
    }

    MutexHandle* CreateMutex() {
        MutexHandle* handle = new MutexHandle();
        handle->sdlMutex = SDL_CreateMutex();
        if (!handle->sdlMutex) {
            Debug::LogError("Failed to create mutex: " + std::string(SDL_GetError()));
            delete handle;
            return nullptr;
        }
        return handle;
    }

    void LockMutex(MutexHandle* mutex) {
        if (mutex && mutex->sdlMutex) {
            SDL_LockMutex(mutex->sdlMutex);
        }
    }

    void UnlockMutex(MutexHandle* mutex) {
        if (mutex && mutex->sdlMutex) {
            SDL_UnlockMutex(mutex->sdlMutex);
        }
    }

    void DestroyMutex(MutexHandle* mutex) {
        if (mutex && mutex->sdlMutex) {
            SDL_DestroyMutex(mutex->sdlMutex);
            delete mutex;
        }
    }

    std::uint32_t GetCurrentThreadId() {
        return SDL_ThreadID();
    }

    std::uint32_t GetHardwareThreadCount() {
        return static_cast<std::uint32_t>(SDL_GetNumLogicalCPUCores());
    }

    void Sleep(std::uint32_t milliseconds) {
        SDL_Delay(milliseconds);
    }

}
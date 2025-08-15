#ifndef CORE_THREADING_H
#define CORE_THREADING_H

#include "Platform/Platform.h"
#include "SDL3/SDL_thread.h"

namespace Threading {
    ThreadHandle* CreateThread(int (*function)(void*), const char* name, void* data = nullptr);
    void DetachThread(ThreadHandle* thread);
    void WaitThread(ThreadHandle* thread, int* status = nullptr);

    MutexHandle* CreateMutex();
    void LockMutex(MutexHandle* mutex);
    void UnlockMutex(MutexHandle* mutex);
    void DestroyMutex(MutexHandle* mutex);

    uint32_t GetCurrentThreadId();
    uint32_t GetHardwareThreadCount();
    void Sleep(uint32_t milliseconds);
    void YieldThread();
}

#endif // CORE_THREADING_H
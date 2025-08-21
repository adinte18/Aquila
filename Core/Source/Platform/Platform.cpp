#include "Platform/Platform.h"

#include <iostream>



namespace Core::Platform {

    static PlatformSpec s_PlatformInfo = {};
    static bool s_Initialized = false;

    bool Initialize() {
        if (s_Initialized) {
            return true;
        }

        // Initialize platform info
        #ifdef AQUILA_PLATFORM_WINDOWS
            s_PlatformInfo.name = "Windows";
            
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            s_PlatformInfo.cpuCores = static_cast<int>(sysInfo.dwNumberOfProcessors);
            
            MEMORYSTATUSEX memInfo;
            memInfo.dwLength = sizeof(MEMORYSTATUSEX);
            GlobalMemoryStatusEx(&memInfo);
            s_PlatformInfo.totalMemory = static_cast<std::size_t>(memInfo.ullTotalPhys);

            #ifdef AQUILA_PLATFORM_64BIT
                s_PlatformInfo.is64Bit = true;
                s_PlatformInfo.version = "64-bit";
            #else
                s_PlatformInfo.is64Bit = false;
                s_PlatformInfo.version = "32-bit";
            #endif

        #elif defined(AQUILA_PLATFORM_LINUX)
            s_PlatformInfo.name = "Linux";
            s_PlatformInfo.cpuCores = static_cast<int>(std::thread::hardware_concurrency());
            
            struct sysinfo info;
            if (sysinfo(&info) == 0) {
                s_PlatformInfo.totalMemory = static_cast<std::size_t>(info.totalram * info.mem_unit);
            }
            
            #ifdef PLATFORM_64BIT
                s_PlatformInfo.is64Bit = true;
                s_PlatformInfo.version = "64-bit";
            #else
                s_PlatformInfo.is64Bit = false;
                s_PlatformInfo.version = "32-bit";
            #endif

        #elif defined(AQUILA_PLATFORM_MACOS)
            s_PlatformInfo.name = "macOS";
            s_PlatformInfo.cpuCores = static_cast<int>(std::thread::hardware_concurrency());
            
            int64_t memsize;
            size_t size = sizeof(memsize);
            if (sysctlbyname("hw.memsize", &memsize, &size, nullptr, 0) == 0) {
                s_PlatformInfo.totalMemory = static_cast<std::size_t>(memsize);
            }
            
            s_PlatformInfo.is64Bit = true;
            s_PlatformInfo.version = "64-bit";
        #endif

        s_Initialized = true;
        Debug::Log("Platform layer initialized successfully");
        return true;
    }

    const PlatformSpec& GetPlatformInfo() {
        return s_PlatformInfo;
    }

    void Shutdown() {
        if (!s_Initialized) {
            return;
        }

        s_Initialized = false;
        Debug::Log("Platform layer shutdown successfully");
    }

    std::uint64_t GetHighResolutionTime() {
        #ifdef AQUILA_PLATFORM_WINDOWS
            LARGE_INTEGER counter;
            QueryPerformanceCounter(&counter);
            return static_cast<std::uint64_t>(counter.QuadPart);
        #else
            auto now = std::chrono::high_resolution_clock::now();
            return static_cast<std::uint64_t>(now.time_since_epoch().count());
        #endif
    }

    double GetTimeFrequency() {
        #ifdef AQUILA_PLATFORM_WINDOWS
            static double frequency = 0.0;
            if (frequency == 0.0) {
                LARGE_INTEGER freq;
                QueryPerformanceFrequency(&freq);
                frequency = static_cast<double>(freq.QuadPart);
            }
            return frequency;
        #else
            return static_cast<double>(std::chrono::high_resolution_clock::period::den) /
                   static_cast<double>(std::chrono::high_resolution_clock::period::num);
        #endif
    }
}


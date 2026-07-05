#include "Aquila/Platform/Platform.h"

namespace Aquila::Platform {

static PlatformSpec s_PlatformInfo = {};
static bool s_Initialized = false;

bool initialize() {
	if (s_Initialized) {
		return true;
	}

// Initialize platform info
#ifdef AQUILA_PLATFORM_WINDOWS
	s_PlatformInfo.name = "Windows";

	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	s_PlatformInfo.cpu_cores = static_cast<int>(sys_info.dwNumberOfProcessors);

	MEMORYSTATUSEX mem_info;
	mem_info.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&mem_info);
	s_PlatformInfo.total_memory = static_cast<std::size_t>(mem_info.ullTotalPhys);

#ifdef AQUILA_PLATFORM_64BIT
	s_PlatformInfo.is64_bit = true;
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
	return true;
}

void shutdown() {
	if (!s_Initialized) {
		return;
	}
	s_Initialized = false;
}

const PlatformSpec &get_platform_info() {
	return s_PlatformInfo;
}

} // namespace Aquila::Platform

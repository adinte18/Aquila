#ifndef CORE_PLATFORM_H
#define CORE_PLATFORM_H

#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Platform {

struct PlatformSpec {
	const char *name;
	const char *version;
	bool is64Bit;
	int cpuCores;
	usize totalMemory;
};

bool Initialize();
void Shutdown();

const PlatformSpec &GetPlatformInfo();

} // namespace Aquila::Platform

#endif // CORE_PLATFORM_H

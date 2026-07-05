#ifndef CORE_PLATFORM_H
#define CORE_PLATFORM_H

#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Platform {

struct PlatformSpec {
	const char *name;
	const char *version;
	bool is64_bit;
	int cpu_cores;
	Usize total_memory;
};

bool initialize();
void shutdown();

const PlatformSpec &get_platform_info();

} // namespace Aquila::Platform

#endif // CORE_PLATFORM_H

#ifndef CORE_PLATFORM_H
#define CORE_PLATFORM_H

// Debug macros

#include "PrimitiveTypes.h"
#include "Utilities/Log.h"

namespace Core::Platform {

struct PlatformSpec {
  const char *name;
  const char *version;
  bool is64Bit;
  int cpuCores;
  size_t totalMemory;
};

bool Initialize();
void Shutdown();

const PlatformSpec &GetPlatformInfo();

uint64_t GetHighResolutionTime();
double GetTimeFrequency();
} // namespace Core::Platform

#endif // CORE_PLATFORM_H
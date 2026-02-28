#ifndef AQUILA_PCH_H
#define AQUILA_PCH_H

#ifdef AQUILA_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <direct.h>
#include <intrin.h>
#include <sysinfoapi.h>
#include <windows.h>
#include <shellapi.h>

#elif defined(AQUILA_PLATFORM_LINUX)
#include <csignal>
#include <sys/sysinfo.h>
#include <unistd.h>

#elif defined(AQUILA_PLATFORM_MACOS)
#include <csignal>
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <unistd.h>

#endif

#include <any>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include <array>
#include <deque>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

#include <algorithm>
#include <iterator>
#include <numeric>

#include <type_traits>
#include <typeindex>
#include <typeinfo>

#include <cmath>
#include <limits>
#include <random>

#include <chrono>

#include <concepts>
#include <format>
#include <ranges>
#include <source_location>
#include <span>

#include <cctype>
#include <cstring>
#include <inttypes.h>

#include <exception>
#include <stdexcept>
#include <system_error>

#include "vulkan/vulkan.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#endif // AQUILA_PCH_H

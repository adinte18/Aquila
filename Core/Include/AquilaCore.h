#ifndef AQUILA_CORE_H
#define AQUILA_CORE_H

#include "vulkan/vulkan.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <cassert>
#include <iostream>
#include <memory>
#include <utility>
#include <functional>
#include <string>
#include <vector>
#include <filesystem>
#include <variant>
#include <mutex>
#include <typeindex>
#include <fstream>
#include <inttypes.h>
#include <optional>
#include <set>
#include <random>
#include <cstdint>
#include <iomanip>
#include <sstream>

#include "Defines.h"
#include "Asserts.h"


#include "Platform/Platform.h"
#include "Platform/Timer.h"
#include "Platform/DebugLog.h"
#include "Platform/Threading/Threading.h"
#include "Platform/Filesystem/Filesystem.h"

struct UUID {
    uint64_t high;
    uint64_t low;

    static UUID Generate() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dist;

        UUID uuid;
        uuid.high = dist(gen);
        uuid.low = dist(gen);

        // enforce UUID version 4 bits
        uuid.high &= 0xFFFFFFFFFFFF0FFFULL;
        uuid.high |= 0x0000000000004000ULL;
        uuid.low &= 0x3FFFFFFFFFFFFFFFULL;
        uuid.low |= 0x8000000000000000ULL;

        return uuid;
    }

    static UUID Null() {
        return UUID{0, 0};
    }

    std::string ToString() const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0')
            << std::setw(8) << (uint32_t)(high >> 32) << "-"
            << std::setw(4) << (uint16_t)(high >> 16) << "-"
            << std::setw(4) << (uint16_t)high << "-"
            << std::setw(4) << (uint16_t)(low >> 48) << "-"
            << std::setw(12) << (low & 0x0000FFFFFFFFFFFFULL);
        return oss.str();
    }

    static UUID FromString(const std::string& str) {
        UUID uuid{};
        uint32_t data1;
        uint16_t data2, data3;
        uint16_t data4;
        uint64_t data5;


        #if defined(AQUILA_PLATFORM_WINDOWS)
            std::sscanf(str.c_str(),
                        "%8x-%4hx-%4hx-%4hx-%12llx",
                        &data1, &data2, &data3, &data4, &data5);
        #elif defined(AQUILA_PLATFORM_LINUX)
            std::sscanf(str.c_str(),
                        "%8x-%4hx-%4hx-%4hx-%12" SCNx64,
                        &data1, &data2, &data3, &data4, &data5);
        #endif

        uuid.high = (static_cast<uint64_t>(data1) << 32)
                | (static_cast<uint64_t>(data2) << 16)
                | data3;

        uuid.low = (static_cast<uint64_t>(data4) << 48)
                | (data5 & 0x0000FFFFFFFFFFFFULL);

        return uuid;
    }


    bool operator==(const UUID& other) const {
        return high == other.high && low == other.low;
    }
};

namespace std {
    template<>
    struct hash<UUID> {
        std::size_t operator()(const UUID& uuid) const noexcept {
            std::size_t h1 = std::hash<uint64_t>{}(uuid.high);
            std::size_t h2 = std::hash<uint64_t>{}(uuid.low);
            return h1 ^ (h2 << 1);
        }
    };
}



#endif //AQUILA_CORE_H

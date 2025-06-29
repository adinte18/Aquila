#ifndef AQUILA_CORE_H
#define AQUILA_CORE_H

#include "vulkan/vulkan.h"

// Timing
#include <chrono>

// Maths includes
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

// Base includes
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

// Aquila core utilities
#include "Defines.h"
#include "Asserts.h"

template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T>
using Unique = std::unique_ptr<T>;

template<typename T>
using WeakRef = std::weak_ptr<T>;

template<typename T>
using Delegate = std::function<T>;

#include <random>
#include <cstdint>
#include <iomanip>
#include <sstream>

struct DeltaTime {
    float Seconds = 0.0f;

    explicit DeltaTime(float time = 0.0f) : Seconds(time) {}

    operator float() const { return Seconds; }  // implicit conversion to float
};


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

        std::sscanf(str.c_str(),
                    "%8x-%4hx-%4hx-%4hx-%12llx",
                    &data1, &data2, &data3, &data4, &data5);

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


#endif //AQUILA_CORE_H

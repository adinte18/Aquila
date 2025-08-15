#include "Platform/DebugLog.h"

namespace Debug {
    void Log(const char* message) {
        std::cout << "[AQUILA] " << message << std::endl;
    }

    void Log(const std::string& message) {
        std::cout << "[AQUILA] " << message << std::endl;
    }

    void LogError(const char* message) {
        std::cerr << "[AQUILA ERROR] " << message << std::endl;
    }

    void LogError(const std::string& message) {
        std::cerr << "[AQUILA ERROR] " << message << std::endl;
    }

    void AssertFailed(const char* condition, const char* message, const char* file, int line) {
        std::cerr << "ASSERTION FAILED: " << condition << std::endl;
        std::cerr << "Message: " << message << std::endl;
        std::cerr << "File: " << file << ":" << line << std::endl;
                
        std::terminate();
    }
}
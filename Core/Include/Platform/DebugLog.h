#ifndef LOG_H
#define LOG_H

#include <string>
#include <iostream>

namespace Debug {
    void Log(const char* message);
    void Log(const std::string& message);
    void LogError(const char* message);
    void LogError(const std::string& message);
    void AssertFailed(const char* condition, const char* message, const char* file, int line);
}


#endif // LOG_H
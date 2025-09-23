#ifndef AQUILA_LOG_H
#define AQUILA_LOG_H

#include <format>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>

namespace Utility {

enum class LogLevel {
  Trace = 0,
  Debug = 1,
  Info = 2,
  Warning = 3,
  Error = 4,
  Critical = 5
};

class Logger {
private:
  static LogLevel s_currentLevel;
  static bool s_showTimestamp;
  static bool s_showLocation;

public:
  static void SetLogLevel(LogLevel level) { s_currentLevel = level; }
  static LogLevel GetLogLevel() { return s_currentLevel; }

  static void EnableTimestamp(bool enable = true) { s_showTimestamp = enable; }
  static void EnableLocation(bool enable = true) { s_showLocation = enable; }

private:
  static std::string GetTimestamp();
  static std::string GetLevelString(LogLevel level);
  static std::string FormatLocation(const std::source_location &location);

  template <typename... Args>
  static void LogImplInternal(LogLevel level, std::string_view format_str,
                              const std::source_location &location,
                              Args &&...args) {
    if (level < s_currentLevel)
      return;

    std::string message;
    if constexpr (sizeof...(args) > 0) {
      message = std::vformat(format_str, std::make_format_args(args...));
    } else {
      message = std::string{format_str};
    }

    std::string prefix = std::format("[AQUILA {}]", GetLevelString(level));

    if (s_showTimestamp) {
      prefix = std::format("{} {}", GetTimestamp(), prefix);
    }

    if (s_showLocation && level >= LogLevel::Warning) {
      prefix = std::format("{} {}", prefix, FormatLocation(location));
    }

    auto &stream = (level >= LogLevel::Error) ? std::cerr : std::cout;
    stream << std::format("{} {}\n", prefix, message);
    stream.flush();
  }

public:
  template <typename... Args>
  static void LogTrace(
      std::string_view format_str, Args &&...args,
      const std::source_location &location = std::source_location::current()) {
    LogImplInternal(LogLevel::Trace, format_str, location,
                    std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void LogDebug(
      std::string_view format_str, Args &&...args,
      const std::source_location &location = std::source_location::current()) {
    LogImplInternal(LogLevel::Debug, format_str, location,
                    std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void LogInfo(
      std::string_view format_str, Args &&...args,
      const std::source_location &location = std::source_location::current()) {
    LogImplInternal(LogLevel::Info, format_str, location,
                    std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void LogWarning(
      std::string_view format_str, Args &&...args,
      const std::source_location &location = std::source_location::current()) {
    LogImplInternal(LogLevel::Warning, format_str, location,
                    std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void LogError(
      std::string_view format_str, Args &&...args,
      const std::source_location &location = std::source_location::current()) {
    LogImplInternal(LogLevel::Error, format_str, location,
                    std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void LogCritical(
      std::string_view format_str, Args &&...args,
      const std::source_location &location = std::source_location::current()) {
    LogImplInternal(LogLevel::Critical, format_str, location,
                    std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void SimpleLogTrace(std::string_view format_str, Args &&...args) {
    LogImplInternal(LogLevel::Trace, format_str,
                    std::source_location::current(),
                    std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void SimpleLogDebug(std::string_view format_str, Args &&...args) {
    LogImplInternal(LogLevel::Debug, format_str,
                    std::source_location::current(),
                    std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void SimpleLogInfo(std::string_view format_str, Args &&...args) {
    LogImplInternal(LogLevel::Info, format_str, std::source_location::current(),
                    std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void SimpleLogWarning(std::string_view format_str, Args &&...args) {
    LogImplInternal(LogLevel::Warning, format_str,
                    std::source_location::current(),
                    std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void SimpleLogError(std::string_view format_str, Args &&...args) {
    LogImplInternal(LogLevel::Error, format_str,
                    std::source_location::current(),
                    std::forward<Args>(args)...);
  }

  template <typename... Args>
  static void SimpleLogCritical(std::string_view format_str, Args &&...args) {
    LogImplInternal(LogLevel::Critical, format_str,
                    std::source_location::current(),
                    std::forward<Args>(args)...);
  }
};

template <typename... Args>
void Log(std::string_view format_str, Args &&...args) {
  Logger::SimpleLogInfo(format_str, std::forward<Args>(args)...);
}

template <typename... Args>
void LogError(std::string_view format_str, Args &&...args) {
  Logger::SimpleLogError(format_str, std::forward<Args>(args)...);
}

template <typename... Args>
void LogWarning(std::string_view format_str, Args &&...args) {
  Logger::SimpleLogWarning(format_str, std::forward<Args>(args)...);
}

template <typename... Args>
void LogTrace(std::string_view format_str, Args &&...args) {
  Logger::SimpleLogTrace(format_str, std::forward<Args>(args)...);
}

template <typename... Args>
void LogDebug(std::string_view format_str, Args &&...args) {
  Logger::SimpleLogDebug(format_str, std::forward<Args>(args)...);
}

template <typename... Args>
void LogCritical(std::string_view format_str, Args &&...args) {
  Logger::SimpleLogCritical(format_str, std::forward<Args>(args)...);
}

template <typename... Args>
void LogInfo(std::string_view format_str, Args &&...args) {
  Logger::SimpleLogInfo(format_str, std::forward<Args>(args)...);
}

template <typename... Args>
[[noreturn]] void AssertFailed(std::string_view condition,
                               std::string_view format_str, Args &&...args,
                               const std::source_location &location) {
  std::string message;
  if constexpr (sizeof...(args) > 0) {
    message = std::format(format_str, std::forward<Args>(args)...);
  } else {
    message = std::string{format_str};
  }

  std::cerr << std::format("ASSERTION FAILED: {}\n", condition);
  std::cerr << std::format("Message: {}\n", message);
  std::cerr << std::format("File: {}:{} in {}\n", location.file_name(),
                           location.line(), location.function_name());

  std::terminate();
}

[[noreturn]] inline void AssertFailed(const char *condition,
                                      const char *message, const char *file,
                                      int line) {
  std::cerr << std::format("ASSERTION FAILED: {}\n", condition);
  std::cerr << std::format("Message: {}\n", message);
  std::cerr << std::format("File: {}:{}\n", file, line);
  std::terminate();
}

} // namespace Utility

#endif
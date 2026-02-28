#ifndef AQUILA_LOG_H
#define AQUILA_LOG_H

#include <format>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>

namespace Aquila::Utils {

enum class LogLevel {
	Trace = 1 << 2,
	Debug = 1 << 3,
	Info = 1 << 4,
	Warning = 1 << 5,
	Error = 1 << 6,
	Critical = 1 << 7
};

namespace Color {
constexpr const char *Reset = "\033[0m";
constexpr const char *Bold = "\033[1m";
constexpr const char *Dim = "\033[2m";

constexpr const char *Black = "\033[30m";
constexpr const char *Red = "\033[31m";
constexpr const char *Green = "\033[32m";
constexpr const char *Yellow = "\033[33m";
constexpr const char *Blue = "\033[34m";
constexpr const char *Magenta = "\033[35m";
constexpr const char *Cyan = "\033[36m";
constexpr const char *White = "\033[37m";

constexpr const char *BrightBlack = "\033[90m";
constexpr const char *BrightRed = "\033[91m";
constexpr const char *BrightGreen = "\033[92m";
constexpr const char *BrightYellow = "\033[93m";
constexpr const char *BrightBlue = "\033[94m";
constexpr const char *BrightMagenta = "\033[95m";
constexpr const char *BrightCyan = "\033[96m";
constexpr const char *BrightWhite = "\033[97m";
} // namespace Color

class Logger {
  private:
	static LogLevel s_currentLevel;
	static bool s_showTimestamp;
	static bool s_showLocation;
	static bool s_useColors;

  public:
	static void SetLogLevel(LogLevel level) { s_currentLevel = level; }
	static LogLevel GetLogLevel() { return s_currentLevel; }

	static void EnableTimestamp(bool enable = true) { s_showTimestamp = enable; }
	static void EnableLocation(bool enable = true) { s_showLocation = enable; }
	static void EnableColors(bool enable = true) { s_useColors = enable; }

  private:
	static std::string GetTimestamp();
	static std::string GetLevelString(LogLevel level);
	static std::string FormatLocation(const std::source_location &location);

	static const char *GetLevelColor(LogLevel level) {
		if (!s_useColors) {
			return "";
		}

		switch (level) {
		case LogLevel::Trace:
			return Color::BrightBlack;
		case LogLevel::Debug:
			return Color::Cyan;
		case LogLevel::Info:
			return Color::BrightGreen;
		case LogLevel::Warning:
			return Color::BrightYellow;
		case LogLevel::Error:
			return Color::BrightRed;
		case LogLevel::Critical:
			static const std::string criticalColor = std::format("{}{}", Color::Bold, Color::BrightRed);
			return criticalColor.c_str();
		default:
			return Color::Reset;
		}
	}

	template <typename... Args>
	static void LogImplInternal(LogLevel level, std::string_view format_str, const std::source_location &location,
								Args &&...args) {
		if (level < s_currentLevel) {
			return;
		}

		std::string message;
		if constexpr (sizeof...(args) > 0) {
			message = std::vformat(format_str, std::make_format_args(args...));
		} else {
			message = std::string{ format_str };
		}

		const char *color = GetLevelColor(level);
		const char *reset = s_useColors ? Color::Reset : "";
		const char *dimColor = s_useColors ? Color::Dim : "";

		std::string prefix = std::format("{}[AQUILA {}]{}", color, GetLevelString(level), reset);

		if (s_showTimestamp) {
			prefix = std::format("{}{}{} {}", dimColor, GetTimestamp(), reset, prefix);
		}

		if (s_showLocation && level >= LogLevel::Warning) {
			prefix = std::format("{} {}{}{}", prefix, dimColor, FormatLocation(location), reset);
		}

		auto &stream = (level >= LogLevel::Error) ? std::cerr : std::cout;
		stream << std::format("{} {}\n", prefix, message);
		stream.flush();
	}

  public:
	template <typename... Args>
	static void LogTrace(std::string_view format_str, Args &&...args,
						 const std::source_location &location = std::source_location::current()) {
		LogImplInternal(LogLevel::Trace, format_str, location, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void LogDebug(std::string_view format_str, Args &&...args,
						 const std::source_location &location = std::source_location::current()) {
		LogImplInternal(LogLevel::Debug, format_str, location, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void LogInfo(std::string_view format_str, Args &&...args,
						const std::source_location &location = std::source_location::current()) {
		LogImplInternal(LogLevel::Info, format_str, location, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void LogWarning(std::string_view format_str, Args &&...args,
						   const std::source_location &location = std::source_location::current()) {
		LogImplInternal(LogLevel::Warning, format_str, location, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void LogError(std::string_view format_str, Args &&...args,
						 const std::source_location &location = std::source_location::current()) {
		LogImplInternal(LogLevel::Error, format_str, location, std::forward<Args>(args)...);
	}

	template <typename... Args>
	static void LogCritical(std::string_view format_str, Args &&...args,
							const std::source_location &location = std::source_location::current()) {
		LogImplInternal(LogLevel::Critical, format_str, location, std::forward<Args>(args)...);
	}

	template <typename... Args> static void SimpleLogTrace(std::string_view format_str, Args &&...args) {
		LogImplInternal(LogLevel::Trace, format_str, std::source_location::current(), std::forward<Args>(args)...);
	}

	template <typename... Args> static void SimpleLogDebug(std::string_view format_str, Args &&...args) {
		LogImplInternal(LogLevel::Debug, format_str, std::source_location::current(), std::forward<Args>(args)...);
	}

	template <typename... Args> static void SimpleLogInfo(std::string_view format_str, Args &&...args) {
		LogImplInternal(LogLevel::Info, format_str, std::source_location::current(), std::forward<Args>(args)...);
	}

	template <typename... Args> static void SimpleLogWarning(std::string_view format_str, Args &&...args) {
		LogImplInternal(LogLevel::Warning, format_str, std::source_location::current(), std::forward<Args>(args)...);
	}

	template <typename... Args> static void SimpleLogError(std::string_view format_str, Args &&...args) {
		LogImplInternal(LogLevel::Error, format_str, std::source_location::current(), std::forward<Args>(args)...);
	}

	template <typename... Args> static void SimpleLogCritical(std::string_view format_str, Args &&...args) {
		LogImplInternal(LogLevel::Critical, format_str, std::source_location::current(), std::forward<Args>(args)...);
	}
};

template <typename... Args> void Log(std::string_view format_str, Args &&...args) {
	Logger::SimpleLogInfo(format_str, std::forward<Args>(args)...);
}

template <typename... Args> void LogError(std::string_view format_str, Args &&...args) {
	Logger::SimpleLogError(format_str, std::forward<Args>(args)...);
}

template <typename... Args> void LogWarning(std::string_view format_str, Args &&...args) {
	Logger::SimpleLogWarning(format_str, std::forward<Args>(args)...);
}

template <typename... Args> void LogTrace(std::string_view format_str, Args &&...args) {
	Logger::SimpleLogTrace(format_str, std::forward<Args>(args)...);
}

template <typename... Args> void LogDebug(std::string_view format_str, Args &&...args) {
	Logger::SimpleLogDebug(format_str, std::forward<Args>(args)...);
}

template <typename... Args> void LogCritical(std::string_view format_str, Args &&...args) {
	Logger::SimpleLogCritical(format_str, std::forward<Args>(args)...);
}

template <typename... Args> void LogInfo(std::string_view format_str, Args &&...args) {
	Logger::SimpleLogInfo(format_str, std::forward<Args>(args)...);
}

template <typename... Args>
[[noreturn]] void AssertFailed(std::string_view condition, std::string_view format_str, Args &&...args,
							   const std::source_location &location) {
	std::string message;
	if constexpr (sizeof...(args) > 0) {
		message = std::format(format_str, std::forward<Args>(args)...);
	} else {
		message = std::string{ format_str };
	}

	std::cerr << std::format("{}{}ASSERTION FAILED: {}{}\n", Color::Bold, Color::BrightRed, condition, Color::Reset);
	std::cerr << std::format("{}Message: {}{}\n", Color::BrightYellow, message, Color::Reset);
	std::cerr << std::format("{}File: {}:{} in {}{}\n", Color::Dim, location.file_name(), location.line(),
							 location.function_name(), Color::Reset);

	std::terminate();
}

[[noreturn]] inline void AssertFailed(const char *condition, const char *message, const char *file, int line) {
	std::cerr << std::format("{}{}ASSERTION FAILED: {}{}\n", Color::Bold, Color::BrightRed, condition, Color::Reset);
	std::cerr << std::format("{}Message: {}{}\n", Color::BrightYellow, message, Color::Reset);
	std::cerr << std::format("{}File: {}:{}{}\n", Color::Dim, file, line, Color::Reset);
	std::terminate();
}

} // namespace Aquila::Utils

#endif

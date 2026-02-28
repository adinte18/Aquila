#include "Aquila/Utilities/Log.h"

namespace Aquila::Utils {

#ifdef AQUILA_DEBUG
LogLevel Logger::s_currentLevel = LogLevel::Debug;
#else
LogLevel Logger::s_currentLevel = LogLevel::Info;
#endif
bool Logger::s_showTimestamp = true;
bool Logger::s_showLocation = false;
bool Logger::s_useColors = true;

std::string Logger::GetTimestamp() {
	auto now = std::chrono::system_clock::now();
	auto time_t = std::chrono::system_clock::to_time_t(now);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

	std::stringstream ss;
	ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
	ss << std::format(".{:03d}", ms.count());
	return ss.str();
}

std::string Logger::GetLevelString(LogLevel level) {
	switch (level) {
	case LogLevel::Trace:
		return "TRACE";
	case LogLevel::Debug:
		return "DEBUG";
	case LogLevel::Info:
		return "INFO";
	case LogLevel::Warning:
		return "WARNING";
	case LogLevel::Error:
		return "ERROR";
	case LogLevel::Critical:
		return "CRITICAL";
	default:
		return "UNKNOWN";
	}
}

std::string Logger::FormatLocation(const std::source_location &location) {
	std::string_view file_path = location.file_name();
	auto last_slash = file_path.find_last_of("/\\");
	std::string_view filename = (last_slash != std::string_view::npos) ? file_path.substr(last_slash + 1) : file_path;

	return std::format("({}:{})", filename, location.line());
}

} // namespace Aquila::Utils
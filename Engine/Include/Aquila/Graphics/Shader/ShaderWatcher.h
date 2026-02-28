#ifndef AQUILA_SHADER_WATCHER_H
#define AQUILA_SHADER_WATCHER_H

#include "Aquila/Core/AquilaCore.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

#include <filesystem>

namespace Aquila::Graphics::Shader {

struct WatchedShader {
	std::string slangPath; // original path as supplied (VFS or native)
	std::string programName;
	uint64 lastModified = 0;
	bool isNative = false; // true = bypass VFS, use std::filesystem directly
};

class ShaderWatcher {
  public:
	ShaderWatcher() = default;

	void Enable(bool enable) { m_Enabled = enable; }
	bool IsEnabled() const { return m_Enabled; }

	// Accepts either:
	//   - A VFS virtual path  (e.g. "assets://Shaders/GBuffer.slang")
	//   - A native absolute path (e.g. "C:/Programming/Aquila/Engine/Shaders/GBuffer.slang")
	void WatchSlangFile(const std::string &slangPath, const std::string &programName) {
		bool native = IsNativePath(slangPath);

		if (native) {
			if (!std::filesystem::exists(slangPath)) {
				AQUILA_LOG_WARNING("ShaderWatcher: cannot watch non-existent file '{}'", slangPath);
				return;
			}
			uint64 lastWrite = NativeLastWriteTime(slangPath);
			m_WatchedFiles[slangPath] = { .slangPath = slangPath,
										  .programName = programName,
										  .lastModified = lastWrite,
										  .isNative = true };
		} else {
			auto *vfs = Platform::Filesystem::VirtualFileSystem::Get();
			if (!vfs->Exists(slangPath)) {
				AQUILA_LOG_WARNING("ShaderWatcher: cannot watch non-existent VFS file '{}'", slangPath);
				return;
			}
			m_WatchedFiles[slangPath] = { .slangPath = slangPath,
										  .programName = programName,
										  .lastModified = vfs->GetLastWriteTime(slangPath),
										  .isNative = false };
		}

		AQUILA_LOG_INFO("ShaderWatcher: watching '{}' ({}) -> program '{}'", slangPath, native ? "native" : "VFS",
						programName);
	}

	void Unwatch(const std::string &slangPath) { m_WatchedFiles.erase(slangPath); }
	void Clear() { m_WatchedFiles.clear(); }

	// Returns the set of program names that need to be reloaded.
	std::unordered_set<std::string> CheckForChanges() {
		if (!m_Enabled) {
			return {};
		}

		std::unordered_set<std::string> changed;
		auto *vfs = Platform::Filesystem::VirtualFileSystem::Get();

		for (auto &[path, watch] : m_WatchedFiles) {
			uint64 current = 0;

			if (watch.isNative) {
				if (!std::filesystem::exists(path)) {
					continue;
				}
				current = NativeLastWriteTime(path);
			} else {
				if (!vfs->Exists(path)) {
					continue;
				}
				current = vfs->GetLastWriteTime(path);
			}

			if (current > watch.lastModified) {
				AQUILA_LOG_INFO("ShaderWatcher: '{}' modified, queuing reload of program '{}'", path,
								watch.programName);
				watch.lastModified = current;
				changed.insert(watch.programName);
			}
		}

		return changed;
	}

  private:
	// A path is treated as native if it is an absolute filesystem path
	// (starts with a drive letter on Windows, or '/' on Unix) and does NOT
	// contain "://" which is the VFS scheme separator.
	static bool IsNativePath(const std::string &path) {
		if (path.find("://") != std::string::npos)
			return false; // VFS virtual path

		// std::filesystem considers "C:\..." and "/home/..." as absolute
		return std::filesystem::path(path).is_absolute();
	}

	static uint64 NativeLastWriteTime(const std::string &path) {
		std::error_code ec;
		auto ftime = std::filesystem::last_write_time(path, ec);
		if (ec)
			return 0;
		// Convert to a plain integer comparable across calls
		return static_cast<uint64>(ftime.time_since_epoch().count());
	}

	std::unordered_map<std::string, WatchedShader> m_WatchedFiles;
	bool m_Enabled = false;
};

} // namespace Aquila::Graphics::Shader
#endif // AQUILA_SHADER_WATCHER_H

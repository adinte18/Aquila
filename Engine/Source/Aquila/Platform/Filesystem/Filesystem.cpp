#include "Aquila/Platform/Filesystem/Filesystem.h"
#include "Aquila/Foundation/Macros.h"

#include <algorithm>

namespace Aquila::Platform::Filesystem {
using std::string;

bool FileExists(const std::string &path) {
#ifdef AQUILA_PLATFORM_WINDOWS
	const DWORD attr = GetFileAttributesA(path.c_str());
	return (attr != INVALID_FILE_ATTRIBUTES);
#elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
	struct ::stat buffer;
	return (::stat(path.c_str(), &buffer) == 0);
#endif
}

FileStatInfo FileStat_(const std::string &path) {
	FileStatInfo result;
#ifdef AQUILA_PLATFORM_WINDOWS
	WIN32_FILE_ATTRIBUTE_DATA data;
	if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &data) != 0) {
		result.exists = true;
		result.size = (static_cast<uint64>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
		result.isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		result.isRegularFile = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;

		ULARGE_INTEGER ull;
		ull.LowPart = data.ftLastWriteTime.dwLowDateTime;
		ull.HighPart = data.ftLastWriteTime.dwHighDateTime;
		result.lastWriteTime = static_cast<uint32>((ull.QuadPart - 116444736000000000ULL) / 10000000ULL);
	}
#elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
	struct ::stat buffer;
	if (::stat(path.c_str(), &buffer) == 0) {
		result.exists = true;
		result.size = static_cast<uint32>(buffer.st_size);
		result.isDirectory = S_ISDIR(buffer.st_mode);
		result.isRegularFile = S_ISREG(buffer.st_mode);
		result.lastWriteTime = static_cast<uint32>(buffer.st_mtime);
	}
#endif
	return result;
}

bool PathIsAbsolute(const std::string &path) {
#ifdef AQUILA_PLATFORM_WINDOWS
	return (path.size() > 1 && path[1] == ':');
#elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
	return (!path.empty() && path[0] == '/');
#endif
}

std::string PathJoin(const std::string &a, const std::string &b) {
	if (a.empty()) {
		return b;
	}
	if (b.empty()) {
		return a;
	}

	// Strip leading separator from b to avoid double separators
	const std::string &bClean = (b[0] == '/' || b[0] == '\\') ? b.substr(1) : b;

	const bool trailed = (a.back() == '/' || a.back() == '\\');
	return trailed ? a + bClean : a + '/' + bClean;
}

std::string PathNormalize(const std::string &path) {
	std::string fwd = path;
	std::ranges::replace(fwd, '\\', '/');

	// Collapse double slashes, resolve . and ..
	std::vector<std::string> parts;
	std::istringstream ss(fwd);
	std::string token;
	const bool absolute = (!fwd.empty() && fwd[0] == '/');
	while (std::getline(ss, token, '/')) {
		if (token.empty() || token == ".") {
			continue;
		}
		if (token == "..") {
			if (!parts.empty()) {
				parts.pop_back();
			}
		} else {
			parts.push_back(token);
		}
	}
	std::string out = absolute ? "/" : "";
	for (size_t i = 0; i < parts.size(); ++i) {
		out += parts[i];
		if (i + 1 < parts.size()) {
			out += '/';
		}
	}
	return out.empty() ? (absolute ? "/" : ".") : out;
}

std::string DirGetCurrent() {
#ifdef AQUILA_PLATFORM_WINDOWS
	wchar_t buffer[MAX_PATH];
	if (GetCurrentDirectoryW(MAX_PATH, buffer) != 0u) {
		std::wstring wstr(buffer);
		return PathNormalize({ wstr.begin(), wstr.end() });
	}
	return {};
#elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
	char buffer[PATH_MAX];
	if (getcwd(buffer, sizeof(buffer)) != nullptr) {
		return std::string(buffer);
	}
	return {};
#endif
}

bool DirSetCurrent(const std::string &path) {
#ifdef AQUILA_PLATFORM_WINDOWS
	return SetCurrentDirectoryA(path.c_str()) != 0; // was missing .c_str() and != 0
#elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
	return (chdir(path.c_str()) == 0);
#endif
}

bool DirCreate(const std::string &path) {
#ifdef AQUILA_PLATFORM_WINDOWS
	return _mkdir(path.c_str()) == 0;
#elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
	return (mkdir(path.c_str(), 0755) == 0);
#endif
}

bool DirRemove(const std::string &path) {
#ifdef AQUILA_PLATFORM_WINDOWS
	return RemoveDirectoryA(path.c_str()) != 0;
#elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
	return (rmdir(path.c_str()) == 0);
#endif
}

bool FileRemove(const std::string &path) {
#ifdef AQUILA_PLATFORM_WINDOWS
	return DeleteFileA(path.c_str()) != 0;
#elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
	return (::remove(path.c_str()) == 0); // :: to avoid clashing with std::remove
#endif
}

bool FileMove(const std::string &from, const std::string &to) {
#ifdef AQUILA_PLATFORM_WINDOWS
	return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
	return (std::rename(from.c_str(), to.c_str()) == 0); // :: to avoid any std::rename ambiguity
#endif
}

std::vector<std::string> DirList(const std::string &path, bool recursive) {
	std::vector<std::string> entries;
#ifdef AQUILA_PLATFORM_WINDOWS
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA((path + "\\*").c_str(), &findData);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
				entries.push_back(findData.cFileName);
				if (recursive && (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					auto sub = DirList(path + "\\" + findData.cFileName, true);
					entries.insert(entries.end(), sub.begin(), sub.end());
				}
			}
		} while (FindNextFileA(hFind, &findData));
		FindClose(hFind);
	}
#elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
	DIR *dir = opendir(path.c_str());
	if (dir) {
		struct dirent *entry;
		while ((entry = readdir(dir)) != nullptr) {
			if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
				entries.push_back(entry->d_name);
				if (recursive && entry->d_type == DT_DIR) {
					auto sub = DirList(path + "/" + entry->d_name, true);
					entries.insert(entries.end(), sub.begin(), sub.end());
				}
			}
		}
		closedir(dir);
	}
#endif
	return entries;
}

std::string PathToAbsolute(const std::string &path) {
	if (PathIsAbsolute(path)) {
		return path;
	}
	return PathJoin(DirGetCurrent(), path);
}

std::string PathExtension(const std::string &path) {
	const size_t pos = path.find_last_of('.');
	// pos == 0 catches ".hidden", pos == npos catches no dot,
	// check no separator after dot to avoid "dir.name/file" false positives
	if (pos == std::string::npos || pos == 0)
		return {};
	const size_t sep = path.find_last_of("/\\");
	if (sep != std::string::npos && sep > pos)
		return {};
	return path.substr(pos); // includes the dot, e.g. ".txt"
}

} // namespace Aquila::Platform::Filesystem

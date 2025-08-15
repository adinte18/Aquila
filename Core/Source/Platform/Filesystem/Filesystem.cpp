#include "Platform/Filesystem/Filesystem.h"
#include <algorithm>
#include <string>

namespace Filesystem {
    bool Exists(const std::string& path) {
        #ifdef AQUILA_PLATFORM_WINDOWS 
            DWORD attr = GetFileAttributesA(path.c_str());
            return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
        #elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
            struct stat buffer;
            return (stat(path.c_str(), &buffer) == 0);
        #endif
    }

    FileStat Stat(const std::string& path) {
        FileStat stat;
        #ifdef AQUILA_PLATFORM_WINDOWS
            WIN32_FILE_ATTRIBUTE_DATA data;
            if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &data)) {
                stat.exists = true;
                stat.size = static_cast<uint32>(data.nFileSizeLow);
                stat.isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                stat.isRegularFile = !stat.isDirectory;
                stat.lastWriteTime = static_cast<uint32>(data.ftLastWriteTime.dwLowDateTime);
            }
        #elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
            struct stat buffer;
            if (stat(path.c_str(), &buffer) == 0) {
                stat.exists = true;
                stat.size = static_cast<uint32>(buffer.st_size);
                stat.isDirectory = S_ISDIR(buffer.st_mode);
                stat.isRegularFile = S_ISREG(buffer.st_mode);
                stat.lastWriteTime = static_cast<uint32>(buffer.st_mtime);
            }
        #endif
        return stat;
    }

    bool IsAbsolute(const std::string& path) {
        #ifdef AQUILA_PLATFORM_WINDOWS
            return (path.size() > 1 && path[1] == ':');
        #elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
            return (path.size() > 0 && path[0] == '/');
        #endif
    }

    std::string JoinPath(const std::string& a, const std::string& b) {
        if (a.empty()) return b;
        if (b.empty()) return a;

        #ifdef AQUILA_PLATFORM_WINDOWS
            return a + "\\" + b;
        #else
            return a + "/" + b;
        #endif
    }

    std::string NormalizePath(const std::string& path) {
        #ifdef AQUILA_PLATFORM_WINDOWS
            std::string normalized = path;
            std::replace(normalized.begin(), normalized.end(), '/', '\\');
            return normalized;
        #else
            return path;
        #endif
    }

    std::string GetCurrDirectory() {
        #ifdef AQUILA_PLATFORM_WINDOWS
            wchar_t buffer[MAX_PATH];
            if (GetCurrentDirectoryW(MAX_PATH, buffer)) {
                std::wstring wstr(buffer);
                return std::string(wstr.begin(), wstr.end());
            } else {
                return std::string();
            }
        #elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
            char buffer[PATH_MAX];
            if (getcwd(buffer, sizeof(buffer)) != nullptr) {
                return std::string(buffer);
            } else {
                return std::string();
            }
        #endif
    }

    bool SetCurrentDirectory(const std::string& path) {
        #ifdef AQUILA_PLATFORM_WINDOWS
            return SetCurrentDirectoryA(path.c_str());
        #elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
            return (chdir(path.c_str()) == 0);
        #endif
    }

    bool CreateDirectories(const std::string& path) {
        #ifdef AQUILA_PLATFORM_WINDOWS
            return _mkdir(path.c_str()) == 0;
        #elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
            return (mkdir(path.c_str(), 0755) == 0);
        #endif
    }

    bool Remove(const std::string& path) {
        #ifdef AQUILA_PLATFORM_WINDOWS
            return DeleteFileA(path.c_str()) != 0;
        #elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
            return (remove(path.c_str()) == 0);
        #endif
    }

    bool Rename(const std::string& from, const std::string& to) {
        #ifdef AQUILA_PLATFORM_WINDOWS
            return MoveFileA(from.c_str(), to.c_str()) != 0;
        #elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
            return (rename(from.c_str(), to.c_str()) == 0);
        #endif
    }

    std::vector<std::string> ListDirectory(const std::string& path, bool recursive){
        std::vector<std::string> files;
        #ifdef AQUILA_PLATFORM_WINDOWS
            WIN32_FIND_DATAA findData;
            HANDLE hFind = FindFirstFileA((path + "\\*").c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                        files.push_back(findData.cFileName);
                        if (recursive && (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                            auto subFiles = ListDirectory(path + "\\" + findData.cFileName, true);
                            files.insert(files.end(), subFiles.begin(), subFiles.end());
                        }
                    }
                } while (FindNextFileA(hFind, &findData));
                FindClose(hFind);
            }
        #elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
            DIR* dir = opendir(path.c_str());
            if (dir) {
                struct dirent* entry;
                while ((entry = readdir(dir)) != nullptr) {
                    if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                        files.push_back(entry->d_name);
                        if (recursive && entry->d_type == DT_DIR) {
                            auto subFiles = ListDirectory(path + "/" + entry->d_name, true);
                            files.insert(files.end(), subFiles.begin(), subFiles.end());
                        }
                    }
                }
                closedir(dir);
            }
        #endif
        return files;
    }

    std::string ToAbsolute(const std::string& path) {
        if (IsAbsolute(path)) {
            return path;
        } else {
            return JoinPath(GetCurrDirectory(), path);
        }
    }

    std::string Extension(const std::string& path) {
        size_t pos = path.find_last_of('.');
        if (pos != std::string::npos && pos != 0 && pos != path.size() - 1) {
            return path.substr(pos + 1);
        }
        return std::string();
    }
}
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "Platform/PrimitiveTypes.h"
#include <optional>
#include <vector>

namespace Filesystem {
    struct FileStat {
        uint32 size = 0;
        bool exists = false;
        bool isDirectory = false;
        bool isRegularFile = false;
        uint32 lastWriteTime = 0;
    };

    std::string JoinPath(const std::string& a, const std::string& b);
    std::string NormalizePath(const std::string& path);
    bool IsAbsolute(const std::string& path);
    std::string GetCurrDirectory();
    bool SetCurrentDirectory(const std::string& path);

    bool Exists(const std::string& path);
    FileStat Stat(const std::string& path);
    bool CreateDirectories(const std::string& path);
    bool Remove(const std::string& path);
    bool Rename(const std::string& from, const std::string& to);

    std::vector<std::string> ListDirectory(const std::string& path, bool recursive = false);

    // Helpers
    std::string ToAbsolute(const std::string& path);
    std::string Extension(const std::string& path);
}

#endif // FILESYSTEM_H
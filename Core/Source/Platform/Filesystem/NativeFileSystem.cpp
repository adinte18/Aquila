#include "Platform/Filesystem/NativeFileSystem.h"

namespace VFS {
    NativeFileSystem::NativeFileSystem(const std::string& rootPath) 
        : m_rootPath(Filesystem::NormalizePath(rootPath)) {

            if (!m_rootPath.empty() && m_rootPath.back() != '/' && m_rootPath.back() != '\\') {
            #ifdef AQUILA_PLATFORM_WINDOWS
                m_rootPath += "\\";
            #else
                m_rootPath += "/";
            #endif
        }
    }

    std::unique_ptr<VirtualFile> NativeFileSystem::OpenFile(const std::string& path, const std::string& mode) {
        std::string cleanPath = path;
        if (!cleanPath.empty() && cleanPath[0] == '/') {
            cleanPath = cleanPath.substr(1);
        }
        
        std::string fullPath = Filesystem::JoinPath(m_rootPath, cleanPath);
        fullPath = Filesystem::NormalizePath(fullPath);
        
        FILE* file = fopen(fullPath.c_str(), mode.c_str());
        if (!file) return nullptr;
        
        return CreateUnique<NativeFile>(file);
    }

    bool NativeFileSystem::Exists(const std::string& path) {
        std::string cleanPath = path;
        if (!cleanPath.empty() && cleanPath[0] == '/') {
            cleanPath = cleanPath.substr(1);
        }
        
        std::string fullPath = Filesystem::JoinPath(m_rootPath, cleanPath);
        return Filesystem::Exists(fullPath);
    }

    std::vector<std::string> NativeFileSystem::ListDirectory(const std::string& path) {
        std::string cleanPath = path;
        if (!cleanPath.empty() && cleanPath[0] == '/') {
            cleanPath = cleanPath.substr(1);
        }
        
        std::string fullPath = Filesystem::JoinPath(m_rootPath, cleanPath);
        return Filesystem::ListDirectory(fullPath, false);
    }

    bool NativeFileSystem::IsDirectory(const std::string& path) {
        std::string cleanPath = path;
        if (!cleanPath.empty() && cleanPath[0] == '/') {
            cleanPath = cleanPath.substr(1);
        }
        
        std::string fullPath = Filesystem::JoinPath(m_rootPath, cleanPath);
        auto stat = Filesystem::Stat(Filesystem::NormalizePath(fullPath));
        return stat.exists && stat.isDirectory;
    }

    int64_t NativeFileSystem::GetFileSize(const std::string& path) {
        std::string cleanPath = path;
        if (!cleanPath.empty() && cleanPath[0] == '/') {
            cleanPath = cleanPath.substr(1);
        }
        
        std::string fullPath = Filesystem::JoinPath(m_rootPath, cleanPath);
        auto stat = Filesystem::Stat(fullPath);
        return stat.exists ? static_cast<int64_t>(stat.size) : -1;
    }

    uint64_t NativeFileSystem::GetLastWriteTime(const std::string& path) {
        std::string cleanPath = path;
        if (!cleanPath.empty() && cleanPath[0] == '/') {
            cleanPath = cleanPath.substr(1);
        }

        std::string fullPath = Filesystem::JoinPath(m_rootPath, cleanPath);
        auto stat = Filesystem::Stat(fullPath);
        return stat.exists ? stat.lastWriteTime : 0;
    }

    bool NativeFileSystem::CreateDir(const std::string& path) {
        std::string cleanPath = path;
        if (!cleanPath.empty() && cleanPath[0] == '/') {
            cleanPath = cleanPath.substr(1);
        }

        std::string fullPath = Filesystem::JoinPath(m_rootPath, cleanPath);
        return Filesystem::CreateDirectories(fullPath);
    }

    bool NativeFileSystem::DeleteFile_aq(const std::string& path) {
        std::string cleanPath = path;
        if (!cleanPath.empty() && cleanPath[0] == '/') {
            cleanPath = cleanPath.substr(1);
        }

        std::string fullPath = Filesystem::JoinPath(m_rootPath, cleanPath);
        return Filesystem::RemoveFile(fullPath);
    }

    bool NativeFileSystem::DeleteDirectory(const std::string& path) {
        std::string cleanPath = path;
        if (!cleanPath.empty() && cleanPath[0] == '/') {
            cleanPath = cleanPath.substr(1);
        }

        std::string fullPath = Filesystem::JoinPath(m_rootPath, cleanPath);
        return Filesystem::RemoveDir(fullPath);
    }


}
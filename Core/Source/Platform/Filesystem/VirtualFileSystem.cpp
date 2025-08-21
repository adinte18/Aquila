#include "Platform/Filesystem/VirtualFileSystem.h"

namespace VFS {
    std::string VirtualFileSystem::NormalizePath(const std::string& path) {
        if (path.empty()) return "/";
        
        std::string normalized = path;
        
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        
        if (normalized[0] != '/') {
            normalized = "/" + normalized;
        }
        
        if (normalized.length() > 1 && normalized.back() == '/') {
            normalized.pop_back();
        }
        
        // TODO: Handle .. and . in path resolution
        return normalized;
    }

    MountPoint* VirtualFileSystem::FindMountPoint(const std::string& virtualPath, std::string& relativePath) {
        std::string normalizedPath = NormalizePath(virtualPath);
        
        std::sort(m_MountPoints.begin(), m_MountPoints.end(), 
            [](const MountPoint& a, const MountPoint& b) {
                if (a.priority != b.priority) return a.priority > b.priority;
                return a.virtualPath.length() > b.virtualPath.length();
            });
        
        for (auto& mount : m_MountPoints) {
            const std::string& mountPath = mount.virtualPath;
            
            if (normalizedPath.substr(0, mountPath.length()) == mountPath) {
                if (normalizedPath.length() == mountPath.length()) {
                    relativePath = "/";
                } else if (normalizedPath[mountPath.length()] == '/') {
                    relativePath = normalizedPath.substr(mountPath.length());
                } else {
                    continue;
                }
                
                return &mount;
            }
        }
        
        return nullptr;
    }

    bool VirtualFileSystem::Mount(const std::string& virtualPath, std::shared_ptr<IFileSystem> fileSystem, int priority, bool readOnly) {
        if (!fileSystem) return false;
        
        std::string normalizedPath = NormalizePath(virtualPath);
        
        for (const auto& mount : m_MountPoints) {
            if (mount.virtualPath == normalizedPath) {
                return false;
            }
        }
        
        MountPoint mount;
        mount.virtualPath = normalizedPath;
        mount.fileSystem = fileSystem;
        mount.priority = priority;
        mount.readOnly = readOnly;
        
        m_MountPoints.push_back(mount);
        return true;
    }

    bool VirtualFileSystem::Unmount(const std::string& virtualPath) {
        std::string normalizedPath = NormalizePath(virtualPath);
        
        auto it = std::find_if(m_MountPoints.begin(), m_MountPoints.end(),
            [&normalizedPath](const MountPoint& mount) {
                return mount.virtualPath == normalizedPath;
            });
        
        if (it != m_MountPoints.end()) {
            m_MountPoints.erase(it);
            return true;
        }
        
        return false;
    }

    std::unique_ptr<VirtualFile> VirtualFileSystem::OpenFile(const std::string& virtualPath, const std::string& mode) {
        std::string relativePath;
        MountPoint* mount = FindMountPoint(virtualPath, relativePath);
        
        if (!mount) return nullptr;
        
        if ((mode.find('w') != std::string::npos || mode.find('a') != std::string::npos) && 
            (mount->readOnly || mount->fileSystem->IsReadOnly())) {
            return nullptr;
        }
        
        return mount->fileSystem->OpenFile(relativePath, mode);
    }

    bool VirtualFileSystem::Exists(const std::string& virtualPath) {
        std::string relativePath;
        MountPoint* mount = FindMountPoint(virtualPath, relativePath);
        
        if (!mount) return false;
        return mount->fileSystem->Exists(relativePath);
    }

    std::vector<std::string> VirtualFileSystem::ListDirectory(const std::string& virtualPath) {
        std::string relativePath;
        MountPoint* mount = FindMountPoint(virtualPath, relativePath);

        if (!mount) return {};
        return mount->fileSystem->ListDirectory(relativePath);
    }

    bool VirtualFileSystem::IsDirectory(const std::string& virtualPath) {
        std::string relativePath;
        MountPoint* mount = FindMountPoint(virtualPath, relativePath);

        if (!mount) return false;
        return mount->fileSystem->IsDirectory(relativePath);
    }

    int64_t VirtualFileSystem::GetFileSize(const std::string& virtualPath) {
        std::string relativePath;
        MountPoint* mount = FindMountPoint(virtualPath, relativePath);

        if (!mount) return -1;
        return mount->fileSystem->GetFileSize(relativePath);
    }

    uint64_t VirtualFileSystem::GetLastWriteTime(const std::string& virtualPath) {
        std::string relativePath;
        MountPoint* mount = FindMountPoint(virtualPath, relativePath);

        if (!mount) return 0;
        return mount->fileSystem->GetLastWriteTime(relativePath);
    }

    bool VirtualFileSystem::CreateDir(const std::string& virtualPath) {
        std::string relativePath;
        MountPoint* mount = FindMountPoint(virtualPath, relativePath);

        if (!mount || mount->readOnly || mount->fileSystem->IsReadOnly())
            return false;

        return mount->fileSystem->CreateDir(relativePath);
    }

    bool VirtualFileSystem::DeleteFile_aq(const std::string& virtualPath) {
        std::string relativePath;
        MountPoint* mount = FindMountPoint(virtualPath, relativePath);

        if (!mount || mount->readOnly || mount->fileSystem->IsReadOnly())
            return false;

        return mount->fileSystem->DeleteFile_aq(relativePath);
    }

    bool VirtualFileSystem::DeleteDirectory(const std::string& virtualPath) {
        std::string relativePath;
        MountPoint* mount = FindMountPoint(virtualPath, relativePath);

        if (!mount || mount->readOnly || mount->fileSystem->IsReadOnly())
            return false;

        return mount->fileSystem->DeleteDirectory(relativePath);
    }

    std::vector<std::string> VirtualFileSystem::GetMountPoints() const {
        std::vector<std::string> result;
        result.reserve(m_MountPoints.size());
        for (const auto& mount : m_MountPoints)
            result.push_back(mount.virtualPath);
        return result;
    }

    bool VirtualFileSystem::IsMounted(const std::string& virtualPath) const {
        std::string normalizedPath = const_cast<VirtualFileSystem*>(this)->NormalizePath(virtualPath);
        for (const auto& mount : m_MountPoints) {
            if (mount.virtualPath == normalizedPath)
                return true;
        }
        return false;
    }

    void VirtualFileSystem::UnmountAll() {
        m_MountPoints.clear();
    }

}
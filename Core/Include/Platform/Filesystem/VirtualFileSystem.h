#ifndef VFS_H
#define VFS_H

#include "Platform/Filesystem/IFileSystem.h"
#include "Utilities/Singleton.h"


namespace VFS {
    struct MountPoint {
        std::string virtualPath;
        std::string realPath;
        std::shared_ptr<IFileSystem> fileSystem;
        int priority = 0;
        bool readOnly = false;
    };


    class VirtualFileSystem : public Utility::Singleton<VirtualFileSystem> {
    friend class Utility::Singleton<VirtualFileSystem>; 
    private:
        std::vector<MountPoint> m_MountPoints;

        std::string NormalizePath(const std::string& path);
        MountPoint* FindMountPoint(const std::string& virtualPath, std::string& relativePath);

    public:
        bool Mount(const std::string& virtualPath, std::shared_ptr<IFileSystem> fileSystem, 
                int priority = 0, bool readOnly = false);
        bool Unmount(const std::string& virtualPath);
        void UnmountAll();
        
        std::unique_ptr<VirtualFile> OpenFile(const std::string& virtualPath, const std::string& mode = "rb");
        bool Exists(const std::string& virtualPath);
        std::vector<std::string> ListDirectory(const std::string& virtualPath);
        bool IsDirectory(const std::string& virtualPath);
        int64_t GetFileSize(const std::string& virtualPath);
        uint64_t GetLastWriteTime(const std::string& virtualPath);
        
        bool CreateDir(const std::string& virtualPath);
        bool DeleteFile_aq(const std::string& virtualPath);
        bool DeleteDirectory(const std::string& virtualPath);
        
        std::vector<std::string> GetMountPoints() const;
        bool IsMounted(const std::string& virtualPath) const;
    };

}



#endif // VFS_H
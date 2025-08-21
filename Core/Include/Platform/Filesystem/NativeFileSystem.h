#ifndef NFS_H
#define NFS_H

#include "Platform/Filesystem/Filesystem.h"
#include "Platform/Filesystem/IFileSystem.h"
#include "Platform/Filesystem/Files/NativeFile.h"

namespace VFS {
    class NativeFileSystem : public IFileSystem {
    private:
        std::string m_rootPath;
        
    public:
        NativeFileSystem(const std::string& rootPath);
        ~NativeFileSystem() override = default;

        std::unique_ptr<VirtualFile> OpenFile(const std::string& path, const std::string& mode) override;
        bool Exists(const std::string& path) override;
        std::vector<std::string> ListDirectory(const std::string& path) override;
        bool IsDirectory(const std::string& path) override;
        int64_t GetFileSize(const std::string& path) override;
        uint64_t GetLastWriteTime(const std::string& path) override;
        bool CreateDir(const std::string& path) override;
        bool DeleteFile_aq(const std::string& path) override;
        bool DeleteDirectory(const std::string& path) override;
        bool IsReadOnly() const override { return false; }
        std::string GetDisplayName() const override { return "Native: " + m_rootPath; }
    };

}


#endif // NFS_H
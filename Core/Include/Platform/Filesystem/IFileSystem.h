#ifndef ABSTRACT_FILE_SYSTEM_H
#define ABSTRACT_FILE_SYSTEM_H

#include "Platform/Platform.h"
#include "Platform/Filesystem/Files/VirtualFile.h"

namespace VFS {
    class IFileSystem {
    public:

        virtual ~IFileSystem() = default;
    
        virtual std::unique_ptr<VirtualFile> OpenFile(const std::string& path, const std::string& mode) = 0;
        virtual bool Exists(const std::string& path) = 0;
        virtual std::vector<std::string> ListDirectory(const std::string& path) = 0;
        virtual bool IsDirectory(const std::string& path) = 0;
        virtual int64_t GetFileSize(const std::string& path) = 0;
        virtual uint64_t GetLastWriteTime(const std::string& path) = 0;
        
        virtual bool CreateDir(const std::string& path) { return false; }
        virtual bool DeleteFile_aq(const std::string& path) { return false; }
        virtual bool DeleteDirectory(const std::string& path) { return false; }
        virtual bool IsReadOnly() const { return true; }
        virtual std::string GetDisplayName() const = 0;

    };
}

#endif // ABSTRACT_FILE_SYSTEM_H
#ifndef ABSTRACT_FILE_SYSTEM_H
#define ABSTRACT_FILE_SYSTEM_H

#include "Aquila/Platform/Filesystem/Files/VirtualFile.h"

namespace Aquila::Platform::Filesystem {
class IFileSystem {
  public:
	virtual ~IFileSystem() = default;

	virtual std::unique_ptr<VirtualFile> OpenFile(const std::string &path, const std::string &mode) = 0;
	virtual bool Exists(const std::string &path) = 0;
	virtual std::vector<std::string> ListDirectory(const std::string &path) = 0;
	virtual bool IsDirectory(const std::string &path) = 0;
	virtual int64 GetFileSize(const std::string &path) = 0;
	virtual uint64 GetLastWriteTime(const std::string &path) = 0;
	virtual bool RenameFile(const std::string &oldPath, const std::string &newPath) = 0;
	virtual bool CopyFileA(const std::string &srcPath, const std::string &dstPath) = 0;

	virtual bool CreateDir(const std::string &path) { return false; }
	virtual bool DeleteFile_aq(const std::string &path) { return false; }
	virtual bool DeleteDirectory(const std::string &path) { return false; }
	virtual bool IsReadOnly() const { return true; }
	virtual std::string GetDisplayName() const = 0;
};
} // namespace Aquila::Platform::Filesystem

#endif // ABSTRACT_FILE_SYSTEM_H

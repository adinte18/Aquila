#ifndef ABSTRACT_FILE_SYSTEM_H
#define ABSTRACT_FILE_SYSTEM_H

#include "Aquila/Foundation/Defines.h"
#include "Aquila/Platform/Filesystem/Files/NativeFile.h"

namespace Aquila::Platform::Filesystem {
class IFileSystem {
  public:
	AQUILA_NONCOPYABLE(IFileSystem);
	AQUILA_NONMOVEABLE(IFileSystem);

	IFileSystem() = default;
	virtual ~IFileSystem() = default;

	virtual Unique<NativeFile> FileOpen(const std::string &path, AccessMode accessMode, OpenMode openMode) = 0;
	virtual bool FileExists(const std::string &path) = 0;
	virtual bool FileRemove(const std::string &path) = 0;
	virtual bool FileMove(const std::string &oldPath, const std::string &newPath) = 0;
	virtual bool FileCopy(const std::string &srcPath, const std::string &dstPath) = 0;
	virtual int64 FileGetSize(const std::string &path) = 0;
	virtual uint64 FileGetLastWriteTime(const std::string &path) = 0;

	// Directory operations
	virtual bool DirExists(const std::string &path) = 0;
	virtual bool DirCreate(const std::string &path) = 0;
	virtual bool DirRemove(const std::string &path) = 0;
	virtual std::vector<std::string> DirList(const std::string &path) = 0;

	// Filesystem properties
	[[nodiscard]] virtual bool IsReadOnly() const = 0;
	[[nodiscard]] virtual std::string GetDisplayName() const = 0;
};
} // namespace Aquila::Platform::Filesystem

#endif // ABSTRACT_FILE_SYSTEM_H

#ifndef AQUILA_VFS_H
#define AQUILA_VFS_H

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Platform/Filesystem/IFileSystem.h"
#include "Aquila/Foundation/Singleton.h"

namespace Aquila::Platform::Filesystem {

struct MountPoint {
	std::string virtualPath;
	std::string realPath;
	Ref<IFileSystem> fileSystem;
	int priority = 0;
	bool readOnly = false;
};

class VirtualFileSystem : public Foundation::Singleton<VirtualFileSystem> {
	friend class Singleton;

  private:
	std::vector<MountPoint> m_MountPoints;
	mutable Mutex m_MountPointsMutex;

	std::string NormalizePath(const std::string &path);
	MountPoint *FindMountPoint(const std::string &virtualPath, std::string &relativePath);

  public:
	bool Mount(const std::string &virtualPath, Ref<IFileSystem> fileSystem, int priority = 0, bool readOnly = false);
	bool Unmount(const std::string &virtualPath);
	void UnmountAll();
	bool RenameFile(const std::string &oldVirtualPath, const std::string &newVirtualPath);
	Unique<VirtualFile> OpenFile(const std::string &virtualPath, AccessMode accessMode, OpenMode openMode);
	bool Exists(const std::string &virtualPath);
	std::vector<std::string> ListDirectory(const std::string &virtualPath);
	bool IsDirectory(const std::string &virtualPath);
	int64 GetFileSize(const std::string &virtualPath);
	uint64 GetLastWriteTime(const std::string &virtualPath);

	bool CopyFileA(const std::string &srcVirtualPath, const std::string &dstVirtualPath);
	bool CreateDir(const std::string &virtualPath);
	bool DeleteFile_aq(const std::string &virtualPath);
	bool DeleteDirectory(const std::string &virtualPath);
	bool WriteTextFile(const std::string &virtualPath, const std::string &content);
	std::string ReadTextFile(const std::string &virtualPath);
	std::vector<std::string> GetMountPoints() const;
	bool IsMounted(const std::string &virtualPath) const;
};

} // namespace Aquila::Platform::Filesystem

#endif // VFS_H

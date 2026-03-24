#ifndef AQUILA_NFS_H
#define AQUILA_NFS_H
#include "Aquila/Platform/Filesystem/IFileSystem.h"
#include "Aquila/Platform/Filesystem/Files/NativeFile.h"

namespace Aquila::Platform::Filesystem {

class NativeFileSystem final : public IFileSystem {
  private:
	std::string m_RootPath;

	[[nodiscard]] std::string ResolvePath(const std::string &path) const;
	static std::string ToFopenMode(AccessMode accessMode, OpenMode openMode);

  public:
	explicit NativeFileSystem(const std::string &rootPath);
	~NativeFileSystem() override = default;

	// File operations
	Unique<NativeFile> FileOpen(const std::string &path, AccessMode accessMode, OpenMode openMode) override;
	bool FileExists(const std::string &path) override;
	bool FileRemove(const std::string &path) override;
	int64 FileGetSize(const std::string &path) override;
	uint64 FileGetLastWriteTime(const std::string &path) override;

	bool FileMove(const std::string &oldPath, const std::string &newPath) override;

	bool FileCopy(const std::string &srcPath, const std::string &dstPath) override;

	// Directory operations
	bool DirExists(const std::string &path) override;
	bool DirCreate(const std::string &path) override;
	bool DirRemove(const std::string &path) override;
	std::vector<std::string> DirList(const std::string &path) override;

	[[nodiscard]] bool IsReadOnly() const override { return false; }
	[[nodiscard]] std::string GetDisplayName() const override { return "Native: " + m_RootPath; }
};

} // namespace Aquila::Platform::Filesystem
#endif // AQUILA_NFS_H

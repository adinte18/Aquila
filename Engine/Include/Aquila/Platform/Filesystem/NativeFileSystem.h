#ifndef AQUILA_NFS_H
#define AQUILA_NFS_H

#include "Aquila/Platform/Filesystem/IFileSystem.h"
#include "Aquila/Platform/Filesystem/Files/NativeFile.h"

namespace Aquila::Platform::Filesystem {
class NativeFileSystem final : public IFileSystem {
  private:
	std::string m_RootPath;

  public:
	explicit NativeFileSystem(const std::string &rootPath);
	~NativeFileSystem() override = default;
	bool RenameFile(const std::string &oldPath, const std::string &newPath) override {
		std::string fullOldPath = m_RootPath + oldPath;
		std::string fullNewPath = m_RootPath + newPath;

#ifdef AQUILA_PLATFORM_WINDOWS
		return _wrename(std::filesystem::path(fullOldPath).c_str(), std::filesystem::path(fullNewPath).c_str()) == 0;
#else
		return std::rename(fullOldPath.c_str(), fullNewPath.c_str()) == 0;
#endif
	}

	bool CopyFileA(const std::string &srcPath, const std::string &dstPath) override {
		std::string fullSrcPath = m_RootPath + srcPath;
		std::string fullDstPath = m_RootPath + dstPath;

		try {
			std::filesystem::copy(fullSrcPath, fullDstPath, std::filesystem::copy_options::overwrite_existing);
			return true;
		} catch (const std::exception &e) {
			return false;
		}
	}

	std::unique_ptr<VirtualFile> OpenFile(const std::string &path, const std::string &mode) override;
	bool Exists(const std::string &path) override;
	std::vector<std::string> ListDirectory(const std::string &path) override;
	bool IsDirectory(const std::string &path) override;
	int64_t GetFileSize(const std::string &path) override;
	uint64_t GetLastWriteTime(const std::string &path) override;
	bool CreateDir(const std::string &path) override;
	bool DeleteFile_aq(const std::string &path) override;
	bool DeleteDirectory(const std::string &path) override;
	[[nodiscard]] bool IsReadOnly() const override { return false; }
	[[nodiscard]] std::string GetDisplayName() const override { return "Native: " + m_RootPath; }
};

} // namespace Aquila::Platform::Filesystem

#endif // NFS_H

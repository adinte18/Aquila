#include "Aquila/Platform/Filesystem/NativeFileSystem.h"
#include "Aquila/Platform/Filesystem/Filesystem.h"

namespace Aquila::Platform::Filesystem {

NativeFileSystem::NativeFileSystem(const std::string &rootPath) : m_RootPath(Filesystem::PathNormalize(rootPath)) {
	if (!FileExists(m_RootPath)) {
		DirCreate(m_RootPath);
	}
	if (!m_RootPath.empty() && m_RootPath.back() != '/' && m_RootPath.back() != '\\') {
#ifdef AQUILA_PLATFORM_WINDOWS
		m_RootPath += '\\';
#else
		m_RootPath += '/';
#endif
	}
}

std::string NativeFileSystem::ResolvePath(const std::string &path) const {
	if (Filesystem::PathIsAbsolute(path)) {
		return Filesystem::PathNormalize(path);
	}
	const std::string clean = (!path.empty() && (path[0] == '/' || path[0] == '\\')) ? path.substr(1) : path;
	return Filesystem::PathNormalize(Filesystem::PathJoin(m_RootPath, clean));
}

std::string NativeFileSystem::ToFopenMode(AccessMode accessMode, OpenMode openMode) {
	const bool binary = HasFlag(openMode, OpenMode::Binary);
	const bool append = HasFlag(openMode, OpenMode::Append);
	const char *b = binary ? "b" : "";

	if (append) {
		return std::string("a") + b; // "a" or "ab"
	}

	switch (accessMode) {
	case AccessMode::Read:
		return std::string("r") + b; // "r"  or "rb"
	case AccessMode::Write:
		return std::string("w") + b; // "w"  or "wb"
	case AccessMode::ReadWrite:
		return std::string("r+") + b; // "r+" or "r+b"
	default:
		return std::string("r") + b;
	}
}

Unique<NativeFile> NativeFileSystem::FileOpen(const std::string &path, AccessMode accessMode, OpenMode openMode) {
	FILE *file = fopen(ResolvePath(path).c_str(), ToFopenMode(accessMode, openMode).c_str());
	return (file != nullptr) ? CreateUnique<NativeFile>(file) : nullptr;
}

bool NativeFileSystem::FileExists(const std::string &path) {
	return Filesystem::FileExists(ResolvePath(path));
}

bool NativeFileSystem::FileRemove(const std::string &path) {
	return Filesystem::FileRemove(ResolvePath(path));
}

bool NativeFileSystem::FileMove(const std::string &oldPath, const std::string &newPath) {
	const std::string fullOld = ResolvePath(oldPath);
	const std::string fullNew = ResolvePath(newPath);
#ifdef AQUILA_PLATFORM_WINDOWS
	return MoveFileExA(fullOld.c_str(), fullNew.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
	return std::rename(fullOld.c_str(), fullNew.c_str()) == 0;
#endif
}

bool NativeFileSystem::FileCopy(const std::string &srcPath, const std::string &dstPath) {
	try {
		std::filesystem::copy(ResolvePath(srcPath), ResolvePath(dstPath),
							  std::filesystem::copy_options::overwrite_existing);
		return true;
	} catch (const std::exception &) {
		return false;
	}
}

int64 NativeFileSystem::FileGetSize(const std::string &path) {
	const auto stat = Filesystem::FileStat_(ResolvePath(path));
	return stat.exists ? static_cast<int64>(stat.size) : -1;
}

uint64 NativeFileSystem::FileGetLastWriteTime(const std::string &path) {
	const auto stat = Filesystem::FileStat_(ResolvePath(path));
	return stat.exists ? stat.lastWriteTime : 0;
}

bool NativeFileSystem::DirExists(const std::string &path) {
	const auto stat = Filesystem::FileStat_(ResolvePath(path));
	return stat.exists && stat.isDirectory;
}

bool NativeFileSystem::DirCreate(const std::string &path) {
	return Filesystem::DirCreate(ResolvePath(path));
}

bool NativeFileSystem::DirRemove(const std::string &path) {
	return Filesystem::DirRemove(ResolvePath(path));
}

std::vector<std::string> NativeFileSystem::DirList(const std::string &path) {
	return Filesystem::DirList(ResolvePath(path), false);
}

} // namespace Aquila::Platform::Filesystem

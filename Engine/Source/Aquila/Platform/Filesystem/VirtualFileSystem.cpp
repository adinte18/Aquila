#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

#include <algorithm>

namespace Aquila::Platform::Filesystem {

std::string VirtualFileSystem::NormalizePath(const std::string &path) {
	if (path.empty()) {
		return "/";
	}

	std::string normalized = path;

	std::ranges::replace(normalized, '\\', '/');

	if (normalized[0] != '/') {
		normalized = "/" + normalized;
	}

	if (normalized.length() > 1 && normalized.back() == '/') {
		normalized.pop_back();
	}

	return normalized;
}

MountPoint *VirtualFileSystem::FindMountPoint(const std::string &virtualPath, std::string &relativePath) {
	std::shared_lock<std::shared_mutex> lock(m_MountPointsMutex);

	std::string normalizedPath = NormalizePath(virtualPath);

	for (auto &mount : m_MountPoints) {
		const std::string &mountPath = mount.virtualPath;

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

bool VirtualFileSystem::Mount(const std::string &virtualPath, Ref<IFileSystem> fileSystem, int priority,
							  bool readOnly) {
	if (!fileSystem) {
		return false;
	}

	std::string normalizedPath = NormalizePath(virtualPath);

	std::unique_lock<std::shared_mutex> lock(m_MountPointsMutex);

	for (const auto &mount : m_MountPoints) {
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

	std::sort(m_MountPoints.begin(), m_MountPoints.end(), [](const MountPoint &a, const MountPoint &b) {
		if (a.priority != b.priority)
			return a.priority > b.priority;
		return a.virtualPath.length() > b.virtualPath.length();
	});
	return true;
}

bool VirtualFileSystem::Unmount(const std::string &virtualPath) {
	std::string normalizedPath = NormalizePath(virtualPath);

	std::unique_lock<std::shared_mutex> lock(m_MountPointsMutex);

	auto it = std::find_if(m_MountPoints.begin(), m_MountPoints.end(),
						   [&normalizedPath](const MountPoint &mount) { return mount.virtualPath == normalizedPath; });

	if (it != m_MountPoints.end()) {
		m_MountPoints.erase(it);
		return true;
	}

	return false;
}

std::unique_ptr<VirtualFile> VirtualFileSystem::OpenFile(const std::string &virtualPath, const std::string &mode) {
	std::string relativePath;
	MountPoint *mount = FindMountPoint(virtualPath, relativePath);

	if (!mount)
		return nullptr;

	if ((mode.find('w') != std::string::npos || mode.find('a') != std::string::npos) &&
		(mount->readOnly || mount->fileSystem->IsReadOnly())) {
		return nullptr;
	}

	return mount->fileSystem->OpenFile(relativePath, mode);
}

bool VirtualFileSystem::Exists(const std::string &virtualPath) {
	std::string relativePath;
	MountPoint *mount = FindMountPoint(virtualPath, relativePath);

	if (!mount)
		return false;
	return mount->fileSystem->Exists(relativePath);
}

std::vector<std::string> VirtualFileSystem::ListDirectory(const std::string &virtualPath) {
	std::string relativePath;
	MountPoint *mount = FindMountPoint(virtualPath, relativePath);

	if (!mount)
		return {};
	return mount->fileSystem->ListDirectory(relativePath);
}

bool VirtualFileSystem::IsDirectory(const std::string &virtualPath) {
	std::string relativePath;
	MountPoint *mount = FindMountPoint(virtualPath, relativePath);

	if (!mount)
		return false;
	return mount->fileSystem->IsDirectory(relativePath);
}

int64_t VirtualFileSystem::GetFileSize(const std::string &virtualPath) {
	std::string relativePath;
	MountPoint *mount = FindMountPoint(virtualPath, relativePath);

	if (!mount)
		return -1;
	return mount->fileSystem->GetFileSize(relativePath);
}

uint64_t VirtualFileSystem::GetLastWriteTime(const std::string &virtualPath) {
	std::string relativePath;
	MountPoint *mount = FindMountPoint(virtualPath, relativePath);

	if (!mount)
		return 0;
	return mount->fileSystem->GetLastWriteTime(relativePath);
}

bool VirtualFileSystem::CreateDir(const std::string &virtualPath) {
	std::string relativePath;
	MountPoint *mount = FindMountPoint(virtualPath, relativePath);

	if (!mount || mount->readOnly || mount->fileSystem->IsReadOnly())
		return false;

	return mount->fileSystem->CreateDir(relativePath);
}

bool VirtualFileSystem::DeleteFile_aq(const std::string &virtualPath) {
	std::string relativePath;
	MountPoint *mount = FindMountPoint(virtualPath, relativePath);

	if (!mount || mount->readOnly || mount->fileSystem->IsReadOnly())
		return false;

	return mount->fileSystem->DeleteFile_aq(relativePath);
}

bool VirtualFileSystem::DeleteDirectory(const std::string &virtualPath) {
	std::string relativePath;
	MountPoint *mount = FindMountPoint(virtualPath, relativePath);

	if (!mount || mount->readOnly || mount->fileSystem->IsReadOnly())
		return false;

	return mount->fileSystem->DeleteDirectory(relativePath);
}

bool VirtualFileSystem::RenameFile(const std::string &oldVirtualPath, const std::string &newVirtualPath) {
	std::string oldRelativePath;
	MountPoint *oldMount = FindMountPoint(oldVirtualPath, oldRelativePath);

	std::string newRelativePath;
	MountPoint *newMount = FindMountPoint(newVirtualPath, newRelativePath);

	// Both paths must be on the same mount point
	if (!oldMount || !newMount || oldMount != newMount)
		return false;

	if (oldMount->readOnly || oldMount->fileSystem->IsReadOnly())
		return false;

	return oldMount->fileSystem->RenameFile(oldRelativePath, newRelativePath);
}

bool VirtualFileSystem::CopyFileA(const std::string &srcVirtualPath, const std::string &dstVirtualPath) {
	std::string srcRelativePath;
	MountPoint *srcMount = FindMountPoint(srcVirtualPath, srcRelativePath);

	std::string dstRelativePath;
	MountPoint *dstMount = FindMountPoint(dstVirtualPath, dstRelativePath);

	if (!srcMount || !dstMount)
		return false;

	if (dstMount->readOnly || dstMount->fileSystem->IsReadOnly())
		return false;

	// If same mount, use native copy if available
	if (srcMount == dstMount) {
		return srcMount->fileSystem->CopyFileA(srcRelativePath, dstRelativePath);
	}

	// Otherwise, read from source and write to destination
	auto srcFile = srcMount->fileSystem->OpenFile(srcRelativePath, "rb");
	if (!srcFile)
		return false;

	std::vector<uint8_t> buffer(srcFile->Size());
	if (srcFile->Read(buffer.data(), buffer.size()) != buffer.size()) {
		return false;
	}

	auto dstFile = dstMount->fileSystem->OpenFile(dstRelativePath, "wb");
	if (!dstFile)
		return false;

	return dstFile->Write(buffer.data(), buffer.size()) == buffer.size();
}

bool VirtualFileSystem::WriteTextFile(const std::string &virtualPath, const std::string &content) {
	std::string relativePath;
	MountPoint *mount = FindMountPoint(virtualPath, relativePath);

	if (!mount || mount->readOnly || mount->fileSystem->IsReadOnly()) {
		return false;
	}

	auto file = mount->fileSystem->OpenFile(relativePath, "w");
	if (!file) {
		return false;
	}

	if (content.empty()) {
		return true;
	}

	return file->Write(reinterpret_cast<const uint8_t *>(content.data()), content.size()) == content.size();
}

std::string VirtualFileSystem::ReadTextFile(const std::string &virtualPath) {
	std::string relativePath;
	MountPoint *mount = FindMountPoint(virtualPath, relativePath);

	if (mount == nullptr) {
		return "";
	}

	auto file = mount->fileSystem->OpenFile(relativePath, "r");
	if (!file) {
		return "";
	}

	const int64_t size = file->Size();
	if (size <= 0) {
		return "";
	}

	std::string result(static_cast<size_t>(size), '\0');
	const size_t bytesRead = file->Read(reinterpret_cast<uint8_t *>(result.data()), static_cast<size_t>(size));
	result.resize(bytesRead);
	return result;
}

std::vector<std::string> VirtualFileSystem::GetMountPoints() const {
	std::shared_lock<std::shared_mutex> lock(m_MountPointsMutex);

	std::vector<std::string> result;
	result.reserve(m_MountPoints.size());
	for (const auto &mount : m_MountPoints) {
		result.push_back(mount.virtualPath);
	}
	return result;
}

bool VirtualFileSystem::IsMounted(const std::string &virtualPath) const {
	std::shared_lock<std::shared_mutex> lock(m_MountPointsMutex);

	std::string normalizedPath = const_cast<VirtualFileSystem *>(this)->NormalizePath(virtualPath);
	for (const auto &mount : m_MountPoints) {
		if (mount.virtualPath == normalizedPath)
			return true;
	}
	return false;
}

void VirtualFileSystem::UnmountAll() {
	std::unique_lock<std::shared_mutex> lock(m_MountPointsMutex);
	m_MountPoints.clear();
}

} // namespace Aquila::Platform::Filesystem

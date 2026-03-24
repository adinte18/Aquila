#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "Aquila/Foundation/PrimitiveTypes.h"
namespace Aquila::Platform::Filesystem {
struct FileStatInfo {
	uint32 size = 0;
	bool exists = false;
	bool isDirectory = false;
	bool isRegularFile = false;
	uint32 lastWriteTime = 0;
};
std::string PathJoin(const std::string &a, const std::string &b);
std::string PathNormalize(const std::string &path);
bool PathIsAbsolute(const std::string &path);
std::string DirGetCurrent();
bool DirSetCurrent(const std::string &path);
bool FileExists(const std::string &path);
FileStatInfo FileStat_(const std::string &path);
bool DirCreate(const std::string &path);
bool DirRemove(const std::string &path);
bool FileRemove(const std::string &path);
bool FileMove(const std::string &from, const std::string &to);
std::vector<std::string> DirList(const std::string &path, bool recursive = false);

// Helpers
std::string PathToAbsolute(const std::string &path);
std::string PathExtension(const std::string &path);
} // namespace Aquila::Platform::Filesystem
#endif // FILESYSTEM_H

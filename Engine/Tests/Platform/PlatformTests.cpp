#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "Aquila/Platform/Filesystem/Filesystem.h"
#include "Aquila/Platform/Filesystem/NativeFileSystem.h"

using namespace Aquila::Platform::Filesystem;

static std::string MakeTempRoot(const std::string &name) {
	std::string path = PathJoin(DirGetCurrent(), "__test_" + name);
	DirCreate(path);
	return path;
}

static void CleanupTempRoot(const std::string &path) {
	DirRemove(path);
}

TEST_SUITE("Filesystem::Path") {
	TEST_CASE("PathJoin") {
		CHECK(PathJoin("/foo", "bar") == "/foo/bar");
		CHECK(PathJoin("/foo/", "bar") == "/foo/bar");
		CHECK(PathJoin("", "bar") == "bar");
		CHECK(PathJoin("/foo", "") == "/foo");
	}

	TEST_CASE("PathNormalize") {
		CHECK(PathNormalize("/foo/../bar") == "/bar");
		CHECK(PathNormalize("/foo/./bar") == "/foo/bar");
		CHECK(PathNormalize("//foo//bar") == "/foo/bar");
		CHECK(PathNormalize("foo\\bar") == "foo/bar");
	}

	TEST_CASE("PathIsAbsolute") {
#ifdef AQUILA_PLATFORM_WINDOWS
		CHECK(PathIsAbsolute("C:\\foo") == true);
		CHECK(PathIsAbsolute("foo") == false);
#else
		CHECK(PathIsAbsolute("/foo") == true);
		CHECK(PathIsAbsolute("foo") == false);
		CHECK(PathIsAbsolute("") == false);
#endif
	}

	TEST_CASE("PathExtension") {
		CHECK(PathExtension("file.txt") == ".txt");
		CHECK(PathExtension("file.tar.gz") == ".gz");
		CHECK(PathExtension("file") == "");
		CHECK(PathExtension(".hidden") == "");
	}
}

TEST_SUITE("Filesystem::Dir") {
	TEST_CASE("DirCreate / DirRemove / FileStat") {
		const std::string root = MakeTempRoot("dircreate");
		const std::string sub = PathJoin(root, "subdir");

		CHECK(DirCreate(sub) == true);
		const auto stat = FileStat_(sub);
		CHECK(stat.exists == true);
		CHECK(stat.isDirectory == true);

		CHECK(DirRemove(sub) == true);
		CHECK(FileStat_(sub).exists == false);

		CleanupTempRoot(root);
	}

	TEST_CASE("DirList") {
		const std::string root = MakeTempRoot("dirlist");

		// Create two subdirs and one file
		DirCreate(PathJoin(root, "a"));
		DirCreate(PathJoin(root, "b"));
		// write a tiny file
		FILE *f = fopen(PathJoin(root, "file.txt").c_str(), "w");
		if (f) {
			fputs("x", f);
			fclose(f);
		}

		auto entries = DirList(root);
		CHECK(entries.size() == 3);

		CleanupTempRoot(root);
	}
}

TEST_SUITE("Filesystem::File") {
	TEST_CASE("FileMove") {
		const std::string root = MakeTempRoot("filemove");
		const std::string src = PathJoin(root, "src.txt");
		const std::string dst = PathJoin(root, "dst.txt");

		FILE *f = fopen(src.c_str(), "w");
		REQUIRE(f != nullptr);
		fputs("data", f);
		fclose(f);

		CHECK(FileMove(src, dst) == true);
		CHECK(FileExists(src) == false);
		CHECK(FileExists(dst) == true);

		CleanupTempRoot(root);
	}

	TEST_CASE("FileExists / FileRemove") {
		const std::string root = MakeTempRoot("fileexists");
		const std::string file = PathJoin(root, "test.txt");

		CHECK(FileExists(file) == false);

		FILE *f = fopen(file.c_str(), "w");
		REQUIRE(f != nullptr);
		fputs("hello", f);
		fclose(f);

		CHECK(FileExists(file) == true);
		CHECK(FileRemove(file) == true);
		CHECK(FileExists(file) == false);

		CleanupTempRoot(root);
	}
}

TEST_SUITE("NativeFileSystem") {
	TEST_CASE("DirCreate / DirExists / DirRemove") {
		const std::string root = MakeTempRoot("nfs_dir");
		NativeFileSystem nfs(root);

		CHECK(nfs.DirCreate("sub") == true);
		CHECK(nfs.DirExists("sub") == true);
		CHECK(nfs.DirRemove("sub") == true);
		CHECK(nfs.DirExists("sub") == false);

		CleanupTempRoot(root);
	}

	TEST_CASE("Constructor creates root if missing") {
		const std::string root = PathJoin(DirGetCurrent(), "__test_nfs_root");
		CHECK(FileExists(root) == false);

		NativeFileSystem nfs(root);
		const auto stat = FileStat_(root);
		CHECK(stat.isDirectory == true);

		DirRemove(root);
	}

	TEST_CASE("FileOpen / FileExists / FileRemove") {
		const std::string root = MakeTempRoot("nfs_open");
		NativeFileSystem nfs(root);

		auto file = nfs.FileOpen("hello.txt", AccessMode::Write, OpenMode::Text);
		REQUIRE(file != nullptr);
		file.reset(); // close it

		CHECK(nfs.FileExists("hello.txt") == true);
		CHECK(nfs.FileRemove("hello.txt") == true);
		CHECK(nfs.FileExists("hello.txt") == false);

		CleanupTempRoot(root);
	}

	TEST_CASE("FileGetSize") {
		const std::string root = MakeTempRoot("nfs_size");
		NativeFileSystem nfs(root);

		auto file = nfs.FileOpen("size.txt", AccessMode::Write, OpenMode::Binary);
		REQUIRE(file != nullptr);
		const char *data = "hello";
		file->Write(data, 5);
		file.reset();

		CHECK(nfs.FileGetSize("size.txt") == 5);

		CleanupTempRoot(root);
	}

	TEST_CASE("FileMove renames within root") {
		const std::string root = MakeTempRoot("nfs_move");
		NativeFileSystem nfs(root);

		auto f = nfs.FileOpen("old.txt", AccessMode::Write, OpenMode::Text);
		REQUIRE(f != nullptr);
		f.reset();

		CHECK(nfs.FileMove("old.txt", "new.txt") == true);
		CHECK(nfs.FileExists("old.txt") == false);
		CHECK(nfs.FileExists("new.txt") == true);

		CleanupTempRoot(root);
	}

	TEST_CASE("IsReadOnly returns false") {
		const std::string root = MakeTempRoot("nfs_ro");
		NativeFileSystem nfs(root);
		CHECK(nfs.IsReadOnly() == false);
		CleanupTempRoot(root);
	}
}

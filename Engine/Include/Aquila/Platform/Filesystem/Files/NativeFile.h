#ifndef NATIVE_FILE_H
#define NATIVE_FILE_H

#include "Aquila/Platform/Filesystem/Files/VirtualFile.h"

namespace Aquila::Platform::Filesystem {
class NativeFile final : public VirtualFile {
  private:
	FILE *m_file;
	int64_t m_size;

  public:
	explicit NativeFile(FILE *file);

	~NativeFile() override;

	size_t Read(void *buffer, size_t size) override;

	size_t Write(const void *buffer, size_t size) override;

	bool Seek(int64_t offset, int origin) override;

	int64 Tell() const override;

	int64 Size() const override;

	bool IsValid() const override;

	void Close() override;
};
} // namespace Aquila::Platform::Filesystem

#endif // NATIVE_FILE_H

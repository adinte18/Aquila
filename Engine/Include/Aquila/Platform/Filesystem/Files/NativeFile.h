#ifndef NATIVE_FILE_H
#define NATIVE_FILE_H

#include "Aquila/Platform/Filesystem/Files/VirtualFile.h"

namespace Aquila::Platform::Filesystem {
class NativeFile final : public VirtualFile {
  private:
	FILE *m_file;
	int64 m_size;

  public:
	explicit NativeFile(FILE *file);
	~NativeFile() override;
	usize Read(void *buffer, usize size) override;
	usize Write(const void *buffer, usize size) override;
	bool Seek(int64 offset, int origin) override;
	void Close() override;
	[[nodiscard]] int64 Tell() const override;
	[[nodiscard]] int64 Size() const override;
	[[nodiscard]] bool IsValid() const override;
};
} // namespace Aquila::Platform::Filesystem

#endif // NATIVE_FILE_H

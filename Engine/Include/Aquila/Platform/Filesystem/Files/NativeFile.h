#ifndef NATIVE_FILE_H
#define NATIVE_FILE_H

#include "Aquila/Platform/Filesystem/Files/VirtualFile.h"

namespace Aquila::Platform::Filesystem {
class NativeFile final : public VirtualFile {
  private:
	FILE *m_file;
	Int64 m_size;

  public:
	explicit NativeFile(FILE *file);
	~NativeFile() override;
	Usize read(void *buffer, Usize size) override;
	Usize write(const void *buffer, Usize size) override;
	bool seek(Int64 offset, int origin) override;
	void close() override;
	[[nodiscard]] Int64 tell() const override;
	[[nodiscard]] Int64 size() const override;
	[[nodiscard]] bool is_valid() const override;
};
} // namespace Aquila::Platform::Filesystem

#endif // NATIVE_FILE_H

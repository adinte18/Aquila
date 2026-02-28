#ifndef MEMORY_FILE_H
#define MEMORY_FILE_H

#include "Aquila/Platform/Filesystem/Files/VirtualFile.h"

namespace Aquila::Platform::Filesystem {
class MemoryFile final : public VirtualFile {
  private:
	std::vector<uint8_t> *m_data;
	size_t m_position;
	bool m_canWrite;

  public:
	MemoryFile(std::vector<uint8_t> *data, bool canWrite);

	size_t Read(void *buffer, size_t size) override;

	size_t Write(const void *buffer, size_t size) override;

	bool Seek(int64_t offset, int origin) override;

	int64_t Tell() const override;

	int64_t Size() const override;

	bool IsValid() const override;

	void Close() override;
};
} // namespace Aquila::Platform::Filesystem

#endif // MEMORY_FILE_H
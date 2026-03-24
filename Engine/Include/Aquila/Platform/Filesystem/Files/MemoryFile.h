#ifndef MEMORY_FILE_H
#define MEMORY_FILE_H

#include "Aquila/Platform/Filesystem/Files/VirtualFile.h"

namespace Aquila::Platform::Filesystem {
class MemoryFile final : public VirtualFile {
  private:
	std::vector<uint8> *m_data;
	usize m_position;
	bool m_canWrite;

  public:
	MemoryFile(std::vector<uint8> *data, bool canWrite);
	usize Read(void *buffer, usize size) override;
	usize Write(const void *buffer, usize size) override;
	bool Seek(int64 offset, int origin) override;
	[[nodiscard]] int64 Tell() const override;
	[[nodiscard]] int64 Size() const override;
	[[nodiscard]] bool IsValid() const override;

	void Close() override;
};
} // namespace Aquila::Platform::Filesystem

#endif // MEMORY_FILE_H

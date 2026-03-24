#include "Aquila/Platform/Filesystem/Files/NativeFile.h"

namespace Aquila::Platform::Filesystem {
NativeFile::NativeFile(FILE *file) : m_file(file), m_size(-1) {
	if (m_file != nullptr) {
		fseek(m_file, 0, SEEK_END);
		m_size = ftell(m_file);
		fseek(m_file, 0, SEEK_SET);
	}
}

NativeFile::~NativeFile() {
	Close();
}

usize NativeFile::Read(void *buffer, const usize size) {
	if (m_file == nullptr) {
		return 0;
	}
	return fread(buffer, 1, size, m_file);
}

usize NativeFile::Write(const void *buffer, const usize size) {
	if (m_file == nullptr) {
		return 0;
	}
	return fwrite(buffer, 1, size, m_file);
}

bool NativeFile::Seek(const int64 offset, const int origin) {
	if (m_file == nullptr) {
		return false;
	}
	return fseek(m_file, static_cast<long>(offset), origin) == 0;
}

int64 NativeFile::Tell() const {
	if (m_file == nullptr) {
		return -1;
	}
	return ftell(m_file);
}

int64 NativeFile::Size() const {
	return m_size;
}

bool NativeFile::IsValid() const {
	return m_file != nullptr;
}

void NativeFile::Close() {
	if (m_file != nullptr) {
		fclose(m_file);
		m_file = nullptr;
	}
}

} // namespace Aquila::Platform::Filesystem

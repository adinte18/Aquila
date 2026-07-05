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
	close();
}

Usize NativeFile::read(void *buffer, const Usize size) {
	if (m_file == nullptr) {
		return 0;
	}
	return fread(buffer, 1, size, m_file);
}

Usize NativeFile::write(const void *buffer, const Usize size) {
	if (m_file == nullptr) {
		return 0;
	}
	return fwrite(buffer, 1, size, m_file);
}

bool NativeFile::seek(const Int64 offset, const int origin) {
	if (m_file == nullptr) {
		return false;
	}
	return fseek(m_file, static_cast<long>(offset), origin) == 0;
}

Int64 NativeFile::tell() const {
	if (m_file == nullptr) {
		return -1;
	}
	return ftell(m_file);
}

Int64 NativeFile::size() const {
	return m_size;
}

bool NativeFile::is_valid() const {
	return m_file != nullptr;
}

void NativeFile::close() {
	if (m_file != nullptr) {
		fclose(m_file);
		m_file = nullptr;
	}
}

} // namespace Aquila::Platform::Filesystem

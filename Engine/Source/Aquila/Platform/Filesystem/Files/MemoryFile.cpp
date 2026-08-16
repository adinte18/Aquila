#include "Aquila/Platform/Filesystem/Files/MemoryFile.h"

namespace Aquila::Platform::Filesystem {
MemoryFile::MemoryFile(std::vector<uint8_t> *data, bool can_write) : m_data(data), m_position(0), m_can_write(can_write) {}

size_t MemoryFile::read(void *buffer, size_t size) {
	if ((m_data == nullptr) || m_position >= m_data->size()) {
		return 0;
	}

	size_t bytes_to_read = std::min(size, m_data->size() - m_position);
	memcpy(buffer, m_data->data() + m_position, bytes_to_read);
	m_position += bytes_to_read;
	return bytes_to_read;
}

size_t MemoryFile::write(const void *buffer, size_t size) {
	if ((m_data == nullptr) || !m_can_write) {
		return 0;
	}

	// Resize if necessary
	if (m_position + size > m_data->size()) {
		m_data->resize(m_position + size);
	}

	memcpy(m_data->data() + m_position, buffer, size);
	m_position += size;
	return size;
}

bool MemoryFile::seek(Int64 offset, int origin) {
	if (m_data == nullptr) {
		return false;
	}

	Int64 new_pos;
	switch (origin) {
	case SEEK_SET:
		new_pos = offset;
		break;
	case SEEK_CUR:
		new_pos = static_cast<Int64>(m_position) + offset;
		break;
	case SEEK_END:
		new_pos = static_cast<Int64>(m_data->size()) + offset;
		break;
	default:
		return false;
	}

	return !(new_pos < 0 || new_pos > static_cast<Int64>(m_data->size()));
}

Int64 MemoryFile::tell() const {
	return static_cast<Int64>(m_position);
}

Int64 MemoryFile::size() const {
	return (m_data != nullptr) ? static_cast<Int64>(m_data->size()) : 0;
}

bool MemoryFile::is_valid() const {
	return m_data != nullptr;
}

void MemoryFile::close() {
	// Memory files don't need explicit closing
}

} // namespace Aquila::Platform::Filesystem

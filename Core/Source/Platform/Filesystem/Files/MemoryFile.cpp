#include "Platform/Filesystem/Files/MemoryFile.h"

namespace VFS {
    MemoryFile::MemoryFile(std::vector<uint8_t>* data, bool canWrite) 
        : m_data(data), m_position(0), m_canWrite(canWrite) {}
    
    size_t MemoryFile::Read(void* buffer, size_t size) {
        if (!m_data || m_position >= m_data->size()) return 0;
        
        size_t bytesToRead = std::min(size, m_data->size() - m_position);
        memcpy(buffer, m_data->data() + m_position, bytesToRead);
        m_position += bytesToRead;
        return bytesToRead;
    }
    
    size_t MemoryFile::Write(const void* buffer, size_t size) {
        if (!m_data || !m_canWrite) return 0;
        
        // Resize if necessary
        if (m_position + size > m_data->size()) {
            m_data->resize(m_position + size);
        }
        
        memcpy(m_data->data() + m_position, buffer, size);
        m_position += size;
        return size;
    }
    
    bool MemoryFile::Seek(int64_t offset, int origin) {
        if (!m_data) return false;
        
        int64_t newPos;
        switch (origin) {
            case SEEK_SET: newPos = offset; break;
            case SEEK_CUR: newPos = static_cast<int64_t>(m_position) + offset; break;
            case SEEK_END: newPos = static_cast<int64_t>(m_data->size()) + offset; break;
            default: return false;
        }
        
        if (newPos < 0 || newPos > static_cast<int64_t>(m_data->size())) {
            return false;
        }
        
        m_position = static_cast<size_t>(newPos);
        return true;
    }
    
    int64_t MemoryFile::Tell() const {
        return static_cast<int64_t>(m_position);
    }
    
    int64_t MemoryFile::Size() const {
        return m_data ? static_cast<int64_t>(m_data->size()) : 0;
    }
    
    bool MemoryFile::IsValid() const {
        return m_data != nullptr;
    }
    
    void MemoryFile::Close() {
        // Memory files don't need explicit closing
    }

}
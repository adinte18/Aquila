#include "Platform/Filesystem/Files/NativeFile.h"

namespace VFS {
    NativeFile::NativeFile(FILE* file) : m_file(file), m_size(-1) {
        if (m_file) {
            fseek(m_file, 0, SEEK_END);
            m_size = ftell(m_file);
            fseek(m_file, 0, SEEK_SET);
        }
    }
    
    NativeFile::~NativeFile() {
        Close();
    }
    
    size_t NativeFile::Read(void* buffer, size_t size) {
        if (!m_file) return 0;
        return fread(buffer, 1, size, m_file);
    }

    size_t NativeFile::Write(const void* buffer, size_t size) {
        if (!m_file) return 0;
        return fwrite(buffer, 1, size, m_file);
    }
    
    bool NativeFile::Seek(int64 offset, int origin) {
        if (!m_file) return false;
        return fseek(m_file, static_cast<long>(offset), origin) == 0;
    }
    
    int64 NativeFile::Tell() const {
        if (!m_file) return -1;
        return ftell(m_file);
    }
    
    int64 NativeFile::Size() const {
        return m_size;
    }
    
    bool NativeFile::IsValid() const {
        return m_file != nullptr;
    }
    
    void NativeFile::Close() {
        if (m_file) {
            fclose(m_file);
            m_file = nullptr;
        }
    }

}
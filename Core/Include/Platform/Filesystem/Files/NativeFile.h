#ifndef NATIVE_FILE_H
#define NATIVE_FILE_H

#include "Platform/Filesystem/Files/VirtualFile.h"

namespace VFS {
    class NativeFile : public VirtualFile {
    private:
        FILE* m_file;
        int64_t m_size;
        
    public:
        NativeFile(FILE* file);
        
        ~NativeFile();
        
        size_t Read(void* buffer, size_t size);
        
        size_t Write(const void* buffer, size_t size);
        
        bool Seek(int64_t offset, int origin);

        int64 Tell() const;

        int64 Size() const;

        bool IsValid() const;

        void Close();
    };
}


#endif // NATIVE_FILE_H
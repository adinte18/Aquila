#ifndef VIRTUAL_FILE_H
#define VIRTUAL_FILE_H

#include "Platform/Platform.h"

namespace VFS {
    class VirtualFile {
    public:
        virtual ~VirtualFile() = default;

        virtual size_t Read(void* buffer, size_t size) = 0;
        virtual size_t Write(const void* buffer, size_t size) = 0;

        virtual bool Seek(int64 offset, int origin) = 0;
        virtual int64 Tell() const = 0;
        virtual int64 Size() const = 0;
        virtual bool IsValid() const = 0;
        virtual void Close() = 0;
    };
}

#endif // VIRTUAL_FILE_H
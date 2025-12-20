#include <windows.h>
#include <cstdint>

class RKC_FILE
{
public:
    HANDLE m_handle;
};

extern "C"
{
    void __thiscall RKC_FILE_constructor(RKC_FILE* self)
    {
        self->m_handle = 0;
    }

    void __thiscall RKC_FILE_deconstructor(RKC_FILE* self)
    {
        self->m_handle = 0;
    }

    int __thiscall Create(RKC_FILE* self, char* fileName, long desiredAccess)
    {
        bool result = false;
        switch (desiredAccess)
        {
        case 0:
            self->m_handle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            result = self->m_handle != INVALID_HANDLE_VALUE;
            break;
        case 1:
            self->m_handle = CreateFileA(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
            result = self->m_handle != INVALID_HANDLE_VALUE;
            break;
        case 2:
            self->m_handle = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
            result = self->m_handle != INVALID_HANDLE_VALUE;
            break;
        case 3:
            self->m_handle = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
            result = self->m_handle != INVALID_HANDLE_VALUE;
            if (result)
                SetFilePointer(self->m_handle, 0, 0, 2);
            break;
        default:
            break;
        }

        return result;
    }

    int __thiscall Close(RKC_FILE* self)
    {
        bool result = false;

        if (!self->m_handle || (result = CloseHandle(self->m_handle)))
        {
            self->m_handle = 0;
            return 1;
        }

        return result;
    }

    int __thiscall Write(RKC_FILE* self, void* buffer, long numBytesToWrite)
    {
        DWORD bytesWritten = 0;
        bool result = WriteFile(self->m_handle, buffer, numBytesToWrite, &bytesWritten, nullptr);
        if (bytesWritten != numBytesToWrite)
            return false;

        return result;
    }

    int __thiscall Read(RKC_FILE* self, void* buffer, long numBytesToRead)
    {
        DWORD bytesRead = 0;
        bool result = ReadFile(self->m_handle, buffer, numBytesToRead, &bytesRead, nullptr);
        if (bytesRead != numBytesToRead)
            return false;

        return result;
    }

    long __thiscall GetSize(RKC_FILE* self)
    {
        DWORD fileSizeHigh = 0;
        if (self->m_handle)
            return GetFileSize(self->m_handle, &fileSizeHigh);

        return -1;
    }

    int __thiscall Seek(RKC_FILE* self, long distanceToMove, long moveMethod)
    {
        if (moveMethod < 0 || moveMethod > 2)
            return false;

        return SetFilePointer(self->m_handle, distanceToMove, 0, moveMethod);
    }

    RKC_FILE& __thiscall equalsOperator(RKC_FILE* self, const RKC_FILE& lhs)
    {
        self->m_handle = lhs.m_handle;
        return *self;
    }

    HANDLE __thiscall GetHandle(RKC_FILE* self)
    {
        return self->m_handle;
    }
}
bool __stdcall DllMain(LPVOID, std::uint32_t call_reason, LPVOID)
{
    return true;
}
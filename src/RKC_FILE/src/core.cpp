/**
 * RKC_FILE - File I/O wrapper class
 * 
 * Simple file handle wrapper used throughout the game for file operations.
 * USED BY: ShadowFlare.exe, o_RKC_DIB.dll, o_RKC_DSOUND.dll, o_RKC_FONTMAKER.dll,
 *          o_RKC_RPG_AICONTROL.dll, o_RKC_RPG_SCRIPT.dll, o_RKC_RPGSCRN.dll,
 *          o_RKC_RPG_TABLE.dll, o_RKC_UPDIB.dll
 */

#include <windows.h>
#include <cstdint>

class RKC_FILE
{
public:
    HANDLE m_handle;
};

extern "C"
{
    /**
     * Constructor - initialize file handle to null
     */
    void __thiscall RKC_FILE_constructor(RKC_FILE* self)
    {
        self->m_handle = 0;
    }

    /**
     * Destructor - reset file handle (does NOT close it)
     */
    void __thiscall RKC_FILE_deconstructor(RKC_FILE* self)
    {
        self->m_handle = 0;
    }

    /**
     * Open or create a file
     * Args: fileName = path to file, desiredAccess = mode (0=read, 1=write, 2=read/write, 3=append)
     * Returns: 1 on success, 0 on failure
     */
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

    /**
     * Close file handle
     * Returns: 1 on success
     */
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

    /**
     * Write data to file
     * Returns: 1 if all bytes written successfully
     */
    int __thiscall Write(RKC_FILE* self, void* buffer, long numBytesToWrite)
    {
        DWORD bytesWritten = 0;
        bool result = WriteFile(self->m_handle, buffer, numBytesToWrite, &bytesWritten, nullptr);
        if (bytesWritten != numBytesToWrite)
            return false;

        return result;
    }

    /**
     * Read data from file
     * Returns: 1 if all bytes read successfully
     */
    int __thiscall Read(RKC_FILE* self, void* buffer, long numBytesToRead)
    {
        DWORD bytesRead = 0;
        bool result = ReadFile(self->m_handle, buffer, numBytesToRead, &bytesRead, nullptr);
        if (bytesRead != numBytesToRead)
            return false;

        return result;
    }

    /**
     * Get file size in bytes
     * USED BY: o_RKC_RPGSCRN.dll, o_RKC_RPG_TABLE.dll
     */
    long __thiscall GetSize(RKC_FILE* self)
    {
        DWORD fileSizeHigh = 0;
        if (self->m_handle)
            return GetFileSize(self->m_handle, &fileSizeHigh);

        return -1;
    }

    /**
     * Seek to position in file
     * moveMethod: 0=FILE_BEGIN, 1=FILE_CURRENT, 2=FILE_END
     * USED BY: o_RKC_DIB.dll, o_RKC_UPDIB.dll
     */
    int __thiscall Seek(RKC_FILE* self, long distanceToMove, long moveMethod)
    {
        if (moveMethod < 0 || moveMethod > 2)
            return false;

        return SetFilePointer(self->m_handle, distanceToMove, 0, moveMethod);
    }

    /**
     * Assignment operator - shallow copy of handle
     * NOT REFERENCED - not imported by any module
     */
    RKC_FILE& __thiscall equalsOperator(RKC_FILE* self, const RKC_FILE& other)
    {
        self->m_handle = other.m_handle;
        return *self;
    }

    /**
     * Get underlying Windows HANDLE
     * NOT REFERENCED - not imported by any module
     */
    HANDLE __thiscall GetHandle(RKC_FILE* self)
    {
        return self->m_handle;
    }
}
bool __stdcall DllMain(LPVOID, std::uint32_t call_reason, LPVOID)
{
    return true;
}
/**
 * RK_FUNCTION - Core utility functions (incremental implementation)
 * 
 * Implements functions one by one. Unimplemented functions forward via dll.def.
 */

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstdio>

extern "C" {

/**
 * Check if a byte is a SJIS lead byte (first byte of 2-byte character)
 * Returns: 1 if SJIS lead byte, 0 otherwise
 */
int __cdecl RK_CheckSJIS(int ch)
{
    unsigned char c = (unsigned char)ch;
    if ((c >= 0x80 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFF))
        return 1;
    return 0;
}

/**
 * Check if string contains SJIS characters
 */
int __cdecl RK_CheckStringSJIS(const char* str)
{
    if (!str) return 0;
    
    while (*str)
    {
        if (RK_CheckSJIS((unsigned char)*str))
            return 1;
        str++;
    }
    return 0;
}

/**
 * Check if path ends with backslash
 */
int __cdecl RK_CheckLastRoot(const char* path)
{
    if (!path) return 0;
    
    size_t len = strlen(path);
    if (len == 0) return 0;
    
    return (path[len - 1] == '\\') ? 1 : 0;
}

/**
 * Compare strings (case insensitive)
 */
int __cdecl RK_StringsCompare(const char* str1, const char* str2)
{
    if (!str1 || !str2) return 0;
    return _stricmp(str1, str2) == 0 ? 1 : 0;
}

/**
 * Copy string with automatic sizing
 */
void __cdecl RK_StringsCopyAuto(char* dest, const char* src, int maxLen)
{
    if (!dest || !src) return;
    
    if (maxLen > 0)
    {
        strncpy(dest, src, maxLen - 1);
        dest[maxLen - 1] = '\0';
    }
    else
    {
        strcpy(dest, src);
    }
}

/**
 * Delete tabs and spaces from string
 */
void __cdecl RK_DeleteTabSpaceString(char* str)
{
    if (!str) return;
    
    char* src = str;
    char* dst = str;
    
    while (*src)
    {
        if (*src != ' ' && *src != '\t')
            *dst++ = *src;
        src++;
    }
    *dst = '\0';
}

/**
 * Remove trailing backslash from path if present
 */
void __cdecl RK_CutLastRoot(char* path)
{
    if (!path) return;
    
    size_t len = strlen(path);
    if (len == 0) return;
    
    // Find last backslash, handling SJIS
    int lastSlash = -2;
    for (size_t i = 0; i < len; i++)
    {
        if (RK_CheckSJIS((unsigned char)path[i]))
        {
            i++;  // Skip second byte of SJIS char
            continue;
        }
        if (path[i] == '\\')
            lastSlash = (int)i;
    }
    
    // If last char is backslash, remove it
    if (lastSlash == (int)(len - 1))
        path[len - 1] = '\0';
}

/**
 * Add trailing backslash to path if not present
 */
void __cdecl RK_SetLastRoot(char* path)
{
    if (!path) return;
    
    size_t len = strlen(path);
    if (len == 0) return;
    
    if (path[len - 1] != '\\')
    {
        path[len] = '\\';
        path[len + 1] = '\0';
    }
}

/**
 * Extract directory from full path (remove filename)
 */
void __cdecl RK_CutFilenameFromFullPath(char* fullPath)
{
    if (!fullPath) return;
    
    size_t len = strlen(fullPath);
    if (len == 0) return;
    
    // Find last backslash
    int lastSlash = -1;
    for (size_t i = 0; i < len; i++)
    {
        if (RK_CheckSJIS((unsigned char)fullPath[i]))
        {
            i++;
            continue;
        }
        if (fullPath[i] == '\\')
            lastSlash = (int)i;
    }
    
    if (lastSlash >= 0)
        fullPath[lastSlash + 1] = '\0';
}

/**
 * Extract filename from full path (remove directory)
 */
void __cdecl RK_CutDirectoryFromFullPath(char* dest, const char* fullPath)
{
    if (!dest || !fullPath) return;
    
    size_t len = strlen(fullPath);
    
    // Find last backslash
    int lastSlash = -1;
    for (size_t i = 0; i < len; i++)
    {
        if (RK_CheckSJIS((unsigned char)fullPath[i]))
        {
            i++;
            continue;
        }
        if (fullPath[i] == '\\')
            lastSlash = (int)i;
    }
    
    if (lastSlash >= 0)
        strcpy(dest, fullPath + lastSlash + 1);
    else
        strcpy(dest, fullPath);
}

/**
 * Copy a number as string with specified digits
 */
void __cdecl RK_StringCopyNumber(char* dest, int number, int digits)
{
    if (!dest) return;
    
    char format[16];
    sprintf(format, "%%0%dd", digits);
    sprintf(dest, format, number);
}

} // extern "C"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}

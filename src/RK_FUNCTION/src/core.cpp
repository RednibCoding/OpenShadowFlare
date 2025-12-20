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
 * Compare strings with optional case insensitivity (SJIS-aware)
 * Returns: 0 if str1 empty, 1 if equal, -1 if str1 < str2
 */
int __cdecl RK_StringsCompare(const char* str1, const char* str2, int caseInsensitive)
{
    const unsigned char* s1 = (const unsigned char*)str1;
    const unsigned char* s2 = (const unsigned char*)str2;
    
    // Return 0 if str1 is empty
    if (*s1 == 0) return 0;
    
    while (*s1)
    {
        // If str2 ends first, strings match (original behavior)
        if (*s2 == 0) return 1;
        
        // Check if current byte is SJIS lead byte
        if (RK_CheckSJIS(*s1))
        {
            // Compare 2-byte SJIS sequence byte-by-byte
            for (int i = 0; i < 2 && *s1 && *s2; i++)
            {
                if (*s1 < *s2) return -1;
                if (*s1 > *s2) return 1;
                s1++;
                s2++;
            }
        }
        else
        {
            unsigned char c1 = *s1;
            unsigned char c2 = *s2;
            
            // Case insensitive: convert A-Z to a-z
            if (caseInsensitive)
            {
                if (c1 >= 'A' && c1 <= 'Z') c1 |= 0x20;
                if (c2 >= 'A' && c2 <= 'Z') c2 |= 0x20;
            }
            
            if (c1 != c2)
            {
                // If case-insensitive compare failed, use original bytes for ordering
                if (*s1 < *s2) return -1;
                return 1;
            }
            s1++;
            s2++;
        }
    }
    
    return 1;  // Strings are equal
}

/**
 * Copy string with automatic memory allocation
 * Takes src string and pointer to dest char*. Allocates memory for dest.
 */
void __cdecl RK_StringsCopyAuto(const char* src, char** destPtr)
{
    if (!destPtr) return;
    
    // Clear existing pointer
    *destPtr = NULL;
    
    if (!src) return;
    
    // Get string length
    size_t len = strlen(src);
    
    // Allocate memory (len + 1 for null terminator)
    char* newStr = (char*)GlobalAlloc(0, len + 1);
    if (!newStr) return;
    
    // Copy string
    memcpy(newStr, src, len + 1);
    *destPtr = newStr;
}

/**
 * Delete tabs and spaces from string based on mode
 * mode 0: strip leading spaces/tabs
 * mode 1: strip all spaces/tabs
 * mode 2: strip trailing spaces/tabs
 */
void __cdecl RK_DeleteTabSpaceString(char* str, int mode)
{
    if (!str) return;
    
    if (mode == 0)
    {
        // Strip leading spaces/tabs
        char* src = str;
        while (*src == ' ' || *src == '\t')
        {
            // Handle SJIS - if it looks like space but is SJIS, don't skip
            if (RK_CheckSJIS((unsigned char)*src))
                break;
            src++;
        }
        if (src != str)
        {
            // Shift string left
            char* dst = str;
            while (*src)
                *dst++ = *src++;
            *dst = '\0';
        }
    }
    else if (mode == 1)
    {
        // Strip all spaces/tabs
        char* src = str;
        char* dst = str;
        while (*src)
        {
            if (RK_CheckSJIS((unsigned char)*src))
            {
                // Copy both bytes of SJIS char
                *dst++ = *src++;
                if (*src) *dst++ = *src++;
            }
            else if (*src != ' ' && *src != '\t')
            {
                *dst++ = *src++;
            }
            else
            {
                src++;
            }
        }
        *dst = '\0';
    }
    else if (mode == 2)
    {
        // Strip trailing spaces/tabs
        size_t len = strlen(str);
        if (len == 0) return;
        
        char* end = str + len - 1;
        while (end >= str && (*end == ' ' || *end == '\t'))
            end--;
        end[1] = '\0';
    }
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
 * Copy string to dest with a maximum length, padding with spaces if shorter.
 * SJIS-aware: won't split a 2-byte character.
 * Args: src = source string, dest = destination buffer, maxLen = max chars to copy
 */
void __cdecl RK_StringCopyNumber(const char* src, char* dest, int maxLen)
{
    if (!dest) return;
    
    if (maxLen <= 0)
    {
        *dest = '\0';
        return;
    }
    
    if (!src)
    {
        // Fill with spaces
        memset(dest, ' ', maxLen);
        dest[maxLen] = '\0';
        return;
    }
    
    int copied = 0;
    int sjisCount = 1;  // Tracks position within SJIS char (1 or 2)
    
    while (copied < maxLen && *src)
    {
        if (RK_CheckSJIS((unsigned char)*src))
        {
            // If we're at maxLen-1 and this starts a 2-byte char, stop
            if (sjisCount == maxLen)
                break;
            
            // Copy first byte of SJIS
            dest[copied++] = *src++;
            sjisCount++;
            
            // Copy second byte if available and we have room
            if (copied < maxLen && *src)
            {
                dest[copied++] = *src++;
            }
        }
        else
        {
            // Check for null terminator
            if (*src == '\0')
                break;
            dest[copied++] = *src++;
        }
    }
    
    // Pad with spaces if we copied fewer than maxLen chars
    while (copied < maxLen)
    {
        dest[copied++] = ' ';
    }
    
    dest[copied] = '\0';
}

/**
 * Analyze filename and split into name (without extension) and extension
 * Handles special cases: "." and ".." are treated as names with no extension
 */
void __cdecl RK_AnalyzeFilename(const char* filename, char* nameOut, char* extOut)
{
    if (!filename || !nameOut || !extOut) return;
    
    // Special case: "."
    if (strcmp(filename, ".") == 0)
    {
        strcpy(nameOut, ".");
        extOut[0] = '\0';
        return;
    }
    
    // Special case: ".."
    if (strcmp(filename, "..") == 0)
    {
        strcpy(nameOut, "..");
        extOut[0] = '\0';
        return;
    }
    
    // Find last dot, handling SJIS
    const char* lastDot = NULL;
    const char* p = filename;
    
    while (*p)
    {
        if (RK_CheckSJIS((unsigned char)*p))
        {
            p++;  // Skip second byte of SJIS
            if (*p) p++;
            continue;
        }
        if (*p == '.')
            lastDot = p;
        p++;
    }
    
    // If no dot found, whole thing is the name
    if (!lastDot)
        lastDot = p;  // Point to end
    
    // Copy name (up to but not including last dot)
    size_t nameLen = lastDot - filename;
    memcpy(nameOut, filename, nameLen);
    nameOut[nameLen] = '\0';
    
    // Copy extension (after the dot, or empty if no dot)
    if (*lastDot == '.')
    {
        lastDot++;  // Skip the dot
        strcpy(extOut, lastDot);
    }
    else
    {
        extOut[0] = '\0';
    }
}

/**
 * Check if a file or directory exists
 * Returns: 0 = not found, 1 = regular file, 2 = directory
 * If findData is provided, receives the WIN32_FIND_DATA
 */
int __cdecl RK_CheckFileExist(const char* filename, WIN32_FIND_DATAA* findData)
{
    if (!filename) return 0;
    
    // Copy filename to local buffer and remove trailing slash
    char localPath[576];  // 0x240 bytes like original
    strcpy(localPath, filename);
    RK_CutLastRoot(localPath);
    
    // Use FindFirstFile to check existence
    WIN32_FIND_DATAA localFindData;
    HANDLE hFind = FindFirstFileA(localPath, &localFindData);
    
    if (hFind == INVALID_HANDLE_VALUE)
        return 0;  // Not found
    
    // Check if it's a directory
    int result;
    if (localFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        result = 2;  // Directory
    }
    else
    {
        result = 1;  // Regular file
    }
    
    FindClose(hFind);
    
    // Copy find data if caller wants it
    if (findData)
        memcpy(findData, &localFindData, sizeof(WIN32_FIND_DATAA));
    
    return result;
}

} // extern "C"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}

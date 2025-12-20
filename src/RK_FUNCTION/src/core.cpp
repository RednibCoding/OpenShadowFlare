/**
 * RK_FUNCTION - Core utility functions (incremental implementation)
 * 
 * Implements functions one by one. Unimplemented functions forward via dll.def.
 */

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstdio>

// Debug/tracing system - set to 1 to enable function call tracing
#define OSF_DEBUG 1
#define DLL_NAME "RK_FUNCTION"
#include "../../debug.h"

extern "C" {

/**
 * Check if a byte is a SJIS lead byte (first byte of 2-byte character)
 * Returns: 1 if SJIS lead byte, 0 otherwise
 */
int __cdecl RK_CheckSJIS(int ch)
{
    // Note: No tracing here - called too frequently, would spam logs
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
    OSF_FUNC_TRACE("str='%s'", str ? str : "(null)");
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
    OSF_FUNC_TRACE("path='%s'", path ? path : "(null)");
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
    OSF_FUNC_TRACE("'%s' vs '%s', ci=%d", 
              str1 ? str1 : "(null)", str2 ? str2 : "(null)", caseInsensitive);
    
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
    OSF_FUNC_TRACE("src='%s'", src ? src : "(null)");
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
    OSF_FUNC_TRACE("str='%s', mode=%d", str ? str : "(null)", mode);
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
    OSF_FUNC_TRACE("path='%s'", path ? path : "(null)");
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
    OSF_FUNC_TRACE("path='%s'", path ? path : "(null)");
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
    OSF_FUNC_TRACE("fullPath='%s'", fullPath ? fullPath : "(null)");
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
 * Extract filename from full path (remove directory) - MODIFIES IN PLACE
 * Finds last backslash and moves filename to start of buffer
 */
void __cdecl RK_CutDirectoryFromFullPath(char* path)
{
    OSF_FUNC_TRACE("path='%s'", path ? path : "(null)");
    if (!path || !*path) return;
    
    // Find last backslash (SJIS-aware)
    char* lastSlash = NULL;
    char* p = path;
    while (*p)
    {
        if (RK_CheckSJIS((unsigned char)*p))
        {
            p++;  // Skip first byte
            if (*p) p++;  // Skip second byte
            continue;
        }
        if (*p == '\\')
            lastSlash = p;
        p++;
    }
    
    // If found a backslash, move filename to start
    if (lastSlash)
    {
        char* src = lastSlash + 1;
        char* dst = path;
        while (*src)
            *dst++ = *src++;
        *dst = '\0';
    }
}

/**
 * Copy string to dest with a maximum length, padding with spaces if shorter.
 * SJIS-aware: won't split a 2-byte character.
 * Args: src = source string, dest = destination buffer, maxLen = max chars to copy
 */
void __cdecl RK_StringCopyNumber(const char* src, char* dest, int maxLen)
{
    OSF_FUNC_TRACE("src='%s', maxLen=%d", src ? src : "(null)", maxLen);
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
    OSF_FUNC_TRACE("filename='%s'", filename ? filename : "(null)");
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
 * Compare filename against a pattern with wildcards
 * Supports: ? = match any single char, * = match any sequence
 * Case-insensitive for ASCII letters, SJIS-aware
 * Returns: 1 if matches, 0 if not
 */
int __cdecl RK_FilenameCompareWildCard(const char* pattern, const char* filename)
{
    OSF_FUNC_TRACE("pattern='%s', filename='%s'", 
                   pattern ? pattern : "(null)", filename ? filename : "(null)");
    
    if (!pattern || !filename) return 0;
    
    // Find dot positions in pattern and filename (for extension handling)
    int patternLen = (int)strlen(pattern);
    int filenameLen = (int)strlen(filename);
    
    int patternDot = -1;  // Position of dot in pattern
    int filenameDot = -1; // Position of dot in filename
    
    // Find last dot in pattern
    for (int i = 0; i < patternLen; i++)
    {
        if (RK_CheckSJIS((unsigned char)pattern[i]))
        {
            i++;  // Skip second byte
            continue;
        }
        if (pattern[i] == '.')
            patternDot = i;
    }
    
    // Find last dot in filename
    for (int i = 0; i < filenameLen; i++)
    {
        if (RK_CheckSJIS((unsigned char)filename[i]))
        {
            i++;  // Skip second byte
            continue;
        }
        if (filename[i] == '.')
            filenameDot = i;
    }
    
    // If pattern has no dot but filename does (or vice versa with extension), 
    // they need special handling with wildcards
    if (patternDot == -1 && filenameDot != -1)
        return 0;
    if (patternDot != -1 && filenameDot == -1)
        return 0;
    
    // Match character by character
    int pi = 0;  // Pattern index
    int fi = 0;  // Filename index
    
    while (pattern[pi] != '\0')
    {
        if (filename[fi] == '\0')
        {
            // Filename ended but pattern still has chars
            // Only OK if remaining pattern is all *
            while (pattern[pi] == '*')
                pi++;
            return pattern[pi] == '\0' ? 1 : 0;
        }
        
        unsigned char pc = (unsigned char)pattern[pi];
        unsigned char fc = (unsigned char)filename[fi];
        
        // Handle SJIS characters in pattern
        if (RK_CheckSJIS(pc))
        {
            // Must match both bytes exactly
            if (pattern[pi] != filename[fi])
                return 0;
            pi++; fi++;
            if (pattern[pi] != filename[fi])
                return 0;
            pi++; fi++;
            continue;
        }
        
        // Handle wildcards
        if (pc == '?')
        {
            // Match any single character
            if (RK_CheckSJIS(fc))
            {
                // Skip both bytes of SJIS char
                pi++;
                fi += 2;
            }
            else
            {
                pi++;
                fi++;
            }
            continue;
        }
        
        if (pc == '*')
        {
            // * matches zero or more characters up to extension boundary
            pi++;
            
            // If we're before the dot in pattern and there's a dot in filename,
            // * only matches up to the dot
            if (patternDot != -1 && pi <= patternDot && filenameDot != -1 && fi < filenameDot)
            {
                // Move filename pointer up to the dot
                fi = filenameDot + 1;
                pi = patternDot + 1;
                continue;
            }
            
            // If at end of pattern, match everything
            if (pattern[pi] == '\0')
                return 1;
            
            // Try to match remaining pattern with rest of filename
            while (filename[fi] != '\0')
            {
                // Recursive-like check: try matching from current position
                const char* pp = &pattern[pi];
                const char* fp = &filename[fi];
                int matched = 1;
                
                while (*pp && *fp)
                {
                    if (*pp == '*' || *pp == '?')
                        break;  // Need more complex handling
                    
                    unsigned char c1 = (unsigned char)*pp;
                    unsigned char c2 = (unsigned char)*fp;
                    
                    // Case insensitive for ASCII
                    if (c1 >= 'A' && c1 <= 'Z') c1 |= 0x20;
                    if (c2 >= 'A' && c2 <= 'Z') c2 |= 0x20;
                    
                    if (c1 != c2)
                    {
                        matched = 0;
                        break;
                    }
                    pp++; fp++;
                }
                
                if (matched && (*pp == '\0' || *pp == '*' || *pp == '?'))
                {
                    // Found a match point, continue from here
                    pi = (int)(pp - pattern);
                    fi = (int)(fp - filename);
                    break;
                }
                
                fi++;
            }
            continue;
        }
        
        // Regular character - case insensitive compare
        unsigned char c1 = pc;
        unsigned char c2 = fc;
        
        if (c1 >= 'A' && c1 <= 'Z') c1 |= 0x20;
        if (c2 >= 'A' && c2 <= 'Z') c2 |= 0x20;
        
        if (c1 != c2)
            return 0;
        
        pi++;
        fi++;
    }
    
    // Pattern ended - filename should also have ended
    return (filename[fi] == '\0') ? 1 : 0;
}

/**
 * Check if a file or directory exists
 * Returns: 0 = not found, 1 = regular file, 2 = directory
 * If findData is provided, receives the WIN32_FIND_DATA
 */
int __cdecl RK_CheckFileExist(const char* filename, WIN32_FIND_DATAA* findData)
{
    OSF_FUNC_TRACE("%s", filename ? filename : "(null)");
    
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

/**
 * Enumerate files matching a pattern and return array of WIN32_FIND_DATAA
 * Args: pattern - file pattern (e.g., "*.txt"), outArray - receives pointer to allocated array
 * Returns: number of matching files (0 if none)
 * The array is allocated with GlobalAlloc and must be freed with RK_ReleaseFilesExist
 */
int __cdecl RK_CheckFilesExist(const char* pattern, WIN32_FIND_DATAA** outArray)
{
    OSF_FUNC_TRACE("pattern='%s'", pattern ? pattern : "(null)");
    
    if (!pattern || !outArray) return 0;
    
    *outArray = NULL;
    
    // Copy pattern to local buffer and remove trailing slash
    char localPath[0x244];
    strcpy(localPath, pattern);
    RK_CutLastRoot(localPath);
    
    // First pass: count matching files
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(localPath, &findData);
    
    if (hFind == INVALID_HANDLE_VALUE)
        return 0;
    
    int count = 1;  // We already found one
    while (FindNextFileA(hFind, &findData))
        count++;
    
    FindClose(hFind);
    
    // Allocate array: count * sizeof(WIN32_FIND_DATAA) = count * 0x140
    // Original uses: count * 5 * 64 = count * 320 = count * 0x140
    WIN32_FIND_DATAA* array = (WIN32_FIND_DATAA*)GlobalAlloc(0x40, count * sizeof(WIN32_FIND_DATAA));
    if (!array)
        return 0;
    
    // Second pass: fill the array
    hFind = FindFirstFileA(localPath, &array[0]);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        GlobalFree(array);
        return 0;
    }
    
    if (count > 1)
    {
        for (int i = 1; i < count; i++)
        {
            FindNextFileA(hFind, &array[i]);
        }
    }
    
    FindClose(hFind);
    
    *outArray = array;
    return count;
}

/**
 * Free array allocated by RK_CheckFilesExist
 * Args: arrayPtr - pointer to the WIN32_FIND_DATAA* that was filled by RK_CheckFilesExist
 */
void __cdecl RK_ReleaseFilesExist(WIN32_FIND_DATAA** arrayPtr)
{
    OSF_FUNC_TRACE_NOARGS;
    
    if (!arrayPtr) return;
    
    if (*arrayPtr)
    {
        GlobalFree(*arrayPtr);
    }
    
    *arrayPtr = NULL;
}

/**
 * Check if a string is quoted (starts and ends with double quote)
 * Args: str - string to check
 * Returns: 1 if quoted, 0 otherwise
 */
int __cdecl RK_MesDefineCheck(const char* str)
{
    OSF_FUNC_TRACE("str='%s'", str ? str : "(null)");
    if (!str) return 0;
    
    // Must start with quote
    if (str[0] != '"') return 0;
    
    // Get length (skip first char)
    size_t len = strlen(str + 1);
    if (len == 0) return 0;
    
    // Check if last char is quote
    return (str[len] == '"') ? 1 : 0;
}

/**
 * Remove surrounding quotes from a string if present
 * Args: str - string to modify in place
 */
void __cdecl RK_MesDefineCut(char* str)
{
    OSF_FUNC_TRACE("str='%s'", str ? str : "(null)");
    if (!str) return;
    
    // If doesn't start with quote, nothing to do
    if (str[0] != '"') return;
    
    size_t len = strlen(str);
    if (len == 0) return;
    
    // Shift string left by 1 to remove leading quote
    for (size_t i = 0; i < len; i++)
    {
        str[i] = str[i + 1];
    }
    
    // Now check if it ends with quote and remove it
    len = strlen(str);
    if (len > 0 && str[len - 1] == '"')
    {
        str[len - 1] = '\0';
    }
}

/**
 * Add surrounding quotes to a string if not already quoted
 * Args: str - string to modify in place (must have room for 2 extra chars)
 */
void __cdecl RK_MesDefineSet(char* str)
{
    OSF_FUNC_TRACE("str='%s'", str ? str : "(null)");
    if (!str) return;
    
    // If already starts with quote, check if also ends with quote
    if (str[0] == '"')
    {
        size_t len = strlen(str);
        if (len > 0 && str[len - 1] == '"')
            return;  // Already fully quoted
        
        // Ends without quote - add closing quote
        str[len] = '"';
        str[len + 1] = '\0';
        return;
    }
    
    // Not quoted - shift right and add quotes
    size_t len = strlen(str);
    
    // Shift all chars right by 1
    for (size_t i = len + 1; i > 0; i--)
    {
        str[i] = str[i - 1];
    }
    
    // Add opening quote
    str[0] = '"';
    
    // Add closing quote
    str[len + 1] = '"';
    str[len + 2] = '\0';
}

/**
 * Check if a drive letter is valid/available
 * Args: driveLetter - drive letter character (e.g., 'C', 'D')
 * Returns: 1 if drive exists, 0 if not
 */
int __cdecl RK_CheckDriveEffective(int driveLetter)
{
    OSF_FUNC_TRACE("driveLetter='%c'", (char)driveLetter);
    DWORD drives = GetLogicalDrives();
    
    // Convert to lowercase and get drive index (0-25)
    int index = (driveLetter | 0x20) - 'a';
    
    // Create bitmask for this drive
    DWORD mask = 1 << index;
    
    // Return 1 if drive exists, 0 otherwise
    return (drives & mask) ? 1 : 0;
}

/**
 * Get file's last write time as SYSTEMTIME
 * Args: filename, pointer to SYSTEMTIME to receive result
 * Returns: 1 on success, 0 on failure
 */
int __cdecl RK_GetFileLastWrite(const char* filename, SYSTEMTIME* sysTime)
{
    OSF_FUNC_TRACE("filename='%s'", filename ? filename : "(null)");
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(filename, &findData);
    
    if (hFind == INVALID_HANDLE_VALUE)
        return 0;
    
    FindClose(hFind);
    
    // Convert file time to local file time
    FILETIME localFileTime;
    FileTimeToLocalFileTime(&findData.ftLastWriteTime, &localFileTime);
    
    // Convert local file time to system time
    FileTimeToSystemTime(&localFileTime, sysTime);
    
    return 1;
}

/**
 * Set file's last write time from SYSTEMTIME
 * Args: filename, pointer to SYSTEMTIME with new time
 * Returns: 1 on success, 0 on failure
 */
int __cdecl RK_SetFileLastWrite(const char* filename, const SYSTEMTIME* sysTime)
{
    OSF_FUNC_TRACE("filename='%s'", filename ? filename : "(null)");
    // Convert SYSTEMTIME to FILETIME (local)
    FILETIME localFileTime;
    if (!SystemTimeToFileTime(sysTime, &localFileTime))
        return 0;
    
    // Convert local file time to UTC file time
    FILETIME utcFileTime;
    if (!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
        return 0;
    
    // Open the file for writing attributes
    // 0xC0000000 = GENERIC_READ | GENERIC_WRITE
    // 0x80 = FILE_ATTRIBUTE_NORMAL
    // 3 = OPEN_EXISTING
    HANDLE hFile = CreateFileA(filename, 0xC0000000, 0, NULL, 3, 0x80, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;
    
    // Set the last write time (pass NULL for creation/access times)
    BOOL result = SetFileTime(hFile, NULL, NULL, &utcFileTime);
    CloseHandle(hFile);
    
    return result ? 1 : 0;
}

/**
 * Compare two SYSTEMTIME structures
 * Returns: 1 if time1 > time2, -1 if time1 < time2, 0 if equal
 */
int __cdecl RK_SystemTimeCompare(const SYSTEMTIME* time1, const SYSTEMTIME* time2)
{
    OSF_FUNC_TRACE_NOARGS;
    // Compare year
    if (time1->wYear > time2->wYear) return 1;
    if (time1->wYear < time2->wYear) return -1;
    
    // Compare month
    if (time1->wMonth > time2->wMonth) return 1;
    if (time1->wMonth < time2->wMonth) return -1;
    
    // Compare day (skip wDayOfWeek)
    if (time1->wDay > time2->wDay) return 1;
    if (time1->wDay < time2->wDay) return -1;
    
    // Compare hour
    if (time1->wHour > time2->wHour) return 1;
    if (time1->wHour < time2->wHour) return -1;
    
    // Compare minute
    if (time1->wMinute > time2->wMinute) return 1;
    if (time1->wMinute < time2->wMinute) return -1;
    
    // Compare second
    if (time1->wSecond > time2->wSecond) return 1;
    if (time1->wSecond < time2->wSecond) return -1;
    
    // Compare milliseconds
    if (time1->wMilliseconds > time2->wMilliseconds) return 1;
    if (time1->wMilliseconds < time2->wMilliseconds) return -1;
    
    return 0;  // Equal
}

} // extern "C"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            OSF_DEBUG_INIT();
            OSF_TRACE("DllMain", "PROCESS_ATTACH");
            break;
        case DLL_PROCESS_DETACH:
            OSF_TRACE("DllMain", "PROCESS_DETACH");
            OSF_DEBUG_SHUTDOWN();
            break;
    }
    return TRUE;
}

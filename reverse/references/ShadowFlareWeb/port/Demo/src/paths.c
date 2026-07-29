#include "paths.h"


int DeriveCharacterRoot(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DerivePlayerRoot(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveItemIbnPath(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveTablePath(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveHudBarPath(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveBloodDecalPath(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveBgmPath(const char *mapRoot, long bgmIndex, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written =
        
    return written > 0 && (size_t)written < outSize;
}


int DeriveStatusSheetPath(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveStatusIconSheetPath(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveMagicBarIconPath(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveDarknessPath(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveItemIconSheetPath(const char *mapRoot, int sheetIndex, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written =
        
    return written > 0 && (size_t)written < outSize;
}


int DeriveCursorSheetPath(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveHukidasiPath(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveFontPath(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveControlAidPath(const char *mapRoot, const char *aidPathField, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;

    char relPath[0x104];
    size_t i = 0;
    for (; aidPathField[i] != '\0' && i < sizeof(relPath) - 1; i++)
        relPath[i] = aidPathField[i] == '\\' ? '/' : aidPathField[i];
    relPath[i] = '\0';

    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveScenarioPath(const char *mapRoot, long scenarioId, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


long ParseScenarioIdFromPath(const char *scenarioDir)
{
    size_t len = strlen(scenarioDir);
    while (len > 0 && (scenarioDir[len - 1] == '/' || scenarioDir[len - 1] == '\\'))
        len--;
    if (len < 8)
        return -1;

    const char *digits = scenarioDir + len - 8;
    long id = 0;
    for (int i = 0; i < 8; i++)
    {
        if (digits[i] < '0' || digits[i] > '9')
            return -1;
        id = id * 10 + (digits[i] - '0');
    }
    return id;
}


int DeriveWaitingSheetPath(const char *mapRoot, char *outBuf, size_t outSize)
{
    size_t len = strlen(mapRoot);
    if (len < 4)
        return 0;
    const char *tail = mapRoot + len - 4;
    if (strcmp(tail, "/Map") != 0 && strcmp(tail, "\\Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}


int DeriveMapStem(const char *mapPathField, char *outBuf, size_t outSize)
{
    static const char prefix[] = "Map\\";
    size_t prefixLen = sizeof(prefix) - 1;
    size_t len = strlen(mapPathField);
    if (len <= prefixLen + 4 || strncmp(mapPathField, prefix, prefixLen) != 0)
        return 0;
    const char *ext = mapPathField + len - 4;
    if (strcmp(ext, ".map") != 0 && strcmp(ext, ".Map") != 0)
        return 0;
    int written = 
    return written > 0 && (size_t)written < outSize;
}

/**
 * RKC_RPGSCRN - RPG Screen rendering
 * 
 * Classes: RKC_RPGSCRN, RKC_RPGSCRN_OBJECT, RKC_RPGSCRN_OBJECTDISP,
 *          RKC_RPGSCRN_OBJECTDISPCELL, RKC_RPGSCRN_OBJECTBLOCK,
 *          RKC_RPGSCRN_GROUNDBLOCK, RKC_RPGSCRN_CHARANIM*, etc.
 */

#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

extern "C" {

void* __thiscall RKC_RPGSCRN_OBJECT_constructor(void* self);
void* __thiscall RKC_RPGSCRN_OBJECTBLOCK_constructor(void* self);
void __thiscall RKC_RPGSCRN_OBJECTBLOCK_destructor(void* self);
void* __thiscall RKC_RPGSCRN_OBJECTDISPCELL_constructor(void* self);
void* __thiscall RKC_RPGSCRN_GROUNDBLOCK_constructor(void* self);
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_destructor(void* self);
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_Release(void* self);
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetAreaSize(void* self, long width, long height);
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetAreaJudgeSize(
    void* self, long width, long height, long offsetX, long offsetY);
void* __thiscall RKC_RPGSCRN_CHARANIM_constructor(void* self);
void __thiscall RKC_RPGSCRN_CHARANIM_destructor(void* self);
void* __thiscall RKC_RPGSCRN_CHARANIMBLOCK_constructor(void* self);
void __thiscall RKC_RPGSCRN_CHARANIMBLOCK_destructor(void* self);
void* __thiscall RKC_RPGSCRN_CHARANIMCELL_constructor(void* self);
void* __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_constructor(void* self);
void __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_destructor(void* self);
void* __thiscall RKC_RPGSCRN_CHARANIMCHART_constructor(void* self);
void __thiscall RKC_RPGSCRN_CHARANIMCHART_destructor(void* self);
void __thiscall RKC_RPGSCRN_Release(void* self);
void __thiscall RKC_RPGSCRN_OBJECTBLOCK_Release(void* self);
void __thiscall RKC_RPGSCRN_OBJECTDISP_Release(void* self);
void __thiscall RKC_RPGSCRN_CHARANIM_Release(void* self);
int __thiscall RKC_RPGSCRN_CHARANIM_ReadCafFile(void* self, char* path, long animationNo);
long __thiscall RKC_RPGSCRN_CHARANIM_GetMaxPartsCount(void* self);
void __thiscall RKC_RPGSCRN_CHARANIMBLOCK_Release(void* self, long index);
void __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_Release(void* self);
void __thiscall RKC_RPGSCRN_CHARANIMCHART_Release(void* self);
void* __thiscall RKC_RPGSCRN_GetObjectBlock(void* self, long index);
void* __thiscall RKC_RPGSCRN_GetGroundBlock(void* self, long index);
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_ReadFile(
    void* self, char* path, void* display, int append, long updOffset);
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_ReadFile(
    void* self, char* path, long updOffset);

static void* AllocateObject(SIZE_T size)
{
    return ::operator new(size, std::nothrow);
}

static void FreeObject(void* object)
{
    ::operator delete(object);
}

static FARPROC LoadUpdFunction(const char* name)
{
    HMODULE module = GetModuleHandleA("RKC_UPDIB.dll");
    if (!module)
        module = LoadLibraryA("RKC_UPDIB.dll");
    return module ? GetProcAddress(module, name) : nullptr;
}

static void* GetUpdPattern(void* screen, short updNo, long patternNo)
{
    if (!screen || updNo < 0 || !*(void**)screen)
        return nullptr;
    using GetUpd = void* (__thiscall*)(void*, long);
    using GetPattern = void* (__thiscall*)(void*, long);
    static GetUpd getUpd = reinterpret_cast<GetUpd>(LoadUpdFunction(
        "?GetUpd@RKC_UPDIB@@QAEPAVRKC_UPDIB_UPD@@J@Z"));
    static GetPattern getPattern = reinterpret_cast<GetPattern>(LoadUpdFunction(
        "?GetPattern@RKC_UPDIB_UPD@@QAEPAVRKC_UPDIB_PATTERN@@J@Z"));
    if (!getUpd || !getPattern)
        return nullptr;
    void* upd = getUpd(*(void**)screen, updNo);
    return upd ? getPattern(upd, patternNo) : nullptr;
}

static RECT* GetPatternBuildRect(void* pattern)
{
    using Function = RECT* (__thiscall*)(void*);
    static Function function = reinterpret_cast<Function>(LoadUpdFunction(
        "?GetBuildRect@RKC_UPDIB_PATTERN@@QAEPAUtagRECT@@XZ"));
    return function && pattern ? function(pattern) : nullptr;
}

static bool ReadEncodedBlock(std::FILE* file, std::vector<unsigned char>& output)
{
    unsigned char compressed = 0;
    if (std::fread(&compressed, 1, 1, file) != 1)
        return false;
    if (!compressed)
        return output.empty() ||
               std::fread(output.data(), 1, output.size(), file) == output.size();
    unsigned char header[16];
    if (std::fread(header, 1, sizeof(header), file) != sizeof(header))
        return false;
    long packedSize = *reinterpret_cast<long*>(header + 12);
    if (packedSize < 0)
        return false;
    std::vector<unsigned char> packed(
        static_cast<std::size_t>(packedSize) + sizeof(header));
    std::memcpy(packed.data(), header, sizeof(header));
    if (packedSize &&
        std::fread(packed.data() + sizeof(header), 1, packedSize, file) !=
            static_cast<std::size_t>(packedSize))
        return false;
    using Decode = int (__cdecl*)(void*, long, void**, void*);
    HMODULE module = GetModuleHandleA("RK_FUNCTION.dll");
    if (!module)
        module = LoadLibraryA("RK_FUNCTION.dll");
    Decode decode = module ? reinterpret_cast<Decode>(
        GetProcAddress(module, "RK_LzDecodeMemoryToMemory")) : nullptr;
    void* decoded = nullptr;
    unsigned char decodedHeader[16] = {};
    const int decodeResult = decode
        ? decode(
              packed.data(), static_cast<long>(packed.size()),
              &decoded, decodedHeader)
        : 0;
    const long decodedSize = *reinterpret_cast<long*>(decodedHeader + 8);
    if (decodeResult != 1 || !decoded ||
        decodedSize < static_cast<long>(output.size())) {
        if (decoded)
            GlobalFree(decoded);
        return false;
    }
    if (!output.empty())
        std::memcpy(output.data(), decoded, output.size());
    GlobalFree(decoded);
    return true;
}

// ============================================================================
// LOCAL RECONSTRUCTION
// Object ownership, map and animation readers, sorting, packet creation, and
// display operations are implemented below.
// ============================================================================

// RKC_RPGSCRN
void* __thiscall RKC_RPGSCRN_constructor(void* self) {
    RKC_RPGSCRN_CHARANIMBLOCK_constructor((char*)self + 0xc);
    *(void**)self = nullptr;
    *(void**)((char*)self + 4) = nullptr;
    *(void**)((char*)self + 8) = nullptr;
    *(long*)((char*)self + 0x18) = 15;
    *(long*)((char*)self + 0x1c) = 10;
    *(long*)((char*)self + 0x20) = 10;
    *(long*)((char*)self + 0x24) = 0;
    *(int*)((char*)self + 0x28) = 1;
    return self;
}
void __thiscall RKC_RPGSCRN_destructor(void* self) {
    RKC_RPGSCRN_Release(self);
    RKC_RPGSCRN_CHARANIMBLOCK_destructor((char*)self + 0xc);
}
void* __thiscall RKC_RPGSCRN_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 0x2c);
    return self;
}
int __thiscall RKC_RPGSCRN_DeleteGroundBlock(void* self, long index) {
    char* item = *(char**)((char*)self + 8);
    for (long current = 0; item && current != index; ++current)
        item = *(char**)(item + 0x38);
    if (!item)
        return 0;
    char* previous = *(char**)(item + 0x34);
    char* next = *(char**)(item + 0x38);
    if (previous)
        *(char**)(previous + 0x38) = next;
    else
        *(char**)((char*)self + 8) = next;
    if (next)
        *(char**)(next + 0x34) = previous;
    RKC_RPGSCRN_GROUNDBLOCK_destructor(item);
    FreeObject(item);
    return 1;
}
int __thiscall RKC_RPGSCRN_DeleteObjectBlock(void* self, long index) {
    char* item = *(char**)((char*)self + 4);
    for (long current = 0; item && current != index; ++current)
        item = *(char**)(item + 0xc);
    if (!item)
        return 0;
    char* previous = *(char**)(item + 8);
    char* next = *(char**)(item + 0xc);
    if (previous)
        *(char**)(previous + 0xc) = next;
    else
        *(char**)((char*)self + 4) = next;
    if (next)
        *(char**)(next + 8) = previous;
    RKC_RPGSCRN_OBJECTBLOCK_destructor(item);
    FreeObject(item);
    return 1;
}
void __thiscall RKC_RPGSCRN_GetBaseParam(void* self, long* a, long* b, long* c) {
    *a = *(long*)((char*)self + 0x18);
    *b = *(long*)((char*)self + 0x1c);
    *c = *(long*)((char*)self + 0x20);
}
long __thiscall RKC_RPGSCRN_GetGroundBlockCount(void* self) {
    long count = 0;
    for (char* item = *(char**)((char*)self + 8); item;
         item = *(char**)(item + 0x38))
        ++count;
    return count;
}
void* __thiscall RKC_RPGSCRN_GetGroundBlock(void* self, long index) {
    char* item = *(char**)((char*)self + 8);
    for (long current = 0; item && current != index; ++current)
        item = *(char**)(item + 0x38);
    return item;
}
int __thiscall RKC_RPGSCRN_GetShadowTransFlag(void* self) {
    return *(int*)((char*)self + 0x28);
}
long __thiscall RKC_RPGSCRN_GetObjectBlockCount(void* self) {
    long count = 0;
    for (char* item = *(char**)((char*)self + 4); item;
         item = *(char**)(item + 0xc))
        ++count;
    return count;
}
long __thiscall RKC_RPGSCRN_GetGroundBlockNo(void* self, void* block) {
    long index = 0;
    for (char* item = *(char**)((char*)self + 8); item;
         item = *(char**)(item + 0x38), ++index)
        if (item == block)
            return index;
    return -1;
}
void* __thiscall RKC_RPGSCRN_GetObjectBlock(void* self, long index) {
    char* item = *(char**)((char*)self + 4);
    for (long current = 0; item && current != index; ++current)
        item = *(char**)(item + 0xc);
    return item;
}
long __thiscall RKC_RPGSCRN_GetObjectBlockNo(void* self, void* block) {
    long index = 0;
    for (char* item = *(char**)((char*)self + 4); item;
         item = *(char**)(item + 0xc), ++index)
        if (item == block)
            return index;
    return -1;
}
void* __thiscall RKC_RPGSCRN_GetCharAnimBlock(void* self) {
    return (char*)self + 0xc;
}
void* __thiscall RKC_RPGSCRN_InsertObjectBlock(void* self, long index) {
    const long count = RKC_RPGSCRN_GetObjectBlockCount(self);
    if (index < 0 || index > count)
        return nullptr;
    char* object = static_cast<char*>(AllocateObject(0x10));
    if (!object)
        return nullptr;
    RKC_RPGSCRN_OBJECTBLOCK_constructor(object);
    *(void**)object = self;
    char* next = *(char**)((char*)self + 4);
    for (long current = 0; current < index; ++current)
        next = *(char**)(next + 0xc);
    char* previous = next ? *(char**)(next + 8) : nullptr;
    if (!next) {
        previous = *(char**)((char*)self + 4);
        while (previous && *(char**)(previous + 0xc))
            previous = *(char**)(previous + 0xc);
    }
    *(char**)(object + 8) = previous;
    *(char**)(object + 0xc) = next;
    if (previous)
        *(char**)(previous + 0xc) = object;
    else
        *(char**)((char*)self + 4) = object;
    if (next)
        *(char**)(next + 8) = object;
    return object;
}
void* __thiscall RKC_RPGSCRN_InsertGroundBlock(void* self, long index) {
    const long count = RKC_RPGSCRN_GetGroundBlockCount(self);
    if (index < 0 || index > count)
        return nullptr;
    char* ground = static_cast<char*>(AllocateObject(0x3c));
    if (!ground)
        return nullptr;
    RKC_RPGSCRN_GROUNDBLOCK_constructor(ground);
    *(void**)ground = self;
    char* next = *(char**)((char*)self + 8);
    for (long current = 0; current < index; ++current)
        next = *(char**)(next + 0x38);
    char* previous = next ? *(char**)(next + 0x34) : nullptr;
    if (!next) {
        previous = *(char**)((char*)self + 8);
        while (previous && *(char**)(previous + 0x38))
            previous = *(char**)(previous + 0x38);
    }
    *(char**)(ground + 0x34) = previous;
    *(char**)(ground + 0x38) = next;
    if (previous)
        *(char**)(previous + 0x38) = ground;
    else
        *(char**)((char*)self + 8) = ground;
    if (next)
        *(char**)(next + 0x34) = ground;
    return ground;
}
void __thiscall RKC_RPGSCRN_Release(void* self) {
    while (RKC_RPGSCRN_DeleteGroundBlock(self, 0)) {}
    while (RKC_RPGSCRN_DeleteObjectBlock(self, 0)) {}
    RKC_RPGSCRN_CHARANIMBLOCK_Release((char*)self + 0xc, -1);
}
int __thiscall RKC_RPGSCRN_SetUpdibHost(void* self, void* updib) {
    if (!updib)
        return 0;
    *(void**)self = updib;
    return 1;
}
int __thiscall RKC_RPGSCRN_ReadUpdList(
    void* self, char* patternDirectory, char* listPath,
    long updOffset, int createTemporary) {
    std::FILE* file = listPath ? std::fopen(listPath, "rb") : nullptr;
    if (!file || !*(void**)self) {
        if (file) std::fclose(file);
        return 0;
    }
    using ReadUpd = int (__thiscall*)(void*, long, char*, long, long, long, int);
    using CreateTemporary = void (__thiscall*)(void*);
    static ReadUpd readUpd = reinterpret_cast<ReadUpd>(LoadUpdFunction(
        "?ReadUpd@RKC_UPDIB@@QAEHJPADJJJH@Z"));
    static CreateTemporary create = reinterpret_cast<CreateTemporary>(
        LoadUpdFunction("?CreateTemporaryDIB@RKC_UPDIB@@QAEXXZ"));
    if (!readUpd) {
        std::fclose(file);
        return 0;
    }
    char line[512];
    long index = 0;
    bool found = false;
    bool ok = true;
    while (std::fgets(line, sizeof(line), file)) {
        char* end = line + std::strlen(line);
        while (end > line && static_cast<unsigned char>(end[-1]) < 0x20)
            *--end = '\0';
        char* name = line;
        while (*name && static_cast<unsigned char>(*name) < 0x20)
            ++name;
        if (*name) {
            found = true;
            if (std::strcmp(name, "?") != 0) {
                std::string full = patternDirectory ? patternDirectory : "";
                if (!full.empty() && full.back() != '\\' && full.back() != '/')
                    full += '\\';
                full += name;
                std::vector<char> mutablePath(full.begin(), full.end());
                mutablePath.push_back('\0');
                if (!readUpd(
                        *(void**)self, updOffset + index, mutablePath.data(),
                        0, 0, createTemporary, 1)) {
                    ok = false;
                    break;
                }
            }
            ++index;
        }
    }
    std::fclose(file);
    if (ok && found && createTemporary && create)
        create(*(void**)self);
    return ok && found ? 1 : 0;
}
int __thiscall RKC_RPGSCRN_ReadMapFile(
    void* self, char* path, long groundBlockNo, long objectBlockNo,
    long updOffset, int createTemporary, unsigned short sections) {
    std::FILE* file = path ? std::fopen(path, "rb") : nullptr;
    char header[16];
    long present[4];
    if (!file ||
        std::fread(header, 1, sizeof(header), file) != sizeof(header) ||
        std::memcmp(header, "MAPED_FILE", 10) != 0 ||
        std::fread(present, sizeof(long), 4, file) != 4) {
        if (file) std::fclose(file);
        return 0;
    }
    std::fclose(file);
    std::string fullPath(path);
    const std::size_t slash = fullPath.find_last_of("\\/");
    const std::string directory =
        slash == std::string::npos ? std::string() : fullPath.substr(0, slash + 1);
    std::string filename =
        slash == std::string::npos ? fullPath : fullPath.substr(slash + 1);
    const std::size_t dot = filename.find_last_of('.');
    if (dot != std::string::npos)
        filename.resize(dot);
    auto makePath = [&](const char* subdirectory, const char* extension) {
        return directory + subdirectory + "\\" + filename + extension;
    };
    if (present[0] && (sections & 8)) {
        std::string patternDir = directory + "Pattern";
        std::string list = makePath("Pattern", ".Lst");
        std::vector<char> patternChars(patternDir.begin(), patternDir.end());
        std::vector<char> listChars(list.begin(), list.end());
        patternChars.push_back('\0');
        listChars.push_back('\0');
        if (!RKC_RPGSCRN_ReadUpdList(
                self, patternChars.data(), listChars.data(),
                updOffset, createTemporary))
            return 0;
    }
    if (present[1] && (sections & 2)) {
        void* block = RKC_RPGSCRN_GetObjectBlock(self, objectBlockNo);
        std::string object = makePath("Object", ".Obl");
        std::vector<char> objectChars(object.begin(), object.end());
        objectChars.push_back('\0');
        if (!block || !RKC_RPGSCRN_OBJECTBLOCK_ReadFile(
                block, objectChars.data(), nullptr, 0, updOffset))
            return 0;
    }
    if (present[3] && (sections & 1)) {
        void* block = RKC_RPGSCRN_GetGroundBlock(self, groundBlockNo);
        std::string ground = makePath("Ground", ".Gnd");
        std::vector<char> groundChars(ground.begin(), ground.end());
        groundChars.push_back('\0');
        if (!block || !RKC_RPGSCRN_GROUNDBLOCK_ReadFile(
                block, groundChars.data(), updOffset))
            return 0;
    }
    return 1;
}
void __thiscall RKC_RPGSCRN_SetBaseParam(void* self, long a, long b, long c) {
    *(long*)((char*)self + 0x18) = a;
    *(long*)((char*)self + 0x1c) = b;
    *(long*)((char*)self + 0x20) = c;
}
void __thiscall RKC_RPGSCRN_CalcRealPos(
    void* self, long x, long y, long* outX, long* outY) {
    *outX = ((x - y) * *(long*)((char*)self + 0x18)) / 100;
    *outY = ((x + y) * *(long*)((char*)self + 0x1c)) / 100;
}
void __thiscall RKC_RPGSCRN_CalcWorldPos(
    void* self, long x, long y, long* outX, long* outY) {
    const long baseX = *(long*)((char*)self + 0x18);
    const long baseY = *(long*)((char*)self + 0x1c);
    *outX = ((baseX * y + baseY * x) * 100) / (baseX * baseY * 2);
    *outY = ((baseX * y - baseY * x) * 100) / (baseX * baseY * 2);
    if (x * baseY + y * baseX < 0)
        --*outX;
    if (y * baseX - x * baseY < 0)
        --*outY;
}
void __thiscall RKC_RPGSCRN_SetShadowTransFlag(void* self, int value) {
    *(int*)((char*)self + 0x28) = value;
}

// RKC_RPGSCRN_OBJECT
void* __thiscall RKC_RPGSCRN_OBJECT_constructor(void* self) {
    auto* bytes = static_cast<unsigned char*>(self);
    *(short*)(bytes + 0x10) = 0;
    *(long*)(bytes + 0x14) = 0;
    *(short*)(bytes + 0x18) = -1;
    *(long*)(bytes + 4) = 0;
    *(long*)(bytes + 8) = 0;
    *(short*)(bytes + 0x1a) = 1000;
    std::memset(bytes + 0x28, 0, 0x10);
    *(long*)(bytes + 0x38) = 0;
    *(long*)(bytes + 0x3c) = 0;
    *(short*)(bytes + 0x1c) = 0;
    *(short*)(bytes + 0x1e) = 0;
    *(short*)(bytes + 0x20) = 1000;
    *(short*)(bytes + 0x22) = 1000;
    *(short*)(bytes + 0x24) = 1000;
    *(long*)(bytes + 0xc) = -1;
    return self;
}
void* __thiscall RKC_RPGSCRN_OBJECT_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 0x40);
    return self;
}
int __thiscall RKC_RPGSCRN_OBJECT_Copy(void* self, void* src) {
    std::memcpy(self, src, 0x38);
    return 1;
}
short __thiscall RKC_RPGSCRN_OBJECT_GetPaletteNo(void* self) { return *(short*)((char*)self + 0x18); }
long __thiscall RKC_RPGSCRN_OBJECT_GetPatternNo(void* self) { return *(long*)((char*)self + 0x14); }
short __thiscall RKC_RPGSCRN_OBJECT_GetTrans(void* self) { return *(short*)((char*)self + 0x1a); }
short __thiscall RKC_RPGSCRN_OBJECT_GetUpdNo(void* self) { return *(short*)((char*)self + 0x10); }
long __thiscall RKC_RPGSCRN_OBJECT_GetX(void* self) { return *(long*)((char*)self + 4); }
long __thiscall RKC_RPGSCRN_OBJECT_GetY(void* self) { return *(long*)((char*)self + 8); }
long __thiscall RKC_RPGSCRN_OBJECT_GetCharacterNo(void* self) { return *(long*)((char*)self + 0xc); }
short __thiscall RKC_RPGSCRN_OBJECT_GetStatus(void* self) { return *(short*)((char*)self + 0x1c); }
short __thiscall RKC_RPGSCRN_OBJECT_GetHeight(void* self) { return *(short*)((char*)self + 0x1e); }
short __thiscall RKC_RPGSCRN_OBJECT_GetRStrong(void* self) { return *(short*)((char*)self + 0x20); }
short __thiscall RKC_RPGSCRN_OBJECT_GetGStrong(void* self) { return *(short*)((char*)self + 0x22); }
short __thiscall RKC_RPGSCRN_OBJECT_GetBStrong(void* self) { return *(short*)((char*)self + 0x24); }
void* __thiscall RKC_RPGSCRN_OBJECT_GetPositionAddr(void* self) { return (char*)self + 4; }
void* __thiscall RKC_RPGSCRN_OBJECT_GetJudgementAddr(void* self) { return (char*)self + 0x28; }
int __thiscall RKC_RPGSCRN_OBJECT_GetPosition(void* self, void* out) {
    std::memcpy(out, (char*)self + 4, 8);
    return 1;
}
int __thiscall RKC_RPGSCRN_OBJECT_GetJudgement(void* self, void* out) {
    std::memcpy(out, (char*)self + 0x28, 0x10);
    return 1;
}
void __thiscall RKC_RPGSCRN_OBJECT_Release(void* self) {}
int __thiscall RKC_RPGSCRN_OBJECT_SetPacket(
    void* self, long vsBlockNo, long vsNo, long offsetX, long offsetY,
    int mode, short transScale, void* position) {
    char* object = static_cast<char*>(self);
    char* screen = *(char**)object;
    if (!screen || !*(void**)screen)
        return 0;
    using GetBlock = void* (__thiscall*)(void*, long);
    using GetVs = void* (__thiscall*)(void*, long);
    using SetPacket = void* (__thiscall*)(
        void*, long, long, long, long, long, long, long, long, long, long,
        long, long, short, short, short, RECT*, void*);
    static GetBlock getBlock = reinterpret_cast<GetBlock>(LoadUpdFunction(
        "?GetVSBlock@RKC_UPDIB@@QAEPAVRKC_UPDIB_VSBLOCK@@J@Z"));
    static GetVs getVs = reinterpret_cast<GetVs>(LoadUpdFunction(
        "?GetVScreen@RKC_UPDIB_VSBLOCK@@QAEPAVRKC_UPDIB_VS@@J@Z"));
    static SetPacket setPacket = reinterpret_cast<SetPacket>(LoadUpdFunction(
        "?SetPacket@RKC_UPDIB_VS@@QAEPAVRKC_UPDIB_VSPACKET@@JJJJJJJJJJJJFFFPAUtagRECT@@PAVRKC_DIB@@@Z"));
    void* block = getBlock ? getBlock(*(void**)screen, vsBlockNo) : nullptr;
    void* vs = block && getVs ? getVs(block, vsNo) : nullptr;
    if (!vs || !setPacket)
        return 0;
    if (*(short*)(object + 0x10) < 0)
        return 1;
    long screenX = 0, screenY = 0;
    if (position) {
        screenX = *(long*)position;
        screenY = *(long*)((char*)position + 4);
        mode = mode == 1;
    } else {
        RKC_RPGSCRN_CalcRealPos(
            screen, *(long*)(object + 4), *(long*)(object + 8),
            &screenX, &screenY);
        if (mode != 1)
            screenY += (*(short*)(object + 0x1e) *
                        *(long*)(screen + 0x20)) / 100;
    }
    unsigned long flags = 0;
    const unsigned short status = *(unsigned short*)(object + 0x1c);
    if (status & 0x200) flags |= 4;
    long palette = *(short*)(object + 0x18);
    short trans = static_cast<short>(
        (*(short*)(object + 0x1a) * transScale) / 1000);
    short red = *(short*)(object + 0x20);
    short green = *(short*)(object + 0x22);
    short blue = *(short*)(object + 0x24);
    if (mode == 0) {
        if (status & 0x10) flags |= 2;
        if (status & 4) flags |= 8;
        if (status & 0x4000) flags |= 0x2000;
    } else {
        trans = *(long*)(screen + 0x28) == 1 ? 500 : 1000;
        palette = mode == 1 ? -1 : palette;
        red = green = blue = 1000;
    }
    setPacket(
        vs, 0, *(short*)(object + 0x10) + (mode == 1),
        *(long*)(object + 0x14), palette, flags,
        screenX - offsetX, screenY - offsetY,
        1000, 1000, trans, 1000, 0, red, green, blue, nullptr, nullptr);
    return 1;
}
void __thiscall RKC_RPGSCRN_OBJECT_SetParam(
    void* self, short upd, long pattern, short palette, long x, long y,
    short trans, short status, short height, void* judgement, void* screen,
    short red, short green, short blue, long character) {
    char* object = static_cast<char*>(self);
    *(short*)(object + 0x10) = upd;
    *(long*)(object + 0x14) = pattern;
    *(short*)(object + 0x18) = palette;
    *(long*)(object + 4) = x;
    *(long*)(object + 8) = y;
    *(short*)(object + 0x1a) = trans;
    *(short*)(object + 0x1c) = status;
    *(short*)(object + 0x1e) = height;
    *(short*)(object + 0x20) = red;
    *(short*)(object + 0x22) = green;
    *(short*)(object + 0x24) = blue;
    *(long*)(object + 0xc) = character;
    if (judgement)
        std::memcpy(object + 0x28, judgement, 0x10);
    if (screen)
        *(void**)object = screen;
}
void __thiscall RKC_RPGSCRN_OBJECT_GetParam(
    void* self, short* upd, long* pattern, short* palette, long* x, long* y,
    short* trans, short* status, short* height, void* judgement, void** screen,
    short* red, short* green, short* blue, long* character) {
    char* object = static_cast<char*>(self);
    *upd = *(short*)(object + 0x10);
    *pattern = *(long*)(object + 0x14);
    *palette = *(short*)(object + 0x18);
    *x = *(long*)(object + 4);
    *y = *(long*)(object + 8);
    if (trans) *trans = *(short*)(object + 0x1a);
    if (status) *status = *(short*)(object + 0x1c);
    if (height) *height = *(short*)(object + 0x1e);
    if (judgement) std::memcpy(judgement, object + 0x28, 0x10);
    if (screen) *screen = *(void**)object;
    if (red) *red = *(short*)(object + 0x20);
    if (green) *green = *(short*)(object + 0x22);
    if (blue) *blue = *(short*)(object + 0x24);
    if (character) *character = *(long*)(object + 0xc);
}
int __thiscall RKC_RPGSCRN_OBJECT_CheckClip(
    void* self, void* clipRect, int includeShadow) {
    char* object = static_cast<char*>(self);
    char* screen = *(char**)object;
    RECT* clip = static_cast<RECT*>(clipRect);
    for (int pass = 0; pass <= (includeShadow == 1 ? 1 : 0); ++pass) {
        void* pattern = GetUpdPattern(
            screen, static_cast<short>(*(short*)(object + 0x10) + pass),
            *(long*)(object + 0x14));
        RECT* build = GetPatternBuildRect(pattern);
        if (!build || !build->right || !build->bottom)
            continue;
        long realX = 0, realY = 0;
        RKC_RPGSCRN_CalcRealPos(
            screen, *(long*)(object + 4), *(long*)(object + 8), &realX, &realY);
        const long heightOffset =
            (*(short*)(object + 0x1e) * *(long*)(screen + 0x20)) / 100;
        const long left = build->left + realX;
        const long right = build->left + build->right - 1 + realX;
        const long top = build->top - heightOffset + realY;
        const long bottom = build->top + build->bottom - 1 - heightOffset + realY;
        if (left <= clip->right && clip->left <= right &&
            top <= clip->bottom && clip->top <= bottom)
            return 1;
    }
    return 0;
}
int __thiscall RKC_RPGSCRN_OBJECT_CheckDisplayObject(
    void* self, long x, long y, int checkPixel) {
    char* object = static_cast<char*>(self);
    char* screen = *(char**)object;
    void* pattern = GetUpdPattern(
        screen, *(short*)(object + 0x10), *(long*)(object + 0x14));
    RECT* build = GetPatternBuildRect(pattern);
    if (!pattern || !build || !build->right || !build->bottom)
        return 0;
    using GetCount = long (__thiscall*)(void*);
    using GetParts = void* (__thiscall*)(void*, long);
    static GetCount getCount = reinterpret_cast<GetCount>(LoadUpdFunction(
        "?GetPartsListCount@RKC_UPDIB_PATTERN@@QAEJXZ"));
    static GetParts getParts = reinterpret_cast<GetParts>(LoadUpdFunction(
        "?GetPartsList@RKC_UPDIB_PATTERN@@QAEPAURKC_UPDIB_PARTSLIST@@J@Z"));
    if (!getCount || !getParts)
        return 0;
    long realX = 0, realY = 0;
    RKC_RPGSCRN_CalcRealPos(
        screen, *(long*)(object + 4), *(long*)(object + 8), &realX, &realY);
    const long heightOffset =
        (*(short*)(object + 0x1e) * *(long*)(screen + 0x20)) / 100;
    const long count = getCount(pattern);
    for (long index = 0; index < count; ++index) {
        char* parts = static_cast<char*>(getParts(pattern, index));
        if (!parts)
            continue;
        long* dib = *(long**)(parts + 0x18);
        if (!dib || !dib[3])
            continue;
        const long bitCount = dib[0];
        const long width = dib[1];
        const long height = dib[2];
        const long left = *(long*)(parts + 4) + realX;
        const long top = *(long*)(parts + 8) - heightOffset + realY;
        if (x < left || x >= left + width || y < top || y >= top + height)
            continue;
        if (!checkPixel)
            return 1;
        const long sourceX = x - left;
        const long sourceY = height - 1 - (y - top);
        const unsigned char* bits =
            reinterpret_cast<const unsigned char*>(dib[3]);
        unsigned char value = 0;
        if (bitCount == 1) {
            const long stride = ((width + 31) / 32) * 4;
            value = bits[sourceY * stride + sourceX / 8] &
                    static_cast<unsigned char>(0x80 >> (sourceX & 7));
        } else if (bitCount == 4) {
            const long stride = (((width + 1) / 2) + 3) & ~3;
            const unsigned char packed = bits[sourceY * stride + sourceX / 2];
            value = sourceX & 1 ? packed & 0x0f : packed & 0xf0;
        } else if (bitCount == 8) {
            const long stride = (width + 3) & ~3;
            value = bits[sourceY * stride + sourceX];
        }
        if (value)
            return 1;
    }
    return 0;
}
void __thiscall RKC_RPGSCRN_OBJECT_SetPaletteNo(void* self, short no) { *(short*)((char*)self + 0x18) = no; }
void __thiscall RKC_RPGSCRN_OBJECT_SetPatternNo(void* self, long no) { *(long*)((char*)self + 0x14) = no; }
void __thiscall RKC_RPGSCRN_OBJECT_SetTrans(void* self, short trans) { *(short*)((char*)self + 0x1a) = trans; }
void __thiscall RKC_RPGSCRN_OBJECT_SetUpdNo(void* self, short no) { *(short*)((char*)self + 0x10) = no; }
void __thiscall RKC_RPGSCRN_OBJECT_SetCharacterNo(void* self, long value) { *(long*)((char*)self + 0xc) = value; }
void __thiscall RKC_RPGSCRN_OBJECT_SetStatus(void* self, short value) { *(short*)((char*)self + 0x1c) = value; }
void __thiscall RKC_RPGSCRN_OBJECT_SetHeight(void* self, short value) { *(short*)((char*)self + 0x1e) = value; }
void __thiscall RKC_RPGSCRN_OBJECT_SetRStrong(void* self, short value) { *(short*)((char*)self + 0x20) = value; }
void __thiscall RKC_RPGSCRN_OBJECT_SetGStrong(void* self, short value) { *(short*)((char*)self + 0x22) = value; }
void __thiscall RKC_RPGSCRN_OBJECT_SetBStrong(void* self, short value) { *(short*)((char*)self + 0x24) = value; }
int __thiscall RKC_RPGSCRN_OBJECT_SetPosition(void* self, void* value) {
    std::memcpy((char*)self + 4, value, 8);
    return 1;
}
int __thiscall RKC_RPGSCRN_OBJECT_SetJudgement(void* self, void* value) {
    std::memcpy((char*)self + 0x28, value, 0x10);
    return 1;
}

// RKC_RPGSCRN_OBJECTDISP
void* __thiscall RKC_RPGSCRN_OBJECTDISP_constructor(void* self, void* screen) {
    *(void**)self = screen;
    *(void**)((char*)self + 4) = nullptr;
    return self;
}
void* __thiscall RKC_RPGSCRN_OBJECTDISP_constructor_default(void* self) {
    return RKC_RPGSCRN_OBJECTDISP_constructor(self, nullptr);
}
void __thiscall RKC_RPGSCRN_OBJECTDISP_destructor(void* self) {
    RKC_RPGSCRN_OBJECTDISP_Release(self);
}
void* __thiscall RKC_RPGSCRN_OBJECTDISP_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 8);
    return self;
}
long __thiscall RKC_RPGSCRN_OBJECTDISP_GetCount(void* self) {
    long count = 0;
    for (char* cell = *(char**)((char*)self + 4); cell;
         cell = *(char**)(cell + 0x10))
        ++count;
    return count;
}
void* __thiscall RKC_RPGSCRN_OBJECTDISP_Get(void* self, long index) {
    char* cell = *(char**)((char*)self + 4);
    for (long current = 0; cell && current != index; ++current)
        cell = *(char**)(cell + 0x10);
    return cell;
}
int __thiscall RKC_RPGSCRN_OBJECTDISP_CheckExistStatus(void* self, short status) {
    for (char* cell = *(char**)((char*)self + 4); cell;
         cell = *(char**)(cell + 0x10))
        if ((*(short*)cell & status) != 0)
            return 1;
    return 0;
}
long __thiscall RKC_RPGSCRN_OBJECTDISP_GetNo_Cell(void* self, void* wanted) {
    long index = 0;
    for (char* cell = *(char**)((char*)self + 4); cell;
         cell = *(char**)(cell + 0x10), ++index)
        if (cell == wanted)
            return index;
    return -1;
}
long __thiscall RKC_RPGSCRN_OBJECTDISP_GetNo_Object(void* self, void* object) {
    long index = 0;
    for (char* cell = *(char**)((char*)self + 4); cell;
         cell = *(char**)(cell + 0x10), ++index)
        if (*(void**)(cell + 8) == object)
            return index;
    return -1;
}
void* __thiscall RKC_RPGSCRN_OBJECTDISP_Insert(void* self, long index) {
    const long count = RKC_RPGSCRN_OBJECTDISP_GetCount(self);
    if (index < 0 || index > count)
        return nullptr;
    char* cell = static_cast<char*>(AllocateObject(0x14));
    if (!cell)
        return nullptr;
    RKC_RPGSCRN_OBJECTDISPCELL_constructor(cell);
    char* next = static_cast<char*>(RKC_RPGSCRN_OBJECTDISP_Get(self, index));
    char* previous = next ? *(char**)(next + 0xc) : nullptr;
    if (!next) {
        previous = *(char**)((char*)self + 4);
        while (previous && *(char**)(previous + 0x10))
            previous = *(char**)(previous + 0x10);
    }
    *(char**)(cell + 0xc) = previous;
    *(char**)(cell + 0x10) = next;
    if (previous)
        *(char**)(previous + 0x10) = cell;
    else
        *(char**)((char*)self + 4) = cell;
    if (next)
        *(char**)(next + 0xc) = cell;
    return cell;
}
int __thiscall RKC_RPGSCRN_OBJECTDISP_Delete(void* self, long index) {
    char* cell = static_cast<char*>(RKC_RPGSCRN_OBJECTDISP_Get(self, index));
    if (!cell)
        return 0;
    char* previous = *(char**)(cell + 0xc);
    char* next = *(char**)(cell + 0x10);
    if (previous)
        *(char**)(previous + 0x10) = next;
    else
        *(char**)((char*)self + 4) = next;
    if (next)
        *(char**)(next + 0xc) = previous;
    FreeObject(cell);
    return 1;
}
void __thiscall RKC_RPGSCRN_OBJECTDISP_Release(void* self) {
    while (RKC_RPGSCRN_OBJECTDISP_Delete(self, 0)) {}
}
int __thiscall RKC_RPGSCRN_OBJECTDISP_Copy(void* self, void* src) {
    const long count = RKC_RPGSCRN_OBJECTDISP_GetCount(src);
    for (long index = 0; index < count; ++index)
        if (!RKC_RPGSCRN_OBJECTDISP_Insert(self, 0))
            return 0;
    char* from = *(char**)((char*)src + 4);
    for (char* to = *(char**)((char*)self + 4); to;
         to = *(char**)(to + 0x10), from = *(char**)(from + 0x10))
        *(void**)(to + 8) = *(void**)(from + 8);
    return 1;
}
void* __thiscall RKC_RPGSCRN_OBJECTDISP_GetHostScreen(void* self) {
    return *(void**)self;
}
void __thiscall RKC_RPGSCRN_OBJECTDISP_SetHostScreen(void* self, void* screen) {
    *(void**)self = screen;
}
void* __thiscall RKC_RPGSCRN_OBJECTDISP_InsertSortObjectNo(void* self, void* obj, void* block) {
    if (!obj || RKC_RPGSCRN_OBJECTDISP_GetNo_Object(self, obj) >= 0)
        return nullptr;
    long objectNo = -1;
    long index = 0;
    for (char* object = *(char**)((char*)block + 4); object;
         object = *(char**)(object + 0x3c), ++index)
        if (object == obj) {
            objectNo = index;
            break;
        }
    if (objectNo < 0)
        return nullptr;
    long insertAt = 0;
    for (char* cell = *(char**)((char*)self + 4); cell;
         cell = *(char**)(cell + 0x10), ++insertAt) {
        long existingNo = -1;
        long existingIndex = 0;
        for (char* object = *(char**)((char*)block + 4); object;
             object = *(char**)(object + 0x3c), ++existingIndex)
            if (object == *(void**)(cell + 8)) {
                existingNo = existingIndex;
                break;
            }
        if (existingNo < 0 || objectNo < existingNo)
            break;
    }
    char* cell = static_cast<char*>(RKC_RPGSCRN_OBJECTDISP_Insert(self, insertAt));
    if (cell)
        *(void**)(cell + 8) = obj;
    return cell;
}
static unsigned char DisplayClass(short status) {
    unsigned char result = (status & 0x100) != 0;
    if (status & 0x80) result = 2;
    if (status & 0x20) result = 3;
    return result;
}
void* __thiscall RKC_RPGSCRN_OBJECTDISP_InsertSort(
    void* self, void* pos, void* rect, short status, short) {
    char* position = static_cast<char*>(pos);
    char* bounds = static_cast<char*>(rect);
    long unusedX = 0, targetY = 0;
    RKC_RPGSCRN_CalcRealPos(
        *(void**)self,
        *(long*)position + *(long*)bounds,
        *(long*)(position + 4) + *(long*)(bounds + 4),
        &unusedX, &targetY);
    const unsigned char targetClass = DisplayClass(status);
    long index = 0;
    for (char* cell = *(char**)((char*)self + 4); cell;
         cell = *(char**)(cell + 0x10), ++index) {
        char* object = *(char**)(cell + 8);
        const unsigned char existingClass =
            DisplayClass(*(short*)(object + 0x1c));
        if (targetClass < existingClass)
            break;
        if (targetClass == existingClass) {
            long existingX = 0, existingY = 0;
            RKC_RPGSCRN_CalcRealPos(
                *(void**)self,
                *(long*)(object + 4) + *(long*)(object + 0x28),
                *(long*)(object + 8) + *(long*)(object + 0x2c),
                &existingX, &existingY);
            if (targetY < existingY)
                break;
        }
    }
    return RKC_RPGSCRN_OBJECTDISP_Insert(self, index);
}
int __thiscall RKC_RPGSCRN_OBJECTDISP_SetPacket(
    void* self, long vsBlock, long vs, long offsetX, long offsetY,
    int mode, unsigned short filter) {
    for (char* cell = *(char**)((char*)self + 4); cell;
         cell = *(char**)(cell + 0x10)) {
        char* object = *(char**)(cell + 8);
        if (!object)
            continue;
        const unsigned short status = *(unsigned short*)(object + 0x1c);
        bool selected = ((status & 0x1a0) == 0 && (filter & 1) != 0);
        if ((status & 0x20) && (filter & 2)) selected = true;
        if ((status & 0x80) && (filter & 4)) selected = true;
        if ((status & 0x100) && (filter & 8)) selected = true;
        if (selected)
            RKC_RPGSCRN_OBJECT_SetPacket(
                object, vsBlock, vs, offsetX, offsetY, mode,
                *(short*)(cell + 4), nullptr);
    }
    return 1;
}
int __thiscall RKC_RPGSCRN_OBJECTDISP_SortDisplayObject(void* self) {
    const long count = RKC_RPGSCRN_OBJECTDISP_GetCount(self);
    for (long target = 0; target < count; ++target) {
        long candidate = target;
        for (; candidate < count; ++candidate) {
            char* candidateCell = static_cast<char*>(
                RKC_RPGSCRN_OBJECTDISP_Get(self, candidate));
            char* candidateObject = *(char**)(candidateCell + 8);
            const unsigned char candidateClass =
                DisplayClass(*(short*)(candidateObject + 0x1c));
            const long candidateRight =
                *(long*)(candidateObject + 4) + *(long*)(candidateObject + 0x30);
            const long candidateBottom =
                *(long*)(candidateObject + 8) + *(long*)(candidateObject + 0x34);
            bool blocked = false;
            for (char* cell = static_cast<char*>(
                     RKC_RPGSCRN_OBJECTDISP_Get(self, target));
                 cell; cell = *(char**)(cell + 0x10)) {
                if (cell == candidateCell)
                    continue;
                char* object = *(char**)(cell + 8);
                const unsigned char objectClass =
                    DisplayClass(*(short*)(object + 0x1c));
                if (objectClass == candidateClass) {
                    if (*(long*)(object + 4) + *(long*)(object + 0x28) <
                            candidateRight &&
                        *(long*)(object + 8) + *(long*)(object + 0x2c) <
                            candidateBottom &&
                        (*(long*)(object + 4) + *(long*)(object + 0x30) <
                             candidateRight ||
                         *(long*)(object + 8) + *(long*)(object + 0x34) <
                             candidateBottom))
                        blocked = true;
                } else if (candidateClass < objectClass) {
                    blocked = true;
                }
                if (blocked)
                    break;
            }
            if (!blocked)
                break;
        }
        if (candidate != target && candidate < count) {
            char* cell = static_cast<char*>(
                RKC_RPGSCRN_OBJECTDISP_Get(self, candidate));
            void* object = *(void**)(cell + 8);
            RKC_RPGSCRN_OBJECTDISP_Delete(self, candidate);
            cell = static_cast<char*>(
                RKC_RPGSCRN_OBJECTDISP_Insert(self, target));
            if (cell)
                *(void**)(cell + 8) = object;
        }
    }
    return 1;
}

// RKC_RPGSCRN_OBJECTDISPCELL
void* __thiscall RKC_RPGSCRN_OBJECTDISPCELL_constructor(void* self) {
    *(long*)((char*)self + 8) = 0;
    *(long*)((char*)self + 0xc) = 0;
    *(long*)((char*)self + 0x10) = 0;
    *(short*)self = 0;
    *(long*)((char*)self + 4) = 1000;
    return self;
}
void __thiscall RKC_RPGSCRN_OBJECTDISPCELL_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_OBJECTDISPCELL_operatorAssign(void* self, const void* src) { return self; }
void* __thiscall RKC_RPGSCRN_OBJECTDISPCELL_Get(void* self) { return *(void**)((char*)self + 8); }
short __thiscall RKC_RPGSCRN_OBJECTDISPCELL_GetStatus(void* self) { return *(short*)self; }
short __thiscall RKC_RPGSCRN_OBJECTDISPCELL_GetTrans(void* self) { return *(short*)((char*)self + 4); }
void __thiscall RKC_RPGSCRN_OBJECTDISPCELL_Set(void* self, void* object) { *(void**)((char*)self + 8) = object; }
void __thiscall RKC_RPGSCRN_OBJECTDISPCELL_SetStatus(void* self, short status) { *(short*)self = status; }
void __thiscall RKC_RPGSCRN_OBJECTDISPCELL_SetTrans(void* self, short trans) { *(long*)((char*)self + 4) = trans; }

// RKC_RPGSCRN_OBJECTBLOCK
void* __thiscall RKC_RPGSCRN_OBJECTBLOCK_constructor(void* self) {
    *(long*)((char*)self + 4) = 0;
    *(long*)((char*)self + 8) = 0;
    *(long*)((char*)self + 0xc) = 0;
    return self;
}
void __thiscall RKC_RPGSCRN_OBJECTBLOCK_destructor(void* self) {
    RKC_RPGSCRN_OBJECTBLOCK_Release(self);
}
void* __thiscall RKC_RPGSCRN_OBJECTBLOCK_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 0x10);
    return self;
}
long __thiscall RKC_RPGSCRN_OBJECTBLOCK_GetCount(void* self) {
    long count = 0;
    for (char* object = *(char**)((char*)self + 4); object;
         object = *(char**)(object + 0x3c))
        ++count;
    return count;
}
void* __thiscall RKC_RPGSCRN_OBJECTBLOCK_Get(void* self, long index) {
    char* object = *(char**)((char*)self + 4);
    for (long current = 0; object && current != index; ++current)
        object = *(char**)(object + 0x3c);
    return object;
}
long __thiscall RKC_RPGSCRN_OBJECTBLOCK_GetNo(void* self, void* wanted) {
    long index = 0;
    for (char* object = *(char**)((char*)self + 4); object;
         object = *(char**)(object + 0x3c), ++index)
        if (object == wanted)
            return index;
    return -1;
}
void* __thiscall RKC_RPGSCRN_OBJECTBLOCK_Insert(void* self, long index) {
    const long count = RKC_RPGSCRN_OBJECTBLOCK_GetCount(self);
    if (index < 0 || index > count)
        return nullptr;
    char* object = static_cast<char*>(AllocateObject(0x40));
    if (!object)
        return nullptr;
    RKC_RPGSCRN_OBJECT_constructor(object);
    *(void**)object = *(void**)self;
    char* next = static_cast<char*>(RKC_RPGSCRN_OBJECTBLOCK_Get(self, index));
    char* previous = next ? *(char**)(next + 0x38) : nullptr;
    if (!next) {
        previous = *(char**)((char*)self + 4);
        while (previous && *(char**)(previous + 0x3c))
            previous = *(char**)(previous + 0x3c);
    }
    *(char**)(object + 0x38) = previous;
    *(char**)(object + 0x3c) = next;
    if (previous)
        *(char**)(previous + 0x3c) = object;
    else
        *(char**)((char*)self + 4) = object;
    if (next)
        *(char**)(next + 0x38) = object;
    return object;
}
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_Delete(void* self, long index) {
    char* object = static_cast<char*>(RKC_RPGSCRN_OBJECTBLOCK_Get(self, index));
    if (!object)
        return 0;
    char* previous = *(char**)(object + 0x38);
    char* next = *(char**)(object + 0x3c);
    if (previous)
        *(char**)(previous + 0x3c) = next;
    else
        *(char**)((char*)self + 4) = next;
    if (next)
        *(char**)(next + 0x38) = previous;
    FreeObject(object);
    return 1;
}
void __thiscall RKC_RPGSCRN_OBJECTBLOCK_Release(void* self) {
    while (RKC_RPGSCRN_OBJECTBLOCK_Delete(self, 0)) {}
}
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_Copy(void* self, void* src) {
    RKC_RPGSCRN_OBJECTBLOCK_Release(self);
    *(void**)self = *(void**)src;
    const long count = RKC_RPGSCRN_OBJECTBLOCK_GetCount(src);
    for (long index = 0; index < count; ++index) {
        void* from = RKC_RPGSCRN_OBJECTBLOCK_Get(src, index);
        void* to = RKC_RPGSCRN_OBJECTBLOCK_Insert(self, index);
        if (!to) {
            RKC_RPGSCRN_OBJECTBLOCK_Release(self);
            return 0;
        }
        RKC_RPGSCRN_OBJECT_Copy(to, from);
        *(void**)((char*)to + 0x38) =
            index ? RKC_RPGSCRN_OBJECTBLOCK_Get(self, index - 1) : nullptr;
        *(void**)((char*)to + 0x3c) = nullptr;
        if (index)
            *(void**)((char*)RKC_RPGSCRN_OBJECTBLOCK_Get(self, index - 1) + 0x3c) = to;
    }
    return 1;
}
void* __thiscall RKC_RPGSCRN_OBJECTBLOCK_GetFromCharacterNo(void* self, long charNo) {
    for (char* object = *(char**)((char*)self + 4); object;
         object = *(char**)(object + 0x3c))
        if (*(long*)(object + 0xc) == charNo)
            return object;
    return nullptr;
}
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_ReadFile(
    void* self, char* path, void* display, int append, long updOffset) {
    std::FILE* file = path ? std::fopen(path, "rb") : nullptr;
    if (!file)
        return 0;
    char header[17] = {};
    long count = 0;
    if (std::fread(header, 1, 16, file) != 16 ||
        std::memcmp(header, "RPGSCRN_OBJv", 12) != 0 ||
        std::fread(&count, sizeof(count), 1, file) != 1 || count < 0) {
        std::fclose(file);
        return 0;
    }
    const int version = std::atoi(header + 12);
    long start = 0;
    if (append)
        start = RKC_RPGSCRN_OBJECTBLOCK_GetCount(self);
    else
        RKC_RPGSCRN_OBJECTBLOCK_Release(self);
    if (display)
        RKC_RPGSCRN_OBJECTDISP_Release(display);
    bool ok = true;
    for (long index = 0; ok && index < count; ++index) {
        char* object = static_cast<char*>(
            RKC_RPGSCRN_OBJECTBLOCK_Insert(self, start + index));
        if (!object) {
            ok = false;
            break;
        }
        if (display) {
            char* cell = static_cast<char*>(
                RKC_RPGSCRN_OBJECTDISP_Insert(display, index));
            if (!cell) {
                ok = false;
                break;
            }
            *(void**)(cell + 8) = object;
        }
        ok = std::fread(object + 4, 1, 8, file) == 8 &&
             std::fread(object + 0x10, 1, 2, file) == 2 &&
             std::fread(object + 0x14, 1, 2, file) == 2 &&
             std::fread(object + 0x18, 1, 2, file) == 2 &&
             std::fread(object + 0x1a, 1, 2, file) == 2 &&
             std::fread(object + 0x1c, 1, 2, file) == 2 &&
             std::fread(object + 0x1e, 1, 2, file) == 2;
        if (ok && version > 0)
            ok = std::fread(object + 0x20, 1, 2, file) == 2 &&
                 std::fread(object + 0x22, 1, 2, file) == 2 &&
                 std::fread(object + 0x24, 1, 2, file) == 2;
        if (ok)
            ok = std::fread(object + 0x28, 1, 0x10, file) == 0x10;
        *(short*)(object + 0x10) =
            static_cast<short>(*(short*)(object + 0x10) + updOffset);
    }
    std::fclose(file);
    return ok ? 1 : 0;
}
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_WriteFile(void* self, char* path) { return 0; }
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_SetDisplayObjectRect(
    void* self, void* display, RECT* clip, int sorted) {
    if (!*(void**)self)
        return 0;
    for (char* object = *(char**)((char*)self + 4); object;
         object = *(char**)(object + 0x3c)) {
        void* pattern = GetUpdPattern(
            *(void**)self, *(short*)(object + 0x10), *(long*)(object + 0x14));
        RECT* build = GetPatternBuildRect(pattern);
        if (!build || !build->right || !build->bottom)
            continue;
        long x = 0, y = 0;
        RKC_RPGSCRN_CalcRealPos(
            *(void**)self, *(long*)(object + 4), *(long*)(object + 8), &x, &y);
        const long height =
            (*(long*)((char*)*(void**)self + 0x20) * *(short*)(object + 0x1e)) / 100;
        if (build->left + x < clip->right &&
            clip->left <= build->left + build->right - 1 + x &&
            build->top - height + y < clip->bottom &&
            clip->top <= build->top + build->bottom - 1 - height + y) {
            char* cell = static_cast<char*>(sorted
                ? RKC_RPGSCRN_OBJECTDISP_InsertSort(
                      display, object + 4, object + 0x28,
                      *(short*)(object + 0x1c), *(short*)(object + 0x1e))
                : RKC_RPGSCRN_OBJECTDISP_Insert(display, 0));
            if (cell)
                *(void**)(cell + 8) = object;
        }
    }
    return 1;
}
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_SetDisplayObject(
    void* self, void* display, long left, long top, long right, long bottom,
    int sorted, int pixelCheck, int allowDuplicate) {
    if (!*(void**)self)
        return 0;
    if (!pixelCheck) {
        RECT rect = {left, top, (right - left) + 1, (bottom - top) + 1};
        return RKC_RPGSCRN_OBJECTBLOCK_SetDisplayObjectRect(
            self, display, &rect, sorted);
    }
    for (char* object = *(char**)((char*)self + 4); object;
         object = *(char**)(object + 0x3c)) {
        if (!allowDuplicate) {
            bool exists = false;
            for (char* cell = *(char**)((char*)display + 4); cell;
                 cell = *(char**)(cell + 0x10))
                if (*(long*)(*(char**)(cell + 8) + 0xc) ==
                    *(long*)(object + 0xc)) {
                    exists = true;
                    break;
                }
            if (exists)
                continue;
        }
        bool hit = false;
        for (long y = top; !hit && y <= bottom; ++y)
            for (long x = left; x <= right; ++x)
                if (RKC_RPGSCRN_OBJECT_CheckDisplayObject(object, x, y, 1)) {
                    hit = true;
                    break;
                }
        if (hit) {
            char* cell = static_cast<char*>(sorted
                ? RKC_RPGSCRN_OBJECTDISP_InsertSort(
                      display, object + 4, object + 0x28,
                      *(short*)(object + 0x1c), *(short*)(object + 0x1e))
                : RKC_RPGSCRN_OBJECTDISP_Insert(display, 0));
            if (cell)
                *(void**)(cell + 8) = object;
        }
    }
    return 1;
}
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_SetShadowObject(
    void* self, void* display, RECT* clip, int sorted) {
    if (!*(void**)self)
        return 0;
    using GetType = unsigned int (__thiscall*)(void*);
    static GetType getType = reinterpret_cast<GetType>(LoadUpdFunction(
        "?GetType@RKC_UPDIB_UPD@@QAEIXZ"));
    using GetUpd = void* (__thiscall*)(void*, long);
    static GetUpd getUpd = reinterpret_cast<GetUpd>(LoadUpdFunction(
        "?GetUpd@RKC_UPDIB@@QAEPAVRKC_UPDIB_UPD@@J@Z"));
    for (char* object = *(char**)((char*)self + 4); object;
         object = *(char**)(object + 0x3c)) {
        if ((*(short*)(object + 0x1c) & 8) == 0)
            continue;
        void* upd = getUpd ? getUpd(**(void***)self, *(short*)(object + 0x10) + 1) : nullptr;
        if (!upd || !getType || (getType(upd) & 4) == 0)
            continue;
        void* pattern = GetUpdPattern(
            *(void**)self, *(short*)(object + 0x10) + 1, *(long*)(object + 0x14));
        RECT* build = GetPatternBuildRect(pattern);
        long x = 0, y = 0;
        RKC_RPGSCRN_CalcRealPos(
            *(void**)self, *(long*)(object + 4), *(long*)(object + 8), &x, &y);
        if (build && build->right && build->bottom &&
            build->left + x < clip->right &&
            clip->left <= build->left + build->right - 1 + x &&
            build->top + y < clip->bottom &&
            clip->top <= build->top + build->bottom - 1 + y) {
            char* cell = static_cast<char*>(sorted
                ? RKC_RPGSCRN_OBJECTDISP_InsertSort(
                      display, object + 4, object + 0x28,
                      *(short*)(object + 0x1c), *(short*)(object + 0x1e))
                : RKC_RPGSCRN_OBJECTDISP_Insert(display, 0));
            if (cell)
                *(void**)(cell + 8) = object;
        }
    }
    return 1;
}
int __thiscall RKC_RPGSCRN_OBJECTBLOCK_SetJudgementObject(
    void* self, void* display, RECT* clip, int convert, int sorted,
    int requireStatus, int excludeStatus) {
    if (!*(void**)self)
        return 0;
    for (char* object = *(char**)((char*)self + 4); object;
         object = *(char**)(object + 0x3c)) {
        const unsigned short status = *(unsigned short*)(object + 0x1c);
        if ((requireStatus && (status & 1) == 0) ||
            (excludeStatus && (status & 3) == 3))
            continue;
        long positionX = *(long*)(object + 4);
        long positionY = *(long*)(object + 8);
        long left = *(long*)(object + 0x28);
        long top = *(long*)(object + 0x2c);
        long right = *(long*)(object + 0x30);
        long bottom = *(long*)(object + 0x34);
        if (convert) {
            RKC_RPGSCRN_CalcRealPos(
                *(void**)self, positionX, positionY, &positionX, &positionY);
            RKC_RPGSCRN_CalcRealPos(
                *(void**)self, left, top, &left, &top);
            RKC_RPGSCRN_CalcRealPos(
                *(void**)self, right, bottom, &right, &bottom);
        }
        if (!clip || (left + positionX <= clip->right &&
                      clip->left <= right + positionX &&
                      top + positionY <= clip->bottom &&
                      clip->top <= bottom + positionY)) {
            char* cell = static_cast<char*>(sorted
                ? RKC_RPGSCRN_OBJECTDISP_InsertSort(
                      display, object + 4, object + 0x28,
                      *(short*)(object + 0x1c), *(short*)(object + 0x1e))
                : RKC_RPGSCRN_OBJECTDISP_Insert(display, 0));
            if (cell)
                *(void**)(cell + 8) = object;
        }
    }
    return 1;
}

// RKC_RPGSCRN_GROUNDBLOCK
void* __thiscall RKC_RPGSCRN_GROUNDBLOCK_constructor(void* self) {
    std::memset(self, 0, 0x3c);
    *(long*)((char*)self + 0xc) = 0x40;
    *(long*)((char*)self + 0x10) = 0x40;
    *(long*)((char*)self + 0x18) = 0x10;
    *(long*)((char*)self + 0x1c) = 0x10;
    return self;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_destructor(void* self) {
    RKC_RPGSCRN_GROUNDBLOCK_Release(self);
}
void* __thiscall RKC_RPGSCRN_GROUNDBLOCK_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 0x3c);
    return self;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetBaseParam(void* self, long* a, long* b) {
    char* ground = static_cast<char*>(self);
    char* screen = *(char**)ground;
    *a = (*(long*)(screen + 0x18) * *(long*)(ground + 0x18)) / 100;
    *b = (*(long*)(screen + 0x1c) * *(long*)(ground + 0x1c)) / 100;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_CalcGroundPos(
    void* self, long x, long y, long* outX, long* outY) {
    long baseX = 0, baseY = 0;
    RKC_RPGSCRN_GROUNDBLOCK_GetBaseParam(self, &baseX, &baseY);
    const long numeratorX = baseX * y + baseY * x;
    const long numeratorY = baseX * y - baseY * x;
    const long divisor = baseX * baseY * 2;
    *outX = (numeratorX + (numeratorX < 0 ? 1 : 0)) / divisor;
    *outY = (numeratorY + (numeratorY < 0 ? 1 : 0)) / divisor;
    if (numeratorX < 0) --*outX;
    if (numeratorY < 0) --*outY;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_CalcRealPos(
    void* self, long x, long y, long* outX, long* outY) {
    long baseX = 0, baseY = 0;
    RKC_RPGSCRN_GROUNDBLOCK_GetBaseParam(self, &baseX, &baseY);
    *outX = (x - y) * baseX;
    *outY = (x + y) * baseY;
}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetGroundRect(
    void* self, RECT* input, void* output, int clip, int convert) {
    long points[4][2];
    if (convert) {
        RKC_RPGSCRN_GROUNDBLOCK_CalcGroundPos(
            self, input->left, input->top, &points[0][0], &points[0][1]);
        RKC_RPGSCRN_GROUNDBLOCK_CalcGroundPos(
            self, input->right, input->top, &points[1][0], &points[1][1]);
        RKC_RPGSCRN_GROUNDBLOCK_CalcGroundPos(
            self, input->left, input->bottom, &points[2][0], &points[2][1]);
        RKC_RPGSCRN_GROUNDBLOCK_CalcGroundPos(
            self, input->right, input->bottom, &points[3][0], &points[3][1]);
    } else {
        const long magX = *(long*)((char*)self + 0x18);
        const long magY = *(long*)((char*)self + 0x1c);
        points[0][0] = points[2][0] = input->left / magX;
        points[1][0] = points[3][0] = input->right / magX;
        points[0][1] = points[1][1] = input->top / magY;
        points[2][1] = points[3][1] = input->bottom / magY;
    }
    long left = points[0][0], top = points[0][1];
    long right = left, bottom = top;
    for (int index = 1; index < 4; ++index) {
        if (points[index][0] < left) left = points[index][0];
        if (points[index][1] < top) top = points[index][1];
        if (points[index][0] > right) right = points[index][0];
        if (points[index][1] > bottom) bottom = points[index][1];
    }
    --left; --top; ++right; ++bottom;
    if (clip) {
        const long originX = *(long*)((char*)self + 0x28);
        const long originY = *(long*)((char*)self + 0x2c);
        const long finalX = originX + *(long*)((char*)self + 0x20) - 1;
        const long finalY = originY + *(long*)((char*)self + 0x24) - 1;
        if (left < originX) left = originX;
        if (top < originY) top = originY;
        if (right < originX) right = originX;
        if (bottom < originY) bottom = originY;
        if (left > finalX) left = finalX;
        if (right > finalX) right = finalX;
        if (top > finalY) top = finalY;
        if (bottom > finalY) bottom = finalY;
    }
    long* rect = static_cast<long*>(output);
    rect[0] = left;
    rect[1] = top;
    rect[2] = right;
    rect[3] = bottom;
    return 1;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_CalcAreaJudgeFromMap(
    void* self, long* width, long* height, long* offsetX, long* offsetY) {
    char* ground = static_cast<char*>(self);
    long topX = 0, topY = 0, rightX = 0, rightY = 0;
    long leftX = 0, leftY = 0, bottomX = 0, bottomY = 0;
    RKC_RPGSCRN_GROUNDBLOCK_CalcGroundPos(self, 0, 0, &topX, &topY);
    RKC_RPGSCRN_GROUNDBLOCK_CalcGroundPos(
        self, *(long*)(ground + 0xc) * *(long*)(ground + 4), 0,
        &rightX, &rightY);
    RKC_RPGSCRN_GROUNDBLOCK_CalcGroundPos(
        self, 0, *(long*)(ground + 0x10) * *(long*)(ground + 8),
        &leftX, &leftY);
    RKC_RPGSCRN_GROUNDBLOCK_CalcGroundPos(
        self, *(long*)(ground + 0xc) * *(long*)(ground + 4),
        *(long*)(ground + 0x10) * *(long*)(ground + 8),
        &bottomX, &bottomY);
    *width = bottomX - (topX - 1) + 1;
    *height = leftY - (rightY - 1) + 1;
    *offsetX = topX - 1;
    *offsetY = rightY - 1;
}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_Copy(void* self, void* src) {
    char* target = static_cast<char*>(self);
    char* source = static_cast<char*>(src);
    *(void**)target = *(void**)source;
    *(long*)(target + 0xc) = *(long*)(source + 0xc);
    *(long*)(target + 0x10) = *(long*)(source + 0x10);
    *(long*)(target + 0x18) = *(long*)(source + 0x18);
    *(long*)(target + 0x1c) = *(long*)(source + 0x1c);
    RKC_RPGSCRN_GROUNDBLOCK_SetAreaSize(
        self, *(long*)(source + 4), *(long*)(source + 8));
    for (long y = 0; y < *(long*)(target + 8); ++y)
        std::memcpy((*(void***)(target + 0x14))[y],
                    (*(void***)(source + 0x14))[y],
                    static_cast<SIZE_T>(*(long*)(target + 4)) * 6);
    RKC_RPGSCRN_GROUNDBLOCK_SetAreaJudgeSize(
        self, *(long*)(source + 0x20), *(long*)(source + 0x24),
        *(long*)(source + 0x28), *(long*)(source + 0x2c));
    for (long y = 0; y < *(long*)(target + 0x24); ++y)
        std::memcpy((*(void***)(target + 0x30))[y],
                    (*(void***)(source + 0x30))[y],
                    static_cast<SIZE_T>(*(long*)(target + 0x20)) * 2);
    return 1;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetAreaJudgeSize(void* self, long* a, long* b) {
    *a = *(long*)((char*)self + 0x20);
    *b = *(long*)((char*)self + 0x24);
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetAreaSize(void* self, long* a, long* b) {
    *a = *(long*)((char*)self + 4);
    *b = *(long*)((char*)self + 8);
}
void* __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetArea(void* self) { return *(void**)((char*)self + 0x14); }
void* __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetAreaJudge(void* self) { return *(void**)((char*)self + 0x30); }
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_Get(
    void* self, long x, long y, short* upd, short* pattern, short* status) {
    char* ground = static_cast<char*>(self);
    if (x < 0 || x >= *(long*)(ground + 4) ||
        y < 0 || y >= *(long*)(ground + 8))
        return 0;
    char* cell = (*(char***)(ground + 0x14))[y] + x * 6;
    *upd = *(short*)(cell + 2);
    *pattern = *(short*)(cell + 4);
    *status = *(short*)cell;
    return 1;
}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetJudge(
    void* self, long x, long y, short* out) {
    char* ground = static_cast<char*>(self);
    if (x < 0 || x >= *(long*)(ground + 4) ||
        y < 0 || y >= *(long*)(ground + 8))
        return 0;
    *out = ((*(short***)(ground + 0x30))[y])[x];
    return 1;
}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_ReadFile(
    void* self, char* path, long updOffset) {
    std::FILE* file = path ? std::fopen(path, "rb") : nullptr;
    if (!file)
        return 0;
    char header[16];
    long values[6];
    bool ok =
        std::fread(header, 1, sizeof(header), file) == sizeof(header) &&
        std::memcmp(header, "RPGSCRN_GNDv", 12) == 0 &&
        std::fread(values, sizeof(long), 6, file) == 6;
    for (int index = 0; ok && index < 6; ++index)
        ok = values[index] >= 0;
    if (!ok) {
        std::fclose(file);
        return 0;
    }
    char* ground = static_cast<char*>(self);
    RKC_RPGSCRN_GROUNDBLOCK_Release(self);
    *(long*)(ground + 0xc) = values[2];
    *(long*)(ground + 0x10) = values[3];
    *(long*)(ground + 0x18) = values[4];
    *(long*)(ground + 0x1c) = values[5];
    RKC_RPGSCRN_GROUNDBLOCK_SetAreaSize(self, values[0], values[1]);
    const long cells = values[0] * values[1];
    std::vector<unsigned char> mapData(
        static_cast<std::size_t>(cells) * 6);
    ok = ReadEncodedBlock(file, mapData);
    if (ok) {
        const short* source = reinterpret_cast<const short*>(mapData.data());
        for (long y = 0; y < values[1]; ++y)
            for (long x = 0; x < values[0]; ++x)
                (*(short***)(ground + 0x14))[y][x * 3] =
                    source[y * values[0] + x];
        source += cells;
        for (long y = 0; y < values[1]; ++y)
            for (long x = 0; x < values[0]; ++x)
                (*(short***)(ground + 0x14))[y][x * 3 + 1] =
                    static_cast<short>(source[y * values[0] + x] + updOffset);
        source += cells;
        for (long y = 0; y < values[1]; ++y)
            for (long x = 0; x < values[0]; ++x)
                (*(short***)(ground + 0x14))[y][x * 3 + 2] =
                    source[y * values[0] + x];
    }
    const long judgeCells =
        *(long*)(ground + 0x20) * *(long*)(ground + 0x24);
    std::vector<unsigned char> judgeData(
        static_cast<std::size_t>(judgeCells) * 2);
    if (ok)
        ok = ReadEncodedBlock(file, judgeData);
    if (ok)
        for (long y = 0; y < *(long*)(ground + 0x24); ++y)
            std::memcpy((*(short***)(ground + 0x30))[y],
                        judgeData.data() +
                            y * *(long*)(ground + 0x20) * sizeof(short),
                        static_cast<std::size_t>(
                            *(long*)(ground + 0x20)) * sizeof(short));
    std::fclose(file);
    return ok ? 1 : 0;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_ReleaseJudge(void* self) {
    char* ground = static_cast<char*>(self);
    void** rows = *(void***)(ground + 0x30);
    if (rows) {
        for (long y = 0; y < *(long*)(ground + 0x24); ++y)
            GlobalFree(rows[y]);
        GlobalFree(rows);
        *(long*)(ground + 0x20) = 0;
        *(long*)(ground + 0x24) = 0;
        *(long*)(ground + 0x28) = 0;
        *(long*)(ground + 0x2c) = 0;
        *(void***)(ground + 0x30) = nullptr;
    }
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_ReleaseMap(void* self) {
    char* ground = static_cast<char*>(self);
    void** rows = *(void***)(ground + 0x14);
    if (rows) {
        for (long y = 0; y < *(long*)(ground + 8); ++y)
            GlobalFree(rows[y]);
        GlobalFree(rows);
        *(long*)(ground + 4) = 0;
        *(long*)(ground + 8) = 0;
        *(void***)(ground + 0x14) = nullptr;
    }
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_Release(void* self) {
    RKC_RPGSCRN_GROUNDBLOCK_ReleaseMap(self);
    RKC_RPGSCRN_GROUNDBLOCK_ReleaseJudge(self);
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetAreaJudgeSize(
    void* self, long width, long height, long offsetX, long offsetY) {
    char* ground = static_cast<char*>(self);
    if (*(long*)(ground + 0x20) == width &&
        *(long*)(ground + 0x24) == height &&
        *(long*)(ground + 0x28) == offsetX &&
        *(long*)(ground + 0x2c) == offsetY)
        return;
    short** oldRows = *(short***)(ground + 0x30);
    const long oldWidth = *(long*)(ground + 0x20);
    const long oldHeight = *(long*)(ground + 0x24);
    const long oldOffsetX = *(long*)(ground + 0x28);
    const long oldOffsetY = *(long*)(ground + 0x2c);
    short** rows = nullptr;
    if (width && height) {
        rows = static_cast<short**>(
            GlobalAlloc(GPTR, static_cast<SIZE_T>(height) * sizeof(short*)));
        for (long y = 0; rows && y < height; ++y)
            rows[y] = static_cast<short*>(
                GlobalAlloc(GPTR, static_cast<SIZE_T>(width) * sizeof(short)));
    }
    if (rows && oldRows) {
        for (long worldY = oldOffsetY; worldY < oldOffsetY + oldHeight; ++worldY)
            for (long worldX = oldOffsetX; worldX < oldOffsetX + oldWidth; ++worldX)
                if (worldX >= offsetX && worldX < offsetX + width &&
                    worldY >= offsetY && worldY < offsetY + height)
                    rows[worldY - offsetY][worldX - offsetX] =
                        oldRows[worldY - oldOffsetY][worldX - oldOffsetX];
    }
    RKC_RPGSCRN_GROUNDBLOCK_ReleaseJudge(self);
    *(long*)(ground + 0x20) = width;
    *(long*)(ground + 0x24) = height;
    *(long*)(ground + 0x28) = offsetX;
    *(long*)(ground + 0x2c) = offsetY;
    *(short***)(ground + 0x30) = rows;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetAreaSize(
    void* self, long width, long height) {
    char* ground = static_cast<char*>(self);
    if (*(long*)(ground + 4) == width && *(long*)(ground + 8) == height)
        return;
    char** oldRows = *(char***)(ground + 0x14);
    const long oldWidth = *(long*)(ground + 4);
    const long oldHeight = *(long*)(ground + 8);
    char** rows = nullptr;
    if (width && height) {
        rows = static_cast<char**>(
            GlobalAlloc(GPTR, static_cast<SIZE_T>(height) * sizeof(char*)));
        for (long y = 0; rows && y < height; ++y) {
            rows[y] = static_cast<char*>(
                GlobalAlloc(GPTR, static_cast<SIZE_T>(width) * 6));
            for (long x = 0; rows[y] && x < width; ++x)
                *(short*)(rows[y] + x * 6 + 2) = -1;
        }
    }
    if (rows && oldRows)
        for (long y = 0; y < height && y < oldHeight; ++y)
            std::memcpy(rows[y], oldRows[y],
                        static_cast<SIZE_T>(width < oldWidth ? width : oldWidth) * 6);
    RKC_RPGSCRN_GROUNDBLOCK_ReleaseMap(self);
    *(long*)(ground + 4) = width;
    *(long*)(ground + 8) = height;
    *(char***)(ground + 0x14) = rows;
    long judgeWidth = 0, judgeHeight = 0, judgeX = 0, judgeY = 0;
    if (rows)
        RKC_RPGSCRN_GROUNDBLOCK_CalcAreaJudgeFromMap(
            self, &judgeWidth, &judgeHeight, &judgeX, &judgeY);
    RKC_RPGSCRN_GROUNDBLOCK_SetAreaJudgeSize(
        self, judgeWidth, judgeHeight, judgeX, judgeY);
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetBaseMag(void* self, long a, long b) {
    *(long*)((char*)self + 0x18) = a;
    *(long*)((char*)self + 0x1c) = b;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetBaseMag(void* self, long* a, long* b) {
    *a = *(long*)((char*)self + 0x18);
    *b = *(long*)((char*)self + 0x1c);
}
long __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetChipHeight(void* self) { return *(long*)((char*)self + 0x10); }
long __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetChipWidth(void* self) { return *(long*)((char*)self + 0xc); }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetChipHeight(void* self, long h) { *(long*)((char*)self + 0x10) = h; }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetChipWidth(void* self, long w) { *(long*)((char*)self + 0xc) = w; }
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetJudgeOffset(void* self, long a, long b) {
    *(long*)((char*)self + 0x28) = a;
    *(long*)((char*)self + 0x2c) = b;
}
void __thiscall RKC_RPGSCRN_GROUNDBLOCK_GetJudgeOffset(void* self, long* a, long* b) {
    *a = *(long*)((char*)self + 0x28);
    *b = *(long*)((char*)self + 0x2c);
}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetJudge(
    void* self, long x, long y, short value) {
    char* ground = static_cast<char*>(self);
    if (x < 0 || x >= *(long*)(ground + 0x20) ||
        y < 0 || y >= *(long*)(ground + 0x24))
        return 0;
    ((*(short***)(ground + 0x30))[y])[x] = value;
    return 1;
}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_Set(
    void* self, long x, long y, short upd, short pattern, short status) {
    char* ground = static_cast<char*>(self);
    if (x < 0 || x >= *(long*)(ground + 4) ||
        y < 0 || y >= *(long*)(ground + 8))
        return 0;
    char* cell = (*(char***)(ground + 0x14))[y] + x * 6;
    *(short*)(cell + 2) = upd;
    *(short*)(cell + 4) = pattern;
    *(short*)cell = status;
    return 1;
}
int __thiscall RKC_RPGSCRN_GROUNDBLOCK_WriteFile(void* self, char* path) { return 0; }

// RKC_RPGSCRN_CHARANIMBLOCK
void* __thiscall RKC_RPGSCRN_CHARANIMBLOCK_constructor(void* self) {
    std::memset(self, 0, 0xc);
    return self;
}
void __thiscall RKC_RPGSCRN_CHARANIMBLOCK_destructor(void* self) {
    RKC_RPGSCRN_CHARANIMBLOCK_Release(self, -1);
}
void* __thiscall RKC_RPGSCRN_CHARANIMBLOCK_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 0xc);
    return self;
}
long __thiscall RKC_RPGSCRN_CHARANIMBLOCK_GetCount(void* self) {
    return *(long*)((char*)self + 4);
}
void* __thiscall RKC_RPGSCRN_CHARANIMBLOCK_Get(void* self, long index) {
    const long count = *(long*)((char*)self + 4);
    if (index < 0 || index >= count)
        return nullptr;
    return (*(void***)((char*)self + 8))[index];
}
void __thiscall RKC_RPGSCRN_CHARANIMBLOCK_Release(void* self, long index) {
    long& count = *(long*)((char*)self + 4);
    void**& animations = *(void***)((char*)self + 8);
    if (index < 0) {
        if (animations) {
            for (long item = 0; item < count; ++item) {
                if (animations[item]) {
                    RKC_RPGSCRN_CHARANIM_destructor(animations[item]);
                    FreeObject(animations[item]);
                }
            }
            GlobalFree(animations);
        }
        animations = nullptr;
        count = 0;
        return;
    }
    if (index >= count || !animations)
        return;
    if (animations[index]) {
        RKC_RPGSCRN_CHARANIM_destructor(animations[index]);
        FreeObject(animations[index]);
    }
    animations[index] = AllocateObject(0x18);
    if (animations[index]) {
        RKC_RPGSCRN_CHARANIM_constructor(animations[index]);
        *(void**)animations[index] = *(void**)self;
    }
}
int __thiscall RKC_RPGSCRN_CHARANIMBLOCK_CreateIndex(
    void* self, long count, void* screen) {
    RKC_RPGSCRN_CHARANIMBLOCK_Release(self, -1);
    if (count < 0)
        return 0;
    *(void**)self = screen;
    if (count == 0)
        return 1;
    void** animations = static_cast<void**>(
        GlobalAlloc(GPTR, static_cast<SIZE_T>(count) * sizeof(void*)));
    if (!animations)
        return 0;
    *(void***)((char*)self + 8) = animations;
    *(long*)((char*)self + 4) = count;
    for (long index = 0; index < count; ++index) {
        animations[index] = AllocateObject(0x18);
        if (!animations[index]) {
            RKC_RPGSCRN_CHARANIMBLOCK_Release(self, -1);
            return 0;
        }
        RKC_RPGSCRN_CHARANIM_constructor(animations[index]);
        *(void**)animations[index] = screen;
    }
    return 1;
}
long __thiscall RKC_RPGSCRN_CHARANIMBLOCK_GetMaxPartsCountFromFile(
    void*, char* path) {
    unsigned char animation[0x18];
    RKC_RPGSCRN_CHARANIM_constructor(animation);
    long result = 0;
    if (RKC_RPGSCRN_CHARANIM_ReadCafFile(animation, path, 0))
        result = RKC_RPGSCRN_CHARANIM_GetMaxPartsCount(animation);
    RKC_RPGSCRN_CHARANIM_destructor(animation);
    return result;
}

// RKC_RPGSCRN_CHARANIM
void* __thiscall RKC_RPGSCRN_CHARANIM_constructor(void* self) {
    *(long*)self = 0;
    *(long*)((char*)self + 8) = -1;
    *(long*)((char*)self + 0x10) = 0;
    *(long*)((char*)self + 0x14) = 0;
    return self;
}
void __thiscall RKC_RPGSCRN_CHARANIM_destructor(void* self) {
    RKC_RPGSCRN_CHARANIM_Release(self);
}
void* __thiscall RKC_RPGSCRN_CHARANIM_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 0x18);
    return self;
}
long __thiscall RKC_RPGSCRN_CHARANIM_GetCount(void* self) {
    return *(long*)((char*)self + 0x10);
}
void* __thiscall RKC_RPGSCRN_CHARANIM_Get(void* self, long index) {
    const long count = *(long*)((char*)self + 0x10);
    if (index < 0 || index >= count)
        return nullptr;
    return (*(void***)((char*)self + 0x14))[index];
}
void __thiscall RKC_RPGSCRN_CHARANIM_Release(void* self) {
    long& count = *(long*)((char*)self + 0x10);
    void**& charts = *(void***)((char*)self + 0x14);
    if (charts) {
        for (long index = 0; index < count; ++index) {
            if (charts[index]) {
                RKC_RPGSCRN_CHARANIMCHART_destructor(charts[index]);
                FreeObject(charts[index]);
            }
        }
        GlobalFree(charts);
    }
    charts = nullptr;
    count = 0;
    *(long*)((char*)self + 8) = -1;
    *(void**)self = nullptr;
}
int __thiscall RKC_RPGSCRN_CHARANIM_ReadCafFile(
    void* self, char* path, long animationNo) {
    if (!path)
        return 0;
    std::FILE* file = std::fopen(path, "rb");
    if (!file)
        return 0;
    char header[17] = {};
    long chartCount = 0;
    if (std::fread(header, 1, 16, file) != 16 ||
        std::memcmp(header, "CHRAnimation", 12) != 0 ||
        std::fread(&chartCount, sizeof(chartCount), 1, file) != 1 ||
        chartCount < 0) {
        std::fclose(file);
        return 0;
    }
    const int version = std::atoi(header + 12);
    RKC_RPGSCRN_CHARANIM_Release(self);
    void** charts = nullptr;
    if (chartCount) {
        charts = static_cast<void**>(
            GlobalAlloc(GPTR, static_cast<SIZE_T>(chartCount) * sizeof(void*)));
        if (!charts) {
            std::fclose(file);
            return 0;
        }
    }
    *(void***)((char*)self + 0x14) = charts;
    *(long*)((char*)self + 0x10) = chartCount;
    bool ok = true;
    for (long chartIndex = 0; ok && chartIndex < chartCount; ++chartIndex) {
        char* chart = static_cast<char*>(AllocateObject(0x60));
        if (!chart) {
            ok = false;
            break;
        }
        charts[chartIndex] = chart;
        RKC_RPGSCRN_CHARANIMCHART_constructor(chart);
        if (std::fread(chart, sizeof(short), 1, file) != 1) {
            ok = false;
            break;
        }
        for (long direction = 0; ok && direction < 9; ++direction) {
            long blockCount = 0;
            if (std::fread(&blockCount, sizeof(blockCount), 1, file) != 1 ||
                blockCount < 0) {
                ok = false;
                break;
            }
            *(long*)(chart + 4 + direction * 4) = blockCount;
            char* blocks = nullptr;
            if (blockCount) {
                blocks = static_cast<char*>(
                    AllocateObject(static_cast<SIZE_T>(blockCount) * 8));
                if (!blocks) {
                    ok = false;
                    break;
                }
                for (long block = 0; block < blockCount; ++block)
                    RKC_RPGSCRN_CHARANIMCELLBLOCK_constructor(blocks + block * 8);
            }
            *(char**)(chart + 0x28 + direction * 4) = blocks;
            if (std::fread(chart + 0x4c + direction * 2,
                           sizeof(short), 1, file) != 1) {
                ok = false;
                break;
            }
            for (long block = 0; ok && block < blockCount; ++block) {
                char* cellBlock = blocks + block * 8;
                long cellCount = 0;
                if (std::fread(&cellCount, sizeof(cellCount), 1, file) != 1 ||
                    cellCount < 0) {
                    ok = false;
                    break;
                }
                *(long*)cellBlock = cellCount;
                char* cells = nullptr;
                if (cellCount) {
                    cells = static_cast<char*>(
                        AllocateObject(static_cast<SIZE_T>(cellCount) * 0xc));
                    if (!cells) {
                        ok = false;
                        break;
                    }
                    for (long cell = 0; cell < cellCount; ++cell)
                        RKC_RPGSCRN_CHARANIMCELL_constructor(cells + cell * 0xc);
                }
                *(char**)(cellBlock + 4) = cells;
                for (long cell = 0; ok && cell < cellCount; ++cell) {
                    char* value = cells + cell * 0xc;
                    if (std::fread(value, sizeof(short), 1, file) != 1 ||
                        std::fread(value + 4, sizeof(short), 1, file) != 1) {
                        ok = false;
                        break;
                    }
                    if (version < 2) {
                        short pattern = 0;
                        if (std::fread(&pattern, sizeof(pattern), 1, file) != 1) {
                            ok = false;
                            break;
                        }
                        *(long*)(value + 8) = pattern;
                    } else if (std::fread(value + 8, sizeof(long), 1, file) != 1) {
                        ok = false;
                        break;
                    }
                    if (std::fread(value + 2, sizeof(short), 1, file) != 1) {
                        ok = false;
                        break;
                    }
                }
            }
        }
    }
    if (ok) {
        if (version == 0) {
            *(long*)((char*)self + 4) = 0;
        } else if (std::fread((char*)self + 4, sizeof(long), 1, file) != 1 ||
                   std::fread((char*)self + 0xc, sizeof(long), 1, file) != 1) {
            ok = false;
        }
    }
    std::fclose(file);
    if (!ok) {
        RKC_RPGSCRN_CHARANIM_Release(self);
        return 0;
    }
    *(long*)((char*)self + 8) = animationNo;
    return 1;
}
long __thiscall RKC_RPGSCRN_CHARANIM_GetMaxPartsCount(void* self) {
    long maximum = 0;
    const long count = RKC_RPGSCRN_CHARANIM_GetCount(self);
    for (long chartIndex = 0; chartIndex < count; ++chartIndex) {
        char* chart = static_cast<char*>(RKC_RPGSCRN_CHARANIM_Get(self, chartIndex));
        for (long direction = 0; direction < 9; ++direction) {
            const long parts = *(long*)(chart + 4 + direction * 4);
            if (parts > maximum)
                maximum = parts;
        }
    }
    return maximum;
}

// RKC_RPGSCRN_CHARANIMCELLBLOCK
void* __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_constructor(void* self) {
    *(long*)self = 0;
    *(long*)((char*)self + 4) = 0;
    return self;
}
void __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_destructor(void* self) {
    RKC_RPGSCRN_CHARANIMCELLBLOCK_Release(self);
}
void* __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 8);
    return self;
}
long __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_GetCount(void* self) {
    return *(long*)self;
}
void* __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_Get(void* self, long index) {
    const long count = *(long*)self;
    if (index < 0 || index >= count)
        return nullptr;
    return (char*)*(void**)((char*)self + 4) + index * 0xc;
}
void __thiscall RKC_RPGSCRN_CHARANIMCELLBLOCK_Release(void* self) {
    void*& cells = *(void**)((char*)self + 4);
    if (cells)
        FreeObject(cells);
    cells = nullptr;
    *(long*)self = 0;
}

// RKC_RPGSCRN_CHARANIMCELL
void* __thiscall RKC_RPGSCRN_CHARANIMCELL_constructor(void* self) {
    *(short*)self = 0;
    *(short*)((char*)self + 2) = 0;
    *(short*)((char*)self + 4) = 0;
    *(long*)((char*)self + 8) = 0;
    return self;
}
void __thiscall RKC_RPGSCRN_CHARANIMCELL_destructor(void* self) {}
void* __thiscall RKC_RPGSCRN_CHARANIMCELL_operatorAssign(void* self, const void* src) { return self; }
long __thiscall RKC_RPGSCRN_CHARANIMCELL_GetPatternNo(void* self) { return *(long*)((char*)self + 8); }
short __thiscall RKC_RPGSCRN_CHARANIMCELL_GetPriority(void* self) { return *(short*)((char*)self + 2); }
short __thiscall RKC_RPGSCRN_CHARANIMCELL_GetStatus(void* self) { return *(short*)self; }
short __thiscall RKC_RPGSCRN_CHARANIMCELL_GetTrans(void* self) { return *(short*)((char*)self + 4); }
void __thiscall RKC_RPGSCRN_CHARANIMCELL_SetPatternNo(void* self, long no) { *(long*)((char*)self + 8) = no; }
void __thiscall RKC_RPGSCRN_CHARANIMCELL_SetPriority(void* self, short priority) { *(short*)((char*)self + 2) = priority; }
void __thiscall RKC_RPGSCRN_CHARANIMCELL_SetStatus(void* self, short status) { *(short*)self = status; }
void __thiscall RKC_RPGSCRN_CHARANIMCELL_SetTrans(void* self, short trans) { *(short*)((char*)self + 4) = trans; }

// RKC_RPGSCRN_CHARANIMCHART
void* __thiscall RKC_RPGSCRN_CHARANIMCHART_constructor(void* self) {
    std::memset((char*)self + 4, 0, 0x48);
    return self;
}
void __thiscall RKC_RPGSCRN_CHARANIMCHART_destructor(void* self) {
    RKC_RPGSCRN_CHARANIMCHART_Release(self);
}
void* __thiscall RKC_RPGSCRN_CHARANIMCHART_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 0x60);
    return self;
}
long __thiscall RKC_RPGSCRN_CHARANIMCHART_GetCount(void* self, long index) {
    return *(long*)((char*)self + 4 + index * 4);
}
void* __thiscall RKC_RPGSCRN_CHARANIMCHART_Get(void* self, long direction, long index) {
    if (direction < 0 || direction >= 9)
        return nullptr;
    const long count = *(long*)((char*)self + 4 + direction * 4);
    if (index < 0 || index >= count)
        return nullptr;
    return (char*)*(void**)((char*)self + 0x28 + direction * 4) + index * 8;
}
void __thiscall RKC_RPGSCRN_CHARANIMCHART_Release(void* self) {
    for (long direction = 0; direction < 9; ++direction) {
        char*& blocks = *(char**)((char*)self + 0x28 + direction * 4);
        const long count = *(long*)((char*)self + 4 + direction * 4);
        if (blocks) {
            for (long index = 0; index < count; ++index)
                RKC_RPGSCRN_CHARANIMCELLBLOCK_destructor(blocks + index * 8);
            FreeObject(blocks);
        }
        blocks = nullptr;
        *(long*)((char*)self + 4 + direction * 4) = 0;
    }
}
short __thiscall RKC_RPGSCRN_CHARANIMCHART_GetMaxFrameCount(void* self, long index) {
    return *(short*)((char*)self + 0x4c + index * 2);
}
short __thiscall RKC_RPGSCRN_CHARANIMCHART_GetStatus(void* self) { return *(short*)self; }
void __thiscall RKC_RPGSCRN_CHARANIMCHART_SetMaxFrameCount(void* self, long index, short count) {
    *(short*)((char*)self + 0x4c + index * 2) = count;
}
void __thiscall RKC_RPGSCRN_CHARANIMCHART_SetStatus(void* self, short status) { *(short*)self = status; }

void* __thiscall RKC_RPGSCRN_CHARANIMBLOCK_SetObject(
    void* self, long animationIndex, long chartIndex, long direction,
    long frame, long x, long y, short height, void* objectBlock,
    void* judgement, int* enabled, short* reds, short* greens, short* blues,
    long character, long additionalStatus, long transScale, void* clip,
    long redScale, long greenScale, long blueScale) {
    char* animation = static_cast<char*>(
        RKC_RPGSCRN_CHARANIMBLOCK_Get(self, animationIndex));
    if (!animation)
        return nullptr;
    char* chart = static_cast<char*>(
        RKC_RPGSCRN_CHARANIM_Get(animation, chartIndex));
    if (!chart || direction < 0 || direction >= 9)
        return nullptr;
    const short maximum = *(short*)(chart + 0x4c + direction * 2);
    if ((*(short*)chart & 1) && maximum)
        frame %= maximum;
    const long partCount = *(long*)(chart + 4 + direction * 4);
    std::vector<char*> ordered(static_cast<std::size_t>(partCount), nullptr);
    std::vector<long> sourceIndices(static_cast<std::size_t>(partCount), 0);
    char* blocks = *(char**)(chart + 0x28 + direction * 4);
    for (long index = 0; index < partCount; ++index) {
        char* block = blocks ? blocks + index * 8 : nullptr;
        char* cell = block ? static_cast<char*>(
            RKC_RPGSCRN_CHARANIMCELLBLOCK_Get(block, frame)) : nullptr;
        if (cell && (!enabled || enabled[index] == 1)) {
            const short priority = *(short*)(cell + 2);
            if (priority >= 0 && priority < partCount) {
                ordered[priority] = cell;
                sourceIndices[priority] = index;
            }
        }
    }
    void* result = nullptr;
    for (long priority = 0; priority < partCount; ++priority) {
        char* cell = ordered[priority];
        if (!cell)
            continue;
        unsigned char candidate[0x40];
        RKC_RPGSCRN_OBJECT_constructor(candidate);
        *(void**)candidate = *(void**)self;
        *(short*)(candidate + 0x1e) = height;
        *(long*)(candidate + 0x14) = *(long*)(cell + 8);
        *(short*)(candidate + 0x10) = *(short*)(animation + 8);
        *(long*)(candidate + 4) = x;
        *(long*)(candidate + 8) = y;
        *(short*)(candidate + 0x18) = *(long*)(animation + 4) == 0
            ? -1
            : static_cast<short>(
                  *(short*)(animation + 0xc) * chartIndex + priority);
        *(short*)(candidate + 0x1a) = static_cast<short>(
            (*(short*)(cell + 4) * transScale) / 1000);
        *(short*)(candidate + 0x1c) = static_cast<short>(
            *(short*)cell | static_cast<short>(additionalStatus));
        const long source = sourceIndices[priority];
        if (reds) *(short*)(candidate + 0x20) = reds[source];
        if (greens) *(short*)(candidate + 0x22) = greens[source];
        if (blues) *(short*)(candidate + 0x24) = blues[source];
        *(short*)(candidate + 0x20) = static_cast<short>(
            (*(short*)(candidate + 0x20) * redScale) / 1000);
        *(short*)(candidate + 0x22) = static_cast<short>(
            (*(short*)(candidate + 0x22) * greenScale) / 1000);
        *(short*)(candidate + 0x24) = static_cast<short>(
            (*(short*)(candidate + 0x24) * blueScale) / 1000);
        *(long*)(candidate + 0xc) = character;
        if (judgement)
            std::memcpy(candidate + 0x28, judgement, 0x10);
        else
            std::memset(candidate + 0x28, 0, 0x10);
        if (clip && !RKC_RPGSCRN_OBJECT_CheckClip(candidate, clip, 1))
            continue;
        char* object = static_cast<char*>(
            RKC_RPGSCRN_OBJECTBLOCK_Insert(objectBlock, 0));
        if (object) {
            RKC_RPGSCRN_OBJECT_Copy(object, candidate);
            result = object;
        }
    }
    return result;
}

void __thiscall RKC_RPGSCRN_CHARANIMBLOCK_SetPacket(
    void* self, long vsBlock, long vs, long animationIndex, long chartIndex,
    long direction, long frame, long x, long y, int* enabled,
    short* reds, short* greens, short* blues, long additionalStatus,
    long transScale) {
    char* animation = static_cast<char*>(
        RKC_RPGSCRN_CHARANIMBLOCK_Get(self, animationIndex));
    char* chart = animation ? static_cast<char*>(
        RKC_RPGSCRN_CHARANIM_Get(animation, chartIndex)) : nullptr;
    if (!chart || direction < 0 || direction >= 9)
        return;
    const short maximum = *(short*)(chart + 0x4c + direction * 2);
    if ((*(short*)chart & 1) && maximum)
        frame %= maximum;
    const long partCount = *(long*)(chart + 4 + direction * 4);
    std::vector<char*> ordered(static_cast<std::size_t>(partCount), nullptr);
    std::vector<long> sourceIndices(static_cast<std::size_t>(partCount), 0);
    char* blocks = *(char**)(chart + 0x28 + direction * 4);
    for (long index = 0; index < partCount; ++index) {
        char* cell = blocks ? static_cast<char*>(
            RKC_RPGSCRN_CHARANIMCELLBLOCK_Get(blocks + index * 8, frame)) : nullptr;
        if (cell && (!enabled || enabled[index] == 1)) {
            const short priority = *(short*)(cell + 2);
            if (priority >= 0 && priority < partCount) {
                ordered[priority] = cell;
                sourceIndices[priority] = index;
            }
        }
    }
    auto render = [&](char* cell, long source, int mode) {
        unsigned char object[0x40];
        RKC_RPGSCRN_OBJECT_constructor(object);
        *(void**)object = *(void**)self;
        *(long*)(object + 0x14) = *(long*)(cell + 8);
        *(short*)(object + 0x10) = *(short*)(animation + 8);
        *(long*)(object + 4) = x;
        *(long*)(object + 8) = y;
        *(short*)(object + 0x18) = -1;
        *(short*)(object + 0x1a) = *(short*)(cell + 4);
        *(short*)(object + 0x1c) = static_cast<short>(
            *(short*)cell | static_cast<short>(additionalStatus));
        if (reds) *(short*)(object + 0x20) = reds[source];
        if (greens) *(short*)(object + 0x22) = greens[source];
        if (blues) *(short*)(object + 0x24) = blues[source];
        long position[2] = {x, y};
        RKC_RPGSCRN_OBJECT_SetPacket(
            object, vsBlock, vs, 0, 0, mode,
            static_cast<short>(transScale), position);
    };
    for (long priority = partCount - 1; priority >= 0; --priority)
        if (ordered[priority] && (*(short*)ordered[priority] & 8))
            render(ordered[priority], sourceIndices[priority], 1);
    for (long priority = partCount - 1; priority >= 0; --priority)
        if (ordered[priority])
            render(ordered[priority], sourceIndices[priority], 0);
}

int __thiscall RKC_RPGSCRN_GROUNDBLOCK_SetPacket(
    void* self, long vsBlockNo, long vsNo, RECT* clip,
    long offsetX, long offsetY, RECT* patternClip) {
    char* ground = static_cast<char*>(self);
    char* screen = *(char**)ground;
    if (!screen || !*(void**)screen)
        return 0;
    using GetBlock = void* (__thiscall*)(void*, long);
    using GetVs = void* (__thiscall*)(void*, long);
    using SetPacket = void* (__thiscall*)(
        void*, long, long, long, long, long, long, long, long, long, long,
        long, long, short, short, short, RECT*, void*);
    static GetBlock getBlock = reinterpret_cast<GetBlock>(LoadUpdFunction(
        "?GetVSBlock@RKC_UPDIB@@QAEPAVRKC_UPDIB_VSBLOCK@@J@Z"));
    static GetVs getVs = reinterpret_cast<GetVs>(LoadUpdFunction(
        "?GetVScreen@RKC_UPDIB_VSBLOCK@@QAEPAVRKC_UPDIB_VS@@J@Z"));
    static SetPacket setPacket = reinterpret_cast<SetPacket>(LoadUpdFunction(
        "?SetPacket@RKC_UPDIB_VS@@QAEPAVRKC_UPDIB_VSPACKET@@JJJJJJJJJJJJFFFPAUtagRECT@@PAVRKC_DIB@@@Z"));
    void* block = getBlock ? getBlock(*(void**)screen, vsBlockNo) : nullptr;
    void* vs = block && getVs ? getVs(block, vsNo) : nullptr;
    if (!vs || !setPacket || !clip)
        return 0;
    long startX = clip->left < 0 ? 0 : clip->left / *(long*)(ground + 0xc);
    long startY = clip->top < 0 ? 0 : clip->top / *(long*)(ground + 0x10);
    long endX = clip->right < 0 ? 0 : clip->right / *(long*)(ground + 0xc);
    long endY = clip->bottom < 0 ? 0 : clip->bottom / *(long*)(ground + 0x10);
    if (endX >= *(long*)(ground + 4)) endX = *(long*)(ground + 4) - 1;
    if (endY >= *(long*)(ground + 8)) endY = *(long*)(ground + 8) - 1;
    for (long y = startY; y <= endY; ++y)
        for (long x = startX; x <= endX; ++x) {
            short* cell = (*(short***)(ground + 0x14))[y] + x * 3;
            if (cell[1] < 0)
                continue;
            const long screenX = *(long*)(ground + 0xc) * x - offsetX;
            const long screenY = *(long*)(ground + 0x10) * y - offsetY;
            if (patternClip) {
                void* pattern = GetUpdPattern(screen, cell[1], cell[2]);
                RECT* build = GetPatternBuildRect(pattern);
                if (build && (build->left + screenX > patternClip->right ||
                    build->left + build->right + screenX < patternClip->left ||
                    build->top + screenY > patternClip->bottom ||
                    build->top + build->bottom + screenY < patternClip->top))
                    continue;
            }
            setPacket(
                vs, 0, cell[1], cell[2], -1, 1, screenX, screenY,
                1000, 1000, 1000, 1000, 0, 1000, 1000, 1000,
                nullptr, nullptr);
        }
    return 1;
}

} // extern "C"

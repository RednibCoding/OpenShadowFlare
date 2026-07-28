/**
 * RKC_UPDIB - Universal Pattern DIB handling
 * 
 * This DLL provides sprite/pattern rendering functions for ShadowFlare.
 * It manages UPD files (sprite sheets with multiple patterns/animations).
 * 
 * Multiple classes with different layouts are defined here.
 */

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>

#define OSF_DEBUG 1
#define DLL_NAME "RKC_UPDIB"
#include "../../debug.h"
#include "../../utils.h"

// Forward declarations
class RKC_DIB;
class RKC_DIBHISPEEDMODE;
struct RKC_UPDIB_JUDGE;
struct RKC_UPDIB_PARTSLIST;
struct RKC_UPDIB_UPD_PARTS;

extern "C" void* __thiscall RKC_UPDIB_GetUpd(void* self, long index);
extern "C" void* __thiscall RKC_UPDIB_UPD_GetPattern(void* self, long index);
extern "C" void* __thiscall RKC_UPDIB_UPD_GetPalette(void* self, long index);
extern "C" int __thiscall RKC_UPDIB_UPD_Read(void* self, char* filename, long flags);
extern "C" void __thiscall RKC_UPDIB_CreateTemporaryDIB(void* self);
extern "C" int __thiscall RKC_UPDIB_CreateUpdBlock(void* self, long count);
extern "C" long __thiscall RKC_UPDIB_GetVSBlockCount(void* self);
extern "C" void* __thiscall RKC_UPDIB_InsertVSBlock(void* self, long index);
extern "C" void __thiscall RKC_UPDIB_VSBLOCK_Release(void* self);
extern "C" int __thiscall RKC_UPDIB_VSBLOCK_CreateVS(void* self, long count);
extern "C" int __thiscall RKC_UPDIB_VSBLOCK_Render(void* self, RKC_DIB* dib, long vsIndex, long reverseVsOrder, long reversePacketOrder, RECT* clip);
extern "C" void* __thiscall RKC_UPDIB_VS_GetVSPacket(void* self, long index);
extern "C" void* __thiscall RKC_UPDIB_VSBLOCK_GetVScreen(void* self, long index);
extern "C" int __thiscall RKC_UPDIB_VS_Render(void* self, RKC_DIB* dib, long packetIndex, long reverseOrder, RECT* clip);
extern "C" void* __thiscall RKC_UPDIB_VS_InsertVSPacket(void* self, long index);
extern "C" void __thiscall RKC_UPDIB_Release(void* self);
extern "C" void __thiscall RKC_UPDIB_UPD_Release(void* self);
extern "C" int __thiscall RKC_UPDIB_VSPACKET_Render(void* self, RKC_DIB* dib, RECT* clip);
extern "C" int __thiscall RKC_UPDIB_VSPACKET_RenderBox(void* self, RKC_DIB* dib, RECT* clip);
extern "C" int __thiscall RKC_UPDIB_VSPACKET_RenderFill(void* self, RKC_DIB* dib, RECT* clip);
extern "C" int __thiscall RKC_UPDIB_VSPACKET_RenderLine(void* self, RKC_DIB* dib, RECT* clip);
extern "C" int __thiscall RKC_UPDIB_VSPACKET_RenderPoint(void* self, RKC_DIB* dib, RECT* clip);
extern "C" void* __thiscall RKC_UPDIB_VSPACKET_constructor(void* self);
extern "C" void __thiscall RKC_UPDIB_VSPACKET_destructor(void* self);
extern "C" void* __thiscall RKC_UPDIB_VS_SetPacket(void* self, long index, void* packet);
extern "C" void* __thiscall RKC_UPDIB_VSPACKET_SetPacket_copy(void* self, void* packet);
extern "C" void* __thiscall RKC_UPDIB_VSPACKET_SetPacket_full(
    void* self,
    void* updib,
    long updIndex,
    long patternIndex,
    long paletteIndex,
    long flags,
    long x,
    long y,
    long scaleX,
    long scaleY,
    long alpha,
    long blendMode,
    long userValue,
    short red,
    short green,
    short blue,
    RECT* clip,
    RKC_DIB* dib
);

static void* AllocateCountedArray(size_t count, size_t elementSize) {
    size_t totalSize = sizeof(uint32_t) + (count * elementSize);
    auto* raw = (uint8_t*)std::malloc(totalSize);
    if (raw == nullptr) {
        return nullptr;
    }
    *(uint32_t*)raw = (uint32_t)count;
    void* array = raw + sizeof(uint32_t);
    std::memset(array, 0, count * elementSize);
    return array;
}

static uint32_t GetCountedArraySize(void* array) {
    if (array == nullptr) {
        return 0;
    }
    auto* raw = (uint8_t*)array - sizeof(uint32_t);
    return *(uint32_t*)raw;
}

static void FreeCountedArray(void* array) {
    if (array == nullptr) {
        return;
    }
    auto* raw = (uint8_t*)array - sizeof(uint32_t);
    std::free(raw);
}

static void DIB_Construct(void* dib) {
    CallFunctionInDLL<void*>("RKC_DIB.dll", "??0RKC_DIB@@QAE@XZ", dib);
}

static void DIB_Destruct(void* dib) {
    CallFunctionInDLL<void>("RKC_DIB.dll", "??1RKC_DIB@@QAE@XZ", dib);
}

static int DIB_Create(void* dib, long width, long height, long bpp, int allocBitmap) {
    return CallFunctionInDLL<int>("RKC_DIB.dll", "?Create@RKC_DIB@@QAEHJJJH@Z", dib, width, height, bpp, allocBitmap);
}

static void DIB_SetBitmap(void* dib, unsigned char* bitmap) {
    CallFunctionInDLL<unsigned char*>("RKC_DIB.dll", "?SetBitmap@RKC_DIB@@QAEPAEPAE@Z", dib, bitmap);
}

static int DIB_FillByte(void* dib, unsigned char fillValue) {
    return CallFunctionInDLL<int>("RKC_DIB.dll", "?FillByte@RKC_DIB@@QAEHE@Z", dib, fillValue);
}

static int DIB_SetPalette(void* dib, RGBQUAD* palette) {
    return CallFunctionInDLL<int>("RKC_DIB.dll", "?SetPalette@RKC_DIB@@QAEHPAUtagRGBQUAD@@@Z", dib, palette);
}

static int DIB_ZoomToDIB(void* dib, RECT* destRect, void* srcDib, RECT* srcRect, long transparentColor) {
    return CallFunctionInDLL<int>(
        "RKC_DIB.dll",
        "?ZoomToDIB@RKC_DIB@@QAEHPAUtagRECT@@PAV1@0J@Z",
        dib,
        destRect,
        srcDib,
        srcRect,
        transparentColor
    );
}

static int DIB_ZoomToDIBEx(
    void* dib, RECT* destRect, void* srcDib, RECT* srcRect,
    long paletteOffset, long transparentColor, long alpha, long flags) {
    return CallFunctionInDLL<int>(
        "RKC_DIB.dll",
        "?ZoomToDIBEx@RKC_DIB@@QAEHPAUtagRECT@@PAV1@0JJJJ@Z",
        dib, destRect, srcDib, srcRect, paletteOffset, transparentColor,
        alpha, flags);
}

static int DIB_TransferFast(
    void* dib, long x, long y, long width, long height,
    void* source, long sourceX, long sourceY) {
    return CallFunctionInDLL<int>(
        "RKC_DIB.dll",
        "?TransferToDIBFast@RKC_DIB@@QAEHJJJJPAV1@JJ@Z",
        dib, x, y, width, height, source, sourceX, sourceY);
}

static int DIB_Transfer(
    void* dib, long x, long y, long width, long height,
    void* source, long sourceX, long sourceY, long transparentColor) {
    return CallFunctionInDLL<int>(
        "RKC_DIB.dll",
        "?TransferToDIB@RKC_DIB@@QAEHJJJJPAV1@JJJ@Z",
        dib, x, y, width, height, source, sourceX, sourceY,
        transparentColor);
}

static int DIB_TransferEx(
    void* dib, long x, long y, long width, long height, void* source,
    long sourceX, long sourceY, long paletteOffset, long transparentColor,
    long alpha, long flags, void* highSpeedMode) {
    return CallFunctionInDLL<int>(
        "RKC_DIB.dll",
        "?TransferToDIBEx@RKC_DIB@@QAEHJJJJPAV1@JJJJJJPAVRKC_DIBHISPEEDMODE@@@Z",
        dib, x, y, width, height, source, sourceX, sourceY,
        paletteOffset, transparentColor, alpha, flags, highSpeedMode);
}

static int DIB_TransferEx(
    void* dib, long x, long y, void* source, long paletteOffset,
    long transparentColor, long alpha, long flags, void* highSpeedMode) {
    return CallFunctionInDLL<int>(
        "RKC_DIB.dll",
        "?TransferToDIBEx@RKC_DIB@@QAEHJJPAV1@JJJJPAVRKC_DIBHISPEEDMODE@@@Z",
        dib, x, y, source, paletteOffset, transparentColor, alpha, flags,
        highSpeedMode);
}

struct UpdFileCursor {
    std::vector<unsigned char> bytes;
    size_t position = 0;

    bool Load(const char* filename) {
        if (!filename)
            return false;
        HANDLE file = CreateFileA(
            filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return false;
        const DWORD size = GetFileSize(file, nullptr);
        if (size == INVALID_FILE_SIZE) {
            CloseHandle(file);
            return false;
        }
        bytes.resize(size);
        DWORD amount = 0;
        const bool success =
            size == 0
            || (ReadFile(file, bytes.data(), size, &amount, nullptr)
                && amount == size);
        CloseHandle(file);
        return success;
    }

    bool Read(void* output, size_t size) {
        if (size > bytes.size() - position)
            return false;
        if (size)
            std::memcpy(output, bytes.data() + position, size);
        position += size;
        return true;
    }

    template <typename Value>
    bool Read(Value& value) {
        return Read(&value, sizeof(value));
    }

    bool Skip(size_t size) {
        if (size > bytes.size() - position)
            return false;
        position += size;
        return true;
    }
};

static size_t UpdBitmapStride(long bitsPerPixel, long width, bool shadow) {
    if (shadow)
        return (static_cast<size_t>((width + 7) / 8) + 3u) & ~3u;
    if (bitsPerPixel == 4)
        return (static_cast<size_t>((width + 1) / 2) + 3u) & ~3u;
    return (static_cast<size_t>(width) + 3u) & ~3u;
}

static int DecodeUpdLz(
    const void* source, int sourceSize, void** output, void* header) {
    using Decode = int (__cdecl*)(const void*, int, void**, void*);
    HMODULE module = LoadLibraryA("RK_FUNCTION.dll");
    if (!module)
        return 0;
    auto decode = reinterpret_cast<Decode>(
        GetProcAddress(module, "RK_LzDecodeMemoryToMemory"));
    const int result =
        decode ? decode(source, sourceSize, output, header) : 0;
    FreeLibrary(module);
    return result;
}

static void* DIBHISPEEDMODE_Construct(void* mode) {
    return CallFunctionInDLL<void*>("RKC_DIB.dll", "??0RKC_DIBHISPEEDMODE@@QAE@XZ", mode);
}

static void DIBHISPEEDMODE_Destruct(void* mode) {
    CallFunctionInDLL<void>("RKC_DIB.dll", "??1RKC_DIBHISPEEDMODE@@QAE@XZ", mode);
}

// ============================================================================
// CLASS LAYOUTS (from reverse engineering)
// ============================================================================

/**
 * RKC_UPDIB_PATTERN class layout (0x28+ bytes):
 *   +0x00: long partsListCount   - Number of parts in pattern
 *   +0x04: RKC_UPDIB_PARTSLIST* partsList - Array of parts (28 bytes each)
 *   +0x08: RKC_UPDIB_JUDGE* judgement - Collision/hit detection data
 *   +0x0c: RECT buildRect        - Bounding rectangle (16 bytes: 0x0c-0x1b)
 *   +0x1c: long defaultPaletteNo - Default palette index
 *   +0x20: char* name            - Pattern name string
 *   +0x24: RKC_DIB* icon         - Icon/thumbnail DIB
 */

/**
 * RKC_UPDIB_UPD class layout (0x28+ bytes):
 *   +0x00: long type             - UPD type/format
 *   +0x04: char* filename        - Source filename
 *   +0x08: long status           - Load/ready status
 *   +0x0c: long partsCount       - Number of parts
 *   +0x10: RKC_UPDIB_UPD_PARTS* parts - Parts array
 *   +0x14: long patternCount     - Number of patterns
 *   +0x18: RKC_UPDIB_PATTERN* patterns - Patterns array
 *   +0x1c: long paletteCount     - Number of palettes
 *   +0x20: void* palettes        - Palette data
 *   +0x24: long versionNo        - UPD format version
 */

/**
 * RKC_UPDIB class layout (0x30+ bytes):
 *   +0x00: void* vsBlockList     - Linked list of VS blocks
 *   +0x04: long updCount         - Number of loaded UPDs
 *   +0x08: RKC_UPDIB_UPD** upds  - Array of UPD pointers
 *   ...
 *   +0x1c: RKC_DIB tempDIB       - Temporary DIB (embedded, 12 bytes)
 *   +0x28: RKC_DIBHISPEEDMODE* hispeedMode - Fast blending lookup tables
 *   +0x2c: RKC_DIB* dibPtr       - Another DIB pointer
 */

// ============================================================================
// RKC_UPDIB_PATTERN FUNCTIONS
// ============================================================================

/**
 * RKC_UPDIB_PATTERN::constructor - Initialize pattern object
 * USED BY: o_RKC_UPDIB.dll (internal)
 * 
 * Zeros fields and sets defaultPaletteNo to -1.
 */
extern "C" void* __thiscall RKC_UPDIB_PATTERN_constructor(void* self) {
    char* p = (char*)self;
    *(long*)(p + 0x00) = 0;  // partsListCount
    *(void**)(p + 0x04) = nullptr;  // partsList
    *(void**)(p + 0x08) = nullptr;  // judgement
    *(long*)(p + 0x1c) = -1; // defaultPaletteNo
    *(void**)(p + 0x20) = nullptr;  // name
    *(void**)(p + 0x24) = nullptr;  // icon
    return self;
}

/**
 * RKC_UPDIB_PATTERN::GetPartsListCount - Get number of parts
 * USED BY: ShadowFlare.exe, o_RKC_RPGSCRN.dll
 */
extern "C" long __thiscall RKC_UPDIB_PATTERN_GetPartsListCount(void* self) {
    return *(long*)((char*)self + 0x00);
}

/**
 * RKC_UPDIB_PATTERN::GetPartsList - Get parts list entry by index
 * USED BY: ShadowFlare.exe, o_RKC_RPGSCRN.dll
 * 
 * Returns pointer to parts list entry. Each entry is 28 bytes.
 */
extern "C" void* __thiscall RKC_UPDIB_PATTERN_GetPartsList(void* self, long index) {
    char* partsList = *(char**)((char*)self + 0x04);
    // Each RKC_UPDIB_PARTSLIST is 28 bytes (7 * 4)
    return partsList + index * 28;
}

/**
 * RKC_UPDIB_PATTERN::GetJudgement - Get collision/judgement data
 * USED BY: ShadowFlare.exe
 */
extern "C" void* __thiscall RKC_UPDIB_PATTERN_GetJudgement(void* self) {
    return *(void**)((char*)self + 0x08);
}

/**
 * RKC_UPDIB_PATTERN::GetBuildRect - Get bounding rectangle pointer
 * USED BY: o_RKC_RPGSCRN.dll, o_RKC_UPDIB.dll (internal)
 * 
 * Returns pointer to embedded RECT at offset 0x0c.
 */
extern "C" RECT* __thiscall RKC_UPDIB_PATTERN_GetBuildRect(void* self) {
    return (RECT*)((char*)self + 0x0c);
}

/**
 * RKC_UPDIB_PATTERN::GetDefaultPaletteNo - Get default palette index
 * USED BY: ShadowFlare.exe
 */
extern "C" long __thiscall RKC_UPDIB_PATTERN_GetDefaultPaletteNo(void* self) {
    return *(long*)((char*)self + 0x1c);
}

/**
 * RKC_UPDIB_PATTERN::GetName - Get pattern name string
 * USED BY: ShadowFlare.exe
 */
extern "C" char* __thiscall RKC_UPDIB_PATTERN_GetName(void* self) {
    return *(char**)((char*)self + 0x20);
}

/**
 * RKC_UPDIB_PATTERN::GetIcon - Get icon DIB
 * USED BY: ShadowFlare.exe
 */
extern "C" RKC_DIB* __thiscall RKC_UPDIB_PATTERN_GetIcon(void* self) {
    return *(RKC_DIB**)((char*)self + 0x24);
}

// ============================================================================
// RKC_UPDIB_UPD FUNCTIONS
// ============================================================================

/**
 * RKC_UPDIB_UPD::GetType - Get UPD type
 * USED BY: o_RKC_RPGSCRN.dll
 */
extern "C" long __thiscall RKC_UPDIB_UPD_GetType(void* self) {
    return *(long*)((char*)self + 0x00);
}

/**
 * RKC_UPDIB_UPD::GetFilename - Get source filename
 * USED BY: ShadowFlare.exe
 */
extern "C" char* __thiscall RKC_UPDIB_UPD_GetFilename(void* self) {
    return *(char**)((char*)self + 0x04);
}

/**
 * RKC_UPDIB_UPD::GetStatus - Get load status
 * USED BY: ShadowFlare.exe
 */
extern "C" long __thiscall RKC_UPDIB_UPD_GetStatus(void* self) {
    return *(long*)((char*)self + 0x08);
}

/**
 * RKC_UPDIB_UPD::SetStatus - Set load status
 * USED BY: ShadowFlare.exe
 */
extern "C" void __thiscall RKC_UPDIB_UPD_SetStatus(void* self, long status) {
    *(long*)((char*)self + 0x08) = status;
}

/**
 * RKC_UPDIB_UPD::GetPartsCount - Get number of parts
 * USED BY: ShadowFlare.exe
 */
extern "C" long __thiscall RKC_UPDIB_UPD_GetPartsCount(void* self) {
    return *(long*)((char*)self + 0x0c);
}

/**
 * RKC_UPDIB_UPD::GetParts - Get parts entry by index
 * USED BY: ShadowFlare.exe, o_RKC_UPDIB.dll (internal)
 *
 * Returns null when index >= partsCount. The original code does not reject
 * negative indices here, so we keep the same behavior.
 */
extern "C" void* __thiscall RKC_UPDIB_UPD_GetParts(void* self, long index) {
    OSF_FUNC_TRACE("self=%p, index=%ld", self, index);

    char* p = (char*)self;
    long partsCount = *(long*)(p + 0x0c);
    if (index >= partsCount) {
        return nullptr;
    }

    char* parts = *(char**)(p + 0x10);
    return parts + (index * 0x10);
}

/**
 * RKC_UPDIB_UPD::GetPatternCount - Get number of patterns
 * USED BY: ShadowFlare.exe, o_RKC_UPDIB.dll (internal)
 */
extern "C" long __thiscall RKC_UPDIB_UPD_GetPatternCount(void* self) {
    return *(long*)((char*)self + 0x14);
}

/**
 * RKC_UPDIB_UPD::GetPattern - Get pattern by index
 * USED BY: o_RKC_RPGSCRN.dll, o_RKC_UPDIB.dll (internal)
 *
 * Returns null when index >= patternCount. The original code does not reject
 * negative indices here, so we keep the same behavior.
 */
extern "C" void* __thiscall RKC_UPDIB_UPD_GetPattern(void* self, long index) {
    OSF_FUNC_TRACE("self=%p, index=%ld", self, index);

    char* p = (char*)self;
    long patternCount = *(long*)(p + 0x14);
    if (index >= patternCount) {
        return nullptr;
    }

    char* patterns = *(char**)(p + 0x18);
    return patterns + (index * 0x28);
}

/**
 * RKC_UPDIB_UPD::GetPaletteCount - Get number of palettes
 * USED BY: ShadowFlare.exe
 */
extern "C" long __thiscall RKC_UPDIB_UPD_GetPaletteCount(void* self) {
    return *(long*)((char*)self + 0x1c);
}

/**
 * RKC_UPDIB_UPD::GetVersionNo - Get UPD format version
 * USED BY: ShadowFlare.exe
 */
extern "C" long __thiscall RKC_UPDIB_UPD_GetVersionNo(void* self) {
    return *(long*)((char*)self + 0x24);
}

// ============================================================================
// RKC_UPDIB FUNCTIONS  
// ============================================================================

extern "C" void* __thiscall RKC_UPDIB_constructor(void* self) {
    char* p = (char*)self;
    DIB_Construct(p + 0x1c);

    *(void**)p = nullptr;
    *(long*)(p + 0x04) = 0;
    *(void**)(p + 0x08) = nullptr;
    *(long*)(p + 0x14) = 0;
    *(long*)(p + 0x18) = 0;
    *(void**)(p + 0x28) = nullptr;

    void* dibPtr = std::malloc(0x0c);
    if (dibPtr != nullptr) {
        DIB_Construct(dibPtr);
    }
    *(void**)(p + 0x2c) = dibPtr;
    if (dibPtr != nullptr) {
        DIB_Create(dibPtr, 1, 1, 8, 0);
    }

    return self;
}

extern "C" void __thiscall RKC_UPDIB_destructor(void* self) {
    char* p = (char*)self;
    RKC_UPDIB_Release(self);

    void* dibPtr = *(void**)(p + 0x2c);
    if (dibPtr != nullptr) {
        DIB_Destruct(dibPtr);
        std::free(dibPtr);
        *(void**)(p + 0x2c) = nullptr;
    }

    DIB_Destruct(p + 0x1c);
}

/**
 * RKC_UPDIB::GetUpdCount - Get number of loaded UPDs
 * USED BY: ShadowFlare.exe
 */
extern "C" long __thiscall RKC_UPDIB_GetUpdCount(void* self) {
    return *(long*)((char*)self + 0x04);
}

/**
 * RKC_UPDIB::GetDIBHISpeedMode - Get fast blending lookup table
 * USED BY: ShadowFlare.exe
 */
extern "C" RKC_DIBHISPEEDMODE* __thiscall RKC_UPDIB_GetDIBHISpeedMode(void* self) {
    return *(RKC_DIBHISPEEDMODE**)((char*)self + 0x28);
}

/**
 * RKC_UPDIB::GetFontParam - Get font cell size from pattern 0
 * USED BY: ShadowFlare.exe
 *
 * The original code looks up UPD index -> pattern 0 and returns the
 * pattern width and height divided by 16 using signed division semantics.
 */
extern "C" int __thiscall RKC_UPDIB_GetFontParam(void* self, long updIndex, long* outWidth, long* outHeight) {
    OSF_FUNC_TRACE("self=%p, updIndex=%ld, outWidth=%p, outHeight=%p", self, updIndex, outWidth, outHeight);

    void* upd = RKC_UPDIB_GetUpd(self, updIndex);
    if (upd == nullptr) {
        return 0;
    }

    void* pattern = RKC_UPDIB_UPD_GetPattern(upd, 0);
    if (pattern == nullptr) {
        return 0;
    }

    long width = *(long*)((char*)pattern + 0x14);
    long height = *(long*)((char*)pattern + 0x18);

    *outWidth = width / 16;
    *outHeight = height / 16;
    return 1;
}

extern "C" int __thiscall RKC_UPDIB_Initialize(void* self, long vsBlockCount, long vsCount, long updCount, int createHispeedMode) {
    OSF_FUNC_TRACE("self=%p, vsBlockCount=%ld, vsCount=%ld, updCount=%ld, createHispeedMode=%d",
        self, vsBlockCount, vsCount, updCount, createHispeedMode);
    RKC_UPDIB_Release(self);
    for (long index = 0; index < vsBlockCount; ++index) {
        void* block = RKC_UPDIB_InsertVSBlock(self, 0);
        if (!block || RKC_UPDIB_VSBLOCK_CreateVS(block, vsCount) != 1) {
            RKC_UPDIB_Release(self);
            return 0;
        }
    }
    if (updCount > 0 && RKC_UPDIB_CreateUpdBlock(self, updCount) != 1) {
        RKC_UPDIB_Release(self);
        return 0;
    }
    if (createHispeedMode == 1) {
        void* mode = std::malloc(0x11a300);
        if (!mode) {
            RKC_UPDIB_Release(self);
            return 0;
        }
        *(void**)((char*)self + 0x28) = DIBHISPEEDMODE_Construct(mode);
    }
    return 1;
}

extern "C" int __thiscall RKC_UPDIB_ReadUpd(
    void* self,
    long updIndex,
    char* filename,
    long flags,
    long previewWidth,
    long previewHeight,
    int rebuildTempDib
) {
    OSF_FUNC_TRACE("self=%p, updIndex=%ld, filename=%s, flags=%ld, previewWidth=%ld, previewHeight=%ld, rebuildTempDib=%d",
        self, updIndex, filename ? filename : "(null)", flags, previewWidth, previewHeight, rebuildTempDib);

    char packet[0x54];
    char dibStorage[0x0c];
    long patternIndex = 0;

    RKC_UPDIB_VSPACKET_constructor(packet);
    DIB_Construct(dibStorage);

    if (updIndex >= *(long*)((char*)self + 0x04)) {
        DIB_Destruct(dibStorage);
        RKC_UPDIB_VSPACKET_destructor(packet);
        return 0;
    }

    void* upd = RKC_UPDIB_GetUpd(self, updIndex);
    if (upd == nullptr || RKC_UPDIB_UPD_Read(upd, filename, flags) == 0) {
        if (upd != nullptr) {
            RKC_UPDIB_UPD_Release(upd);
        }
        DIB_Destruct(dibStorage);
        RKC_UPDIB_VSPACKET_destructor(packet);
        return 0;
    }

    if (rebuildTempDib == 1) {
        RKC_UPDIB_CreateTemporaryDIB(self);
    }

    if (previewWidth != 0 && previewHeight != 0) {
        RECT zoomDestRect;
        SetRect(&zoomDestRect, 0, 0, previewWidth, previewHeight);

        long patternCount = RKC_UPDIB_UPD_GetPatternCount(upd);
        while (patternIndex < patternCount) {
            char* pattern = (char*)RKC_UPDIB_UPD_GetPattern(upd, patternIndex);
            if (pattern != nullptr) {
                RKC_UPDIB_VSPACKET_SetPacket_full(
                    packet,
                    self,
                    updIndex,
                    patternIndex,
                    -1,
                    1,
                    -*(long*)(pattern + 0x0c),
                    -*(long*)(pattern + 0x10),
                    1000,
                    1000,
                    1000,
                    1000,
                    0,
                    1000,
                    1000,
                    1000,
                    nullptr,
                    nullptr
                );

                DIB_Create(dibStorage, *(long*)(pattern + 0x14), *(long*)(pattern + 0x18), 0x18, 1);
                RKC_UPDIB_VSPACKET_Render(packet, (RKC_DIB*)dibStorage, nullptr);

                if (*(void**)(pattern + 0x24) == nullptr) {
                    void* icon = std::malloc(0x0c);
                    if (icon != nullptr) {
                        DIB_Construct(icon);
                    }
                    *(void**)(pattern + 0x24) = icon;
                }

                void* icon = *(void**)(pattern + 0x24);
                if (icon != nullptr) {
                    DIB_Create(icon, previewWidth, previewHeight, 0x18, 1);
                    DIB_FillByte(icon, 0);

                    RECT srcRect;
                    SetRect(&srcRect, 0, 0, *(long*)(pattern + 0x14), *(long*)(pattern + 0x18));
                    DIB_ZoomToDIB(icon, &zoomDestRect, dibStorage, &srcRect, -1);

                    auto* palette = (RGBQUAD*)RKC_UPDIB_UPD_GetPalette(upd, *(long*)(pattern + 0x1c));
                    if (palette != nullptr) {
                        DIB_SetPalette(icon, palette);
                    }
                }
            }

            patternIndex++;
            patternCount = RKC_UPDIB_UPD_GetPatternCount(upd);
        }
    }

    DIB_Destruct(dibStorage);
    RKC_UPDIB_VSPACKET_destructor(packet);
    return 1;
}

/**
 * RKC_UPDIB::GetUpd - Get UPD by index
 * USED BY: ShadowFlare.exe, o_RKC_RPGSCRN.dll, o_RKC_UPDIB.dll (internal)
 * 
 * Returns null if index < 0 or >= updCount.
 */
extern "C" void* __thiscall RKC_UPDIB_GetUpd(void* self, long index) {
    char* p = (char*)self;
    
    // Bounds check - return null if index < 0
    if (index < 0) {
        return nullptr;
    }
    
    // Also check against updCount (at offset 0x04)
    long updCount = *(long*)(p + 0x04);
    if (index >= updCount) {
        return nullptr;
    }
    
    // Get array pointer at offset 0x08, return element at index
    void** upds = *(void***)(p + 0x08);
    return upds[index];
}

/**
 * RKC_UPDIB::DeleteUpd - Release one UPD entry
 * USED BY: ShadowFlare.exe
 *
 * The original code rejects negative indices and indices >= updCount.
 * When the release succeeds, it rebuilds the temp DIB only if rebuildTempDib
 * is exactly 1.
 */
extern "C" int __thiscall RKC_UPDIB_DeleteUpd(void* self, long index, int rebuildTempDib) {
    OSF_FUNC_TRACE("self=%p, index=%ld, rebuildTempDib=%d", self, index, rebuildTempDib);

    char* p = (char*)self;
    long updCount = *(long*)(p + 0x04);
    if (index < 0 || index >= updCount) {
        return 0;
    }

    void** upds = *(void***)(p + 0x08);
    void* upd = upds[index];
    RKC_UPDIB_UPD_Release(upd);

    if (rebuildTempDib == 1) {
        RKC_UPDIB_CreateTemporaryDIB(self);
    }

    return 1;
}

/**
 * RKC_UPDIB::GetVSBlock - Get VS block by index
 * USED BY: ShadowFlare.exe, o_RKC_RPGSCRN.dll
 *
 * The original code walks the linked list at offset 0x00 and follows the
 * nextBlock pointer at offset 0x10 until it reaches the requested index.
 */
extern "C" void* __thiscall RKC_UPDIB_GetVSBlock(void* self, long index) {
    OSF_FUNC_TRACE("self=%p, index=%ld", self, index);

    char* block = *(char**)self;
    long currentIndex = 0;

    while (block != nullptr) {
        if (currentIndex == index) {
            return block;
        }
        block = *(char**)(block + 0x10);
        currentIndex++;
    }

    return nullptr;
}

/**
 * RKC_UPDIB::CreateTemporaryDIB - Rebuild the temp 8bpp DIB from UPD parts
 * USED BY: o_RKC_RPGSCRN.dll
 *
 * The original code releases the embedded temp DIB at +0x1c, scans every UPD
 * part entry, and keeps the largest width/height seen in the cached fields at
 * +0x14 and +0x18. If both are nonzero, it recreates the temp DIB as 8bpp.
 */
extern "C" void __thiscall RKC_UPDIB_CreateTemporaryDIB(void* self) {
    OSF_FUNC_TRACE("self=%p", self);

    char* p = (char*)self;
    void* tempDib = p + 0x1c;
    CallFunctionInDLL<void>("RKC_DIB.dll", "?Release@RKC_DIB@@QAEXXZ", tempDib);

    long updCount = *(long*)(p + 0x04);
    if (updCount > 0) {
        void** upds = *(void***)(p + 0x08);
        for (long updIndex = 0; updIndex < updCount; ++updIndex) {
            char* upd = (char*)upds[updIndex];
            if (upd == nullptr) {
                continue;
            }

            long partsCount = *(long*)(upd + 0x0c);
            if (partsCount <= 0) {
                continue;
            }

            char* parts = *(char**)(upd + 0x10);
            for (long partIndex = 0; partIndex < partsCount; ++partIndex) {
                char* part = parts + (partIndex * 0x10);
                long width = *(long*)(part + 0x04);
                long height = *(long*)(part + 0x08);

                if (width > *(long*)(p + 0x14)) {
                    *(long*)(p + 0x14) = width;
                }
                if (height > *(long*)(p + 0x18)) {
                    *(long*)(p + 0x18) = height;
                }
            }
        }
    }

    long maxWidth = *(long*)(p + 0x14);
    if (maxWidth == 0) {
        return;
    }

    long maxHeight = *(long*)(p + 0x18);
    if (maxHeight == 0) {
        return;
    }

    CallFunctionInDLL<int>(
        "RKC_DIB.dll",
        "?Create@RKC_DIB@@QAEHJJJH@Z",
        tempDib,
        maxWidth,
        maxHeight,
        8L,
        1
    );
}

// ============================================================================
// RKC_UPDIB_VS FUNCTIONS
// ============================================================================

/**
 * RKC_UPDIB_VS class layout (8 bytes):
 *   +0x00: void* unknown1        - First pointer (linked list?)
 *   +0x04: void* vsPacketList    - First VSPacket in list
 */

/**
 * RKC_UPDIB_VS::constructor - Initialize VS object
 * USED BY: o_RKC_UPDIB.dll (internal)
 */
extern "C" void* __thiscall RKC_UPDIB_VS_constructor(void* self) {
    char* p = (char*)self;
    *(void**)(p + 0x00) = nullptr;
    *(void**)(p + 0x04) = nullptr;
    return self;
}

// ============================================================================
// RKC_UPDIB_VSBLOCK FUNCTIONS
// ============================================================================

/**
 * RKC_UPDIB_VSBLOCK class layout (0x14 bytes):
 *   +0x00: void* vsList         - List of VS objects
 *   +0x04: long vsCount         - Count of VS objects  
 *   +0x08: void* unknown1
 *   +0x0c: void* unknown2
 *   +0x10: void* nextBlock      - Next block in linked list
 */

/**
 * RKC_UPDIB_VSBLOCK::constructor - Initialize VSBLOCK object
 * USED BY: o_RKC_UPDIB.dll (internal)
 */
extern "C" void* __thiscall RKC_UPDIB_VSBLOCK_constructor(void* self) {
    char* p = (char*)self;
    *(void**)(p + 0x00) = nullptr;
    *(void**)(p + 0x04) = nullptr;
    *(void**)(p + 0x08) = nullptr;
    *(void**)(p + 0x0c) = nullptr;
    *(void**)(p + 0x10) = nullptr;
    return self;
}

/**
 * RKC_UPDIB_VSBLOCK::GetVScreen - Get VS entry by index
 * USED BY: o_RKC_RPGSCRN.dll
 *
 * Returns null when index >= vsCount. The original code does not reject
 * negative indices here, so we keep the same behavior.
 */
extern "C" void* __thiscall RKC_UPDIB_VSBLOCK_GetVScreen(void* self, long index) {
    OSF_FUNC_TRACE("self=%p, index=%ld", self, index);

    char* p = (char*)self;
    long vsCount = *(long*)(p + 0x04);
    if (index >= vsCount) {
        return nullptr;
    }

    char* vsArray = *(char**)(p + 0x08);
    return vsArray + (index * 8);
}

/**
 * RKC_UPDIB_VS::GetVSPacketCount - Count linked VS packets
 * USED BY: o_RKC_UPDIB.dll (internal)
 */
extern "C" long __thiscall RKC_UPDIB_VS_GetVSPacketCount(void* self) {
    OSF_FUNC_TRACE("self=%p", self);

    char* packet = *(char**)((char*)self + 0x04);
    long count = 0;

    while (packet != nullptr) {
        packet = *(char**)(packet + 0x3c);
        count++;
    }

    return count;
}

// ============================================================================
// RKC_UPDIB_VSPACKET FUNCTIONS
// ============================================================================

/**
 * RKC_UPDIB_VSPACKET class layout (0x54 bytes):
 *   +0x00-0x10: Various pointers/values (zeroed)
 *   +0x14-0x20: Four longs set to 1000 (0x3e8)
 *   +0x24-0x28: Two longs (zeroed)
 *   +0x2c: long set to -1
 *   +0x30-0x34: Three shorts set to 1000
 *   +0x38-0x3c: Two longs (zeroed)  
 *   +0x3c: void* nextPacket - Next packet in linked list
 *   +0x50: long (zeroed)
 */

/**
 * RKC_UPDIB_VSPACKET::constructor - Initialize VSPACKET object
 * USED BY: o_RKC_UPDIB.dll (internal)
 */
extern "C" void* __thiscall RKC_UPDIB_VSPACKET_constructor(void* self) {
    char* p = (char*)self;
    
    // Zero first block
    *(long*)(p + 0x00) = 0;
    *(long*)(p + 0x04) = 0;
    *(long*)(p + 0x08) = 0;
    *(long*)(p + 0x0c) = 0;
    *(long*)(p + 0x10) = 0;
    
    // Set four longs to 1000
    *(long*)(p + 0x14) = 1000;
    *(long*)(p + 0x18) = 1000;
    *(long*)(p + 0x1c) = 1000;
    *(long*)(p + 0x20) = 1000;
    
    // Zero and set special values
    *(long*)(p + 0x24) = 0;
    *(long*)(p + 0x28) = 0;
    *(long*)(p + 0x2c) = -1;
    
    // Three shorts set to 1000
    *(short*)(p + 0x30) = 1000;
    *(short*)(p + 0x32) = 1000;
    *(short*)(p + 0x34) = 1000;
    
    // Zero remaining
    *(long*)(p + 0x38) = 0;
    *(long*)(p + 0x3c) = 0;  // nextPacket
    *(long*)(p + 0x50) = 0;
    
    return self;
}

/**
 * RKC_UPDIB_VSPACKET::destructor - Destroy VSPACKET object
 * USED BY: o_RKC_UPDIB.dll (internal)
 * 
 * Empty destructor - no cleanup needed.
 */
extern "C" void __thiscall RKC_UPDIB_VSPACKET_destructor(void* self) {
    // Empty - nothing to clean up
}

// ============================================================================
// LOCAL CLASS IMPLEMENTATIONS
// ============================================================================

// RKC_UPDIB_PATTERN
extern "C" void __thiscall RKC_UPDIB_PATTERN_destructor(void* self) {}
extern "C" void* __thiscall RKC_UPDIB_PATTERN_operatorAssign(void* self, const void* src) { return self; }
extern "C" void __thiscall RKC_UPDIB_PATTERN_Release(void* self) {
    char* p = (char*)self;

    if (*(HGLOBAL*)(p + 0x04) != nullptr) {
        GlobalFree(*(HGLOBAL*)(p + 0x04));
        *(void**)(p + 0x04) = nullptr;
    }
    *(long*)(p + 0x00) = 0;

    if (*(HGLOBAL*)(p + 0x08) != nullptr) {
        GlobalFree(*(HGLOBAL*)(p + 0x08));
        *(void**)(p + 0x08) = nullptr;
    }

    *(long*)(p + 0x1c) = -1;

    if (*(HGLOBAL*)(p + 0x20) != nullptr) {
        GlobalFree(*(HGLOBAL*)(p + 0x20));
        *(void**)(p + 0x20) = nullptr;
    }

    void* icon = *(void**)(p + 0x24);
    if (icon != nullptr) {
        DIB_Destruct(icon);
        std::free(icon);
        *(void**)(p + 0x24) = nullptr;
    }
}

// RKC_UPDIB_UPD
extern "C" void* __thiscall RKC_UPDIB_UPD_constructor(void* self) {
    std::memset(self, 0, 0x30);
    return self;
}
extern "C" void __thiscall RKC_UPDIB_UPD_Release(void* self) {
    char* p = (char*)self;

    if (*(void**)(p + 0x10) != nullptr) {
        if (*(void**)(p + 0x28) == nullptr) {
            long partsCount = *(long*)(p + 0x0c);
            char* parts = *(char**)(p + 0x10);
            for (long i = 0; i < partsCount; ++i) {
                HGLOBAL mem = *(HGLOBAL*)(parts + 0x0c + (i * 0x10));
                if (mem != nullptr) {
                    GlobalFree(mem);
                    *(void**)(parts + 0x0c + (i * 0x10)) = nullptr;
                }
            }
        } else {
            std::free(*(void**)(p + 0x28));
            *(void**)(p + 0x28) = nullptr;
        }
        GlobalFree(*(HGLOBAL*)(p + 0x10));
    }
    *(void**)(p + 0x10) = nullptr;
    *(long*)(p + 0x0c) = 0;

    if (*(void**)(p + 0x18) != nullptr) {
        long patternCount = *(long*)(p + 0x14);
        char* patterns = *(char**)(p + 0x18);

        if (*(void**)(p + 0x2c) == nullptr) {
            for (long i = 0; i < patternCount; ++i) {
                GlobalFree(*(HGLOBAL*)(patterns + 0x04 + (i * 0x28)));
                *(void**)(patterns + 0x04 + (i * 0x28)) = nullptr;
            }
        } else {
            for (long i = 0; i < patternCount; ++i) {
                *(void**)(patterns + 0x04 + (i * 0x28)) = nullptr;
            }
            GlobalFree(*(HGLOBAL*)(p + 0x2c));
            *(void**)(p + 0x2c) = nullptr;
        }

        uint32_t count = GetCountedArraySize(patterns);
        for (uint32_t i = 0; i < count; ++i) {
            RKC_UPDIB_PATTERN_Release(patterns + (i * 0x28));
        }
        FreeCountedArray(patterns);
    }
    *(void**)(p + 0x18) = nullptr;
    *(long*)(p + 0x14) = 0;

    if (*(void**)(p + 0x20) != nullptr) {
        long paletteCount = *(long*)(p + 0x1c);
        char* dibs = *(char**)(p + 0x20);
        for (long i = 0; i < paletteCount; ++i) {
            DIB_SetBitmap(dibs + (i * 0x0c), nullptr);
        }
        uint32_t count = GetCountedArraySize(dibs);
        for (uint32_t i = 0; i < count; ++i) {
            DIB_Destruct(dibs + (i * 0x0c));
        }
        FreeCountedArray(dibs);
    }
    *(void**)(p + 0x20) = nullptr;
    *(long*)(p + 0x1c) = 0;

    *(long*)(p + 0x00) = 0;
    if (*(HGLOBAL*)(p + 0x04) != nullptr) {
        GlobalFree(*(HGLOBAL*)(p + 0x04));
        *(void**)(p + 0x04) = nullptr;
    }
    *(long*)(p + 0x24) = 0;
}
extern "C" void __thiscall RKC_UPDIB_UPD_destructor(void* self) {
    RKC_UPDIB_UPD_Release(self);
}
extern "C" void* __thiscall RKC_UPDIB_UPD_operatorAssign(void* self, const void* src) { return self; }
extern "C" RKC_DIB* __thiscall RKC_UPDIB_UPD_GetPaletteDIB(void* self, long index) {
    char* p = (char*)self;
    long paletteCount = *(long*)(p + 0x1c);
    if (index < 0 || index >= paletteCount) {
        return nullptr;
    }
    return (RKC_DIB*)(*(char**)(p + 0x20) + (index * 0x0c));
}
extern "C" void* __thiscall RKC_UPDIB_UPD_GetPalette(void* self, long index) {
    char* p = (char*)self;
    long paletteCount = *(long*)(p + 0x1c);
    if (index < 0 || index >= paletteCount) {
        return nullptr;
    }
    return *(void**)(*(char**)(p + 0x20) + 0x04 + (index * 0x0c));
}
extern "C" int __thiscall RKC_UPDIB_UPD_Read(void* self, char* filename, long flags) {
    OSF_FUNC_TRACE("self=%p, filename=%s, flags=%ld", self, filename ? filename : "(null)", flags);
    (void)flags;
    auto* object = static_cast<unsigned char*>(self);
    RKC_UPDIB_UPD_Release(self);

    UpdFileCursor input;
    if (!input.Load(filename))
        return 0;

    if (*(HGLOBAL*)(object + 4))
        GlobalFree(*(HGLOBAL*)(object + 4));
    const size_t filenameSize = std::strlen(filename) + 1;
    *(char**)(object + 4) =
        static_cast<char*>(GlobalAlloc(GPTR, filenameSize));
    if (!*(char**)(object + 4))
        return 0;
    std::memcpy(*(char**)(object + 4), filename, filenameSize);

    char header[16]{};
    if (!input.Read(header, sizeof(header)))
        return 0;
    const bool united = std::memcmp(header, "UnitePatData", 12) == 0;
    const bool noJudgement = std::memcmp(header, "NJudgeUniPat", 12) == 0;
    const bool shadow = std::memcmp(header, "ShadowLowPat", 12) == 0;
    if (!united && !noJudgement && !shadow)
        return 0;
    *(long*)object = united ? 1 : (noJudgement ? 2 : 4);

    const long version = std::strtol(header + 12, nullptr, 10);
    *(long*)(object + 0x24) = version;
    if (version < 0 || version >= 4)
        return 0;

    long partsCount = 0;
    if (!input.Read(partsCount) || partsCount < 0)
        return 0;
    *(long*)(object + 0x0c) = partsCount;

    const size_t partDataStart = input.position + (version > 2 ? 4u : 0u);
    size_t combinedPartBytes = 0;
    if (version > 2) {
        long ignored = 0;
        if (!input.Read(ignored))
            return 0;
        for (long index = 0; index < partsCount; ++index) {
            long bits = 0;
            long width = 0;
            long height = 0;
            long compressed = 0;
            if (!input.Read(bits) || !input.Read(width)
                || !input.Read(height) || !input.Read(compressed)
                || width < 0 || height < 0)
                return 0;
            if (shadow)
                bits = 1;
            const size_t bitmapSize =
                UpdBitmapStride(bits, width, shadow)
                * static_cast<size_t>(height);
            if (combinedPartBytes > SIZE_MAX - bitmapSize)
                return 0;
            combinedPartBytes += bitmapSize;
            if (compressed == 0) {
                if (!input.Skip(bitmapSize))
                    return 0;
            } else {
                unsigned char compressionHeader[16]{};
                if (!input.Read(compressionHeader, sizeof(compressionHeader)))
                    return 0;
                std::uint32_t payloadSize = 0;
                std::memcpy(&payloadSize, compressionHeader + 12, 4);
                if (!input.Skip(payloadSize))
                    return 0;
            }
        }
        input.position = partDataStart;
        if (combinedPartBytes) {
            *(void**)(object + 0x28) = std::malloc(combinedPartBytes);
            if (!*(void**)(object + 0x28))
                return 0;
        }
    }

    if (partsCount) {
        *(void**)(object + 0x10) =
            GlobalAlloc(GPTR, static_cast<SIZE_T>(partsCount) * 0x10);
        if (!*(void**)(object + 0x10))
            return 0;
    }

    size_t combinedOffset = 0;
    for (long index = 0; index < partsCount; ++index) {
        long bits = 0;
        long width = 0;
        long height = 0;
        long compressed = 0;
        if (!input.Read(bits) || !input.Read(width)
            || !input.Read(height) || !input.Read(compressed)
            || width < 0 || height < 0)
            return 0;
        if (shadow)
            bits = 1;
        const size_t bitmapSize =
            UpdBitmapStride(bits, width, shadow)
            * static_cast<size_t>(height);
        auto* part = static_cast<unsigned char*>(*(void**)(object + 0x10))
            + static_cast<size_t>(index) * 0x10;
        if (!bitmapSize) {
            *(void**)(part + 0x0c) = nullptr;
            continue;
        }

        void* temporary = nullptr;
        if (compressed == 0) {
            temporary = GlobalAlloc(0, bitmapSize);
            if (!temporary || !input.Read(temporary, bitmapSize)) {
                if (temporary)
                    GlobalFree(temporary);
                return 0;
            }
        } else {
            const size_t compressedStart = input.position;
            unsigned char compressionHeader[16]{};
            if (!input.Read(compressionHeader, sizeof(compressionHeader)))
                return 0;
            std::uint32_t payloadSize = 0;
            std::memcpy(&payloadSize, compressionHeader + 12, 4);
            if (!input.Skip(payloadSize))
                return 0;
            struct CompressionHeader {
                char magic[8];
                int uncompressedSize;
                int compressedSize;
            } decodedHeader{};
            if (payloadSize > 0x7fffffff - 16
                || !DecodeUpdLz(
                    input.bytes.data() + compressedStart,
                    static_cast<int>(payloadSize + 16),
                    &temporary, &decodedHeader))
                return 0;
        }

        *(long*)(part + 0x00) = bits;
        *(long*)(part + 0x04) = width;
        *(long*)(part + 0x08) = height;
        if (version < 3) {
            *(void**)(part + 0x0c) = GlobalAlloc(0, bitmapSize);
        } else {
            *(void**)(part + 0x0c) =
                static_cast<unsigned char*>(*(void**)(object + 0x28))
                + combinedOffset;
            combinedOffset += bitmapSize;
        }
        if (!*(void**)(part + 0x0c)) {
            GlobalFree(temporary);
            return 0;
        }
        std::memcpy(*(void**)(part + 0x0c), temporary, bitmapSize);
        GlobalFree(temporary);
    }

    long patternCount = 0;
    if (!input.Read(patternCount) || patternCount < 0)
        return 0;
    *(long*)(object + 0x14) = patternCount;
    long combinedListCount = 0;
    if (version > 2) {
        if (!input.Read(combinedListCount) || combinedListCount < 0)
            return 0;
        if (combinedListCount) {
            *(void**)(object + 0x2c) =
                GlobalAlloc(GPTR, static_cast<SIZE_T>(combinedListCount) * 0x1c);
            if (!*(void**)(object + 0x2c))
                return 0;
        }
    }

    if (patternCount) {
        *(void**)(object + 0x18) =
            AllocateCountedArray(patternCount, 0x28);
        if (!*(void**)(object + 0x18))
            return 0;
        for (long index = 0; index < patternCount; ++index)
            RKC_UPDIB_PATTERN_constructor(
                static_cast<unsigned char*>(*(void**)(object + 0x18))
                + static_cast<size_t>(index) * 0x28);
    }

    size_t combinedListOffset = 0;
    for (long patternIndex = 0; patternIndex < patternCount; ++patternIndex) {
        auto* pattern =
            static_cast<unsigned char*>(*(void**)(object + 0x18))
            + static_cast<size_t>(patternIndex) * 0x28;
        long listCount = 0;
        if (!input.Read(listCount) || listCount < 0
            || !input.Read(pattern + 0x0c, 0x10))
            return 0;
        *(long*)pattern = listCount;
        if (united) {
            *(void**)(pattern + 8) = GlobalAlloc(GPTR, 0xa8);
            if (!*(void**)(pattern + 8)
                || !input.Read(*(void**)(pattern + 8), 0xa8))
                return 0;
        }
        if (version > 0 && !input.Read(*(long*)(pattern + 0x1c)))
            return 0;

        if (listCount) {
            if (version < 3) {
                *(void**)(pattern + 4) =
                    GlobalAlloc(GPTR, static_cast<SIZE_T>(listCount) * 0x1c);
            } else {
                if (combinedListOffset
                    + static_cast<size_t>(listCount)
                    > static_cast<size_t>(combinedListCount))
                    return 0;
                *(void**)(pattern + 4) =
                    static_cast<unsigned char*>(*(void**)(object + 0x2c))
                    + combinedListOffset * 0x1c;
                combinedListOffset += listCount;
            }
            if (!*(void**)(pattern + 4))
                return 0;
        }

        for (long listIndex = 0; listIndex < listCount; ++listIndex) {
            auto* item = static_cast<unsigned char*>(*(void**)(pattern + 4))
                + static_cast<size_t>(listIndex) * 0x1c;
            long partIndex = -1;
            if (!input.Read(*(long*)(item + 0x00))
                || !input.Read(partIndex))
                return 0;
            if (partIndex < 0 || partIndex >= partsCount) {
                *(void**)(item + 0x18) = nullptr;
            } else {
                *(void**)(item + 0x18) =
                    static_cast<unsigned char*>(*(void**)(object + 0x10))
                    + static_cast<size_t>(partIndex) * 0x10;
            }
            if (!input.Read(item + 0x04, 8)
                || !input.Read(*(long*)(item + 0x0c))
                || !input.Read(*(long*)(item + 0x10))
                || !input.Read(*(long*)(item + 0x14)))
                return 0;
        }
        *(char**)(pattern + 0x20) = nullptr;
    }

    long paletteCount = 0;
    if (!input.Read(paletteCount) || paletteCount < 0)
        return 0;
    *(long*)(object + 0x1c) = paletteCount;
    if (paletteCount) {
        *(void**)(object + 0x20) =
            AllocateCountedArray(paletteCount, 0x0c);
        if (!*(void**)(object + 0x20))
            return 0;
        for (long index = 0; index < paletteCount; ++index) {
            auto* dib = static_cast<unsigned char*>(*(void**)(object + 0x20))
                + static_cast<size_t>(index) * 0x0c;
            DIB_Construct(dib);
            if (!DIB_Create(dib, 1, 1, 8, 0))
                return 0;
            auto* palette = *(RGBQUAD**)(dib + 4);
            for (long color = 0; color < 256; ++color) {
                unsigned char entry[4]{};
                if (!input.Read(entry, sizeof(entry)))
                    return 0;
                palette[color].rgbRed = entry[0];
                palette[color].rgbGreen = entry[1];
                palette[color].rgbBlue = entry[2];
                palette[color].rgbReserved = 0;
            }
        }
    }

    if (version > 1 && patternCount > 0) {
        long nameSize = 0;
        // Names were appended by later authoring tools, but many version 2/3
        // retail files end immediately after the palettes.
        if (input.Read(nameSize)) {
            if (nameSize < 0)
                return 0;
            for (long patternIndex = 0;
                 patternIndex < patternCount; ++patternIndex) {
                if (patternIndex > 0
                    && (!input.Read(nameSize) || nameSize < 0))
                    return 0;
                auto* pattern =
                    static_cast<unsigned char*>(*(void**)(object + 0x18))
                    + static_cast<size_t>(patternIndex) * 0x28;
                *(char**)(pattern + 0x20) =
                    static_cast<char*>(GlobalAlloc(0, nameSize + 1));
                if (!*(char**)(pattern + 0x20)
                    || !input.Read(*(char**)(pattern + 0x20), nameSize))
                    return 0;
                (*(char**)(pattern + 0x20))[nameSize] = '\0';
            }
        }
    }

    return 1;
}

// RKC_UPDIB
extern "C" void* __thiscall RKC_UPDIB_operatorAssign(void* self, const void* src) { return self; }
extern "C" int __thiscall RKC_UPDIB_CreateUpdBlock(void* self, long count) {
    OSF_FUNC_TRACE("self=%p, count=%ld", self, count);

    if (count < 1) {
        return 0;
    }

    char* p = (char*)self;
    void** oldUpds = *(void***)(p + 0x08);
    long oldCount = *(long*)(p + 0x04);

    HGLOBAL newBlock = GlobalAlloc(GMEM_FIXED, count * sizeof(void*));
    if (newBlock == nullptr) {
        return 0;
    }

    void** newUpds = (void**)newBlock;
    std::memset(newUpds, 0, count * sizeof(void*));

    long copied = 0;
    if (oldUpds != nullptr) {
        for (; copied < oldCount && copied < count; ++copied) {
            newUpds[copied] = oldUpds[copied];
            oldUpds[copied] = nullptr;
        }
    }

    for (; copied < count; ++copied) {
        void* upd = std::malloc(0x30);
        if (upd == nullptr) {
            for (long i = 0; i < copied; ++i) {
                if (i >= oldCount && newUpds[i] != nullptr) {
                    RKC_UPDIB_UPD_destructor(newUpds[i]);
                    std::free(newUpds[i]);
                }
            }
            GlobalFree(newBlock);
            return 0;
        }
        newUpds[copied] = RKC_UPDIB_UPD_constructor(upd);
    }

    if (oldUpds != nullptr) {
        for (long i = 0; i < oldCount; ++i) {
            if (oldUpds[i] != nullptr) {
                RKC_UPDIB_UPD_destructor(oldUpds[i]);
                std::free(oldUpds[i]);
            }
        }
        GlobalFree((HGLOBAL)oldUpds);
    }

    *(void***)(p + 0x08) = newUpds;
    *(long*)(p + 0x04) = count;
    return 1;
}
extern "C" int __thiscall RKC_UPDIB_DeleteVSBlock(void* self, long index) {
    OSF_FUNC_TRACE("self=%p, index=%ld", self, index);

    long blockCount = RKC_UPDIB_GetVSBlockCount(self);
    if (index < 0 || index >= blockCount) {
        return 0;
    }

    char* p = (char*)self;
    char* block = nullptr;

    if (index == 0) {
        block = *(char**)p;
        char* next = *(char**)(block + 0x10);
        *(char**)p = next;
        if (next != nullptr) {
            *(void**)(next + 0x0c) = nullptr;
        }
    } else {
        char* prev = (char*)RKC_UPDIB_GetVSBlock(self, index - 1);
        block = *(char**)(prev + 0x10);
        char* next = *(char**)(block + 0x10);
        if (next != nullptr) {
            *(char**)(next + 0x0c) = prev;
        }
        *(char**)(prev + 0x10) = next;
    }

    if (block != nullptr) {
        RKC_UPDIB_VSBLOCK_Release(block);
        std::free(block);
    }
    return 1;
}
/**
 * RKC_UPDIB::ExchangeUpd - Swap two UPD entries in the array
 * USED BY: o_RKC_UPDIB.dll (internal)
 */
extern "C" int __thiscall RKC_UPDIB_ExchangeUpd(void* self, long a, long b) {
    OSF_FUNC_TRACE("self=%p, a=%ld, b=%ld", self, a, b);

    long updCount = *(long*)((char*)self + 0x04);
    if (a < 0 || a >= updCount) {
        return 0;
    }
    if (b < 0 || b >= updCount) {
        return 0;
    }
    if (a == b) {
        return 0;
    }

    void** upds = *(void***)((char*)self + 0x08);
    void* tmp = upds[b];
    upds[b] = upds[a];
    upds[a] = tmp;
    return 1;
}
/**
 * RKC_UPDIB::GetVSBlockCount - Count linked VS blocks
 * USED BY: o_RKC_UPDIB.dll (internal)
 */
extern "C" long __thiscall RKC_UPDIB_GetVSBlockCount(void* self) {
    OSF_FUNC_TRACE("self=%p", self);

    char* block = *(char**)self;
    long count = 0;

    while (block != nullptr) {
        block = *(char**)(block + 0x10);
        count++;
    }

    return count;
}
extern "C" void* __thiscall RKC_UPDIB_InsertVSBlock(void* self, long index) {
    OSF_FUNC_TRACE("self=%p, index=%ld", self, index);

    long blockCount = RKC_UPDIB_GetVSBlockCount(self);
    if (index < 0 || index > blockCount) {
        return nullptr;
    }

    char* block = (char*)std::malloc(0x14);
    if (block == nullptr) {
        return nullptr;
    }
    RKC_UPDIB_VSBLOCK_constructor(block);
    *(void**)(block + 0x00) = self;

    if (index == 0) {
        *(void**)(block + 0x10) = *(void**)self;
        if (*(void**)self != nullptr) {
            *(void**)(*(char**)self + 0x0c) = block;
        }
        *(void**)self = block;
        return block;
    }

    char* prev = (char*)RKC_UPDIB_GetVSBlock(self, index - 1);
    char* next = *(char**)(prev + 0x10);
    *(void**)(block + 0x0c) = prev;
    *(void**)(block + 0x10) = next;
    if (next != nullptr) {
        *(void**)(next + 0x0c) = block;
    }
    *(void**)(prev + 0x10) = block;
    return block;
}
extern "C" void __thiscall RKC_UPDIB_Release(void* self) {
    char* p = (char*)self;

    void** upds = *(void***)(p + 0x08);
    long updCount = *(long*)(p + 0x04);
    if (upds != nullptr) {
        for (long i = 0; i < updCount; ++i) {
            if (upds[i] != nullptr) {
                RKC_UPDIB_UPD_destructor(upds[i]);
                std::free(upds[i]);
            }
        }
        GlobalFree((HGLOBAL)upds);
        *(void***)(p + 0x08) = nullptr;
    }
    *(long*)(p + 0x04) = 0;

    while (RKC_UPDIB_DeleteVSBlock(self, 0) == 1) {
    }

    void* hispeed = *(void**)(p + 0x28);
    if (hispeed != nullptr) {
        DIBHISPEEDMODE_Destruct(hispeed);
        std::free(hispeed);
        *(void**)(p + 0x28) = nullptr;
    }
}

extern "C" int __thiscall RKC_UPDIB_Render(
    void* self,
    RKC_DIB* dib,
    long vsBlockIndex,
    long reverseBlockOrder,
    long reversePacketOrder,
    long unusedY,
    RECT* clip
) {
    OSF_FUNC_TRACE("self=%p, dib=%p, vsBlockIndex=%ld, reverseBlockOrder=%ld, reversePacketOrder=%ld, unusedY=%ld, clip=%p",
        self, dib, vsBlockIndex, reverseBlockOrder, reversePacketOrder, unusedY, clip);
    if (vsBlockIndex == -1) {
        char* block = *(char**)self;
        if (reverseBlockOrder == 0) {
            if (block) {
                while (*(char**)(block + 0x10))
                    block = *(char**)(block + 0x10);
                while (block) {
                    RKC_UPDIB_VSBLOCK_Render(
                        block, dib, -1, reversePacketOrder, unusedY, clip);
                    block = *(char**)(block + 0x0c);
                }
            }
        } else {
            while (block) {
                RKC_UPDIB_VSBLOCK_Render(
                    block, dib, -1, reversePacketOrder, unusedY, clip);
                block = *(char**)(block + 0x10);
            }
        }
    } else {
        void* block = RKC_UPDIB_GetVSBlock(self, vsBlockIndex);
        if (block)
            RKC_UPDIB_VSBLOCK_Render(block, dib, -1, 0, 0, clip);
    }
    return 1;
}

// RKC_UPDIB_VS
extern "C" void __thiscall RKC_UPDIB_VS_destructor(void* self) {}
extern "C" void* __thiscall RKC_UPDIB_VS_operatorAssign(void* self, const void* src) { return self; }
extern "C" int __thiscall RKC_UPDIB_VS_DeleteVSPacket(void* self, long index) {
    OSF_FUNC_TRACE("self=%p, index=%ld", self, index);

    long count = RKC_UPDIB_VS_GetVSPacketCount(self);
    if (index < 0 || index >= count) {
        return 0;
    }

    char* p = (char*)self;
    char* packet = nullptr;

    if (index == 0) {
        packet = *(char**)(p + 0x04);
        char* next = *(char**)(packet + 0x3c);
        *(char**)(p + 0x04) = next;
        if (next != nullptr) {
            *(void**)(next + 0x38) = nullptr;
        }
    } else {
        char* prev = (char*)RKC_UPDIB_VS_GetVSPacket(self, index - 1);
        packet = *(char**)(prev + 0x3c);
        char* next = *(char**)(packet + 0x3c);
        if (next != nullptr) {
            *(char**)(next + 0x38) = prev;
        }
        *(char**)(prev + 0x3c) = next;
    }

    if (packet != nullptr) {
        RKC_UPDIB_VSPACKET_destructor(packet);
        std::free(packet);
    }
    return 1;
}
extern "C" void __thiscall RKC_UPDIB_VS_FlushVSPacket(void* self) {
    OSF_FUNC_TRACE("self=%p", self);

    char* packet = *(char**)((char*)self + 0x04);
    while (packet != nullptr) {
        char* next = *(char**)(packet + 0x3c);
        RKC_UPDIB_VSPACKET_destructor(packet);
        std::free(packet);
        packet = next;
    }
    *(void**)((char*)self + 0x04) = nullptr;
}
/**
 * RKC_UPDIB_VS::GetVSPacket - Get linked VS packet by index
 * USED BY: o_RKC_UPDIB.dll (internal)
 *
 * The original code walks the linked list at +0x04 and returns null if the
 * list ends before the requested index is reached.
 */
extern "C" void* __thiscall RKC_UPDIB_VS_GetVSPacket(void* self, long index) {
    OSF_FUNC_TRACE("self=%p, index=%ld", self, index);

    char* packet = *(char**)((char*)self + 0x04);
    long currentIndex = 0;

    while (packet != nullptr) {
        if (currentIndex == index) {
            return packet;
        }

        packet = *(char**)(packet + 0x3c);
        currentIndex++;
    }

    return nullptr;
}
extern "C" void* __thiscall RKC_UPDIB_VS_InsertVSPacket(void* self, long index) {
    OSF_FUNC_TRACE("self=%p, index=%ld", self, index);

    long count = RKC_UPDIB_VS_GetVSPacketCount(self);
    if (index < 0 || index > count) {
        return nullptr;
    }

    char* packet = (char*)std::malloc(0x54);
    if (packet == nullptr) {
        return nullptr;
    }
    RKC_UPDIB_VSPACKET_constructor(packet);
    *(void**)(packet + 0x00) = *(void**)self;

    char* p = (char*)self;
    if (index == 0) {
        *(void**)(packet + 0x3c) = *(void**)(p + 0x04);
        if (*(void**)(p + 0x04) != nullptr) {
            *(void**)(*(char**)(p + 0x04) + 0x38) = packet;
        }
        *(void**)(p + 0x04) = packet;
        return packet;
    }

    char* prev = (char*)RKC_UPDIB_VS_GetVSPacket(self, index - 1);
    char* next = *(char**)(prev + 0x3c);
    *(void**)(packet + 0x38) = prev;
    *(void**)(packet + 0x3c) = next;
    if (next != nullptr) {
        *(void**)(next + 0x38) = packet;
    }
    *(void**)(prev + 0x3c) = packet;
    return packet;
}
extern "C" void __thiscall RKC_UPDIB_VS_Release(void* self) {
    while (RKC_UPDIB_VS_DeleteVSPacket(self, 0) == 1) {
    }
    *(void**)self = nullptr;
}
extern "C" int __thiscall RKC_UPDIB_VS_Render(void* self, RKC_DIB* dib, long packetIndex, long reverseOrder, RECT* clip) {
    OSF_FUNC_TRACE("self=%p, dib=%p, packetIndex=%ld, reverseOrder=%ld, clip=%p", self, dib, packetIndex, reverseOrder, clip);

    if (packetIndex == -1) {
        char* packet = *(char**)((char*)self + 0x04);
        if (reverseOrder == 0) {
            if (packet != nullptr) {
                for (char* next = *(char**)(packet + 0x3c); next != nullptr; next = *(char**)(next + 0x3c)) {
                    packet = next;
                }
                while (packet != nullptr) {
                    RKC_UPDIB_VSPACKET_Render(packet, dib, clip);
                    packet = *(char**)(packet + 0x38);
                }
                return 1;
            }
        } else {
            while (packet != nullptr) {
                RKC_UPDIB_VSPACKET_Render(packet, dib, clip);
                packet = *(char**)(packet + 0x3c);
            }
            return 1;
        }
    } else {
        void* packet = RKC_UPDIB_VS_GetVSPacket(self, packetIndex);
        if (packet != nullptr) {
            RKC_UPDIB_VSPACKET_Render(packet, dib, clip);
        }
    }

    return 1;
}
extern "C" void* __thiscall RKC_UPDIB_VS_SetPacket(void* self, long index, void* packet) {
    OSF_FUNC_TRACE("self=%p, index=%ld, packet=%p", self, index, packet);

    void* newPacket = RKC_UPDIB_VS_InsertVSPacket(self, index);
    if (newPacket != nullptr) {
        RKC_UPDIB_VSPACKET_SetPacket_copy(newPacket, packet);
        *(void**)newPacket = *(void**)self;
    }
    return newPacket;
}
extern "C" void* __thiscall RKC_UPDIB_VS_SetPacket_full(
    void* self,
    long index,
    long updIndex,
    long patternIndex,
    long paletteIndex,
    long flags,
    long x,
    long y,
    long scaleX,
    long scaleY,
    long alpha,
    long blendMode,
    long userValue,
    short red,
    short green,
    short blue,
    RECT* clip,
    RKC_DIB* dib
) {
    OSF_FUNC_TRACE("self=%p, index=%ld, updIndex=%ld, patternIndex=%ld, paletteIndex=%ld, flags=%ld, x=%ld, y=%ld, scaleX=%ld, scaleY=%ld, alpha=%ld, blendMode=%ld, userValue=%ld, red=%d, green=%d, blue=%d, clip=%p, dib=%p",
        self, index, updIndex, patternIndex, paletteIndex, flags, x, y, scaleX, scaleY, alpha, blendMode, userValue, red, green, blue, clip, dib);
    void* packet = RKC_UPDIB_VS_InsertVSPacket(self, index);
    if (!packet)
        return nullptr;
    return RKC_UPDIB_VSPACKET_SetPacket_full(
        packet,
        *(void**)self,
        updIndex,
        patternIndex,
        paletteIndex,
        flags,
        x,
        y,
        scaleX,
        scaleY,
        alpha,
        blendMode,
        userValue,
        red,
        green,
        blue,
        clip,
        dib);
}

// RKC_UPDIB_VSBLOCK
extern "C" void __thiscall RKC_UPDIB_VSBLOCK_destructor(void* self) {}
extern "C" void* __thiscall RKC_UPDIB_VSBLOCK_operatorAssign(void* self, const void* src) { return self; }
extern "C" int __thiscall RKC_UPDIB_VSBLOCK_CreateVS(void* self, long count) {
    OSF_FUNC_TRACE("self=%p, count=%ld", self, count);

    if (count < 0) {
        return 0;
    }

    char* p = (char*)self;
    char* oldArray = *(char**)(p + 0x08);
    if (oldArray != nullptr) {
        uint32_t oldCount = GetCountedArraySize(oldArray);
        for (uint32_t i = 0; i < oldCount; ++i) {
            RKC_UPDIB_VS_Release(oldArray + (i * 8));
        }
        FreeCountedArray(oldArray);
    }

    char* vsArray = (char*)AllocateCountedArray((size_t)count, 8);
    if (count > 0 && vsArray == nullptr) {
        *(void**)(p + 0x08) = nullptr;
        *(long*)(p + 0x04) = 0;
        return 0;
    }

    *(char**)(p + 0x08) = vsArray;
    for (long i = 0; i < count; ++i) {
        RKC_UPDIB_VS_constructor(vsArray + (i * 8));
        *(void**)(vsArray + (i * 8)) = *(void**)p;
    }
    *(long*)(p + 0x04) = count;
    return 1;
}
extern "C" void __thiscall RKC_UPDIB_VSBLOCK_FlushVScreen(void* self) {
    OSF_FUNC_TRACE("self=%p", self);

    char* p = (char*)self;
    long vsCount = *(long*)(p + 0x04);
    char* vsArray = *(char**)(p + 0x08);
    for (long i = 0; i < vsCount; ++i) {
        RKC_UPDIB_VS_FlushVSPacket(vsArray + (i * 8));
    }
}

extern "C" void __thiscall RKC_UPDIB_FlushVSBlock(void* self, long index) {
    if (index == -1) {
        char* block = *(char**)self;
        while (block != nullptr) {
            RKC_UPDIB_VSBLOCK_FlushVScreen(block);
            block = *(char**)(block + 0x10);
        }
        return;
    }

    void* block = RKC_UPDIB_GetVSBlock(self, index);
    if (block != nullptr) {
        RKC_UPDIB_VSBLOCK_FlushVScreen(block);
    }
}
/**
 * RKC_UPDIB_VSBLOCK::GetVSCount - Return VS entry count
 * USED BY: o_RKC_UPDIB.dll (internal)
 */
extern "C" long __thiscall RKC_UPDIB_VSBLOCK_GetVSCount(void* self) {
    OSF_FUNC_TRACE("self=%p", self);
    return *(long*)((char*)self + 0x04);
}
extern "C" void __thiscall RKC_UPDIB_VSBLOCK_Release(void* self) {
    char* p = (char*)self;
    char* vsArray = *(char**)(p + 0x08);
    if (vsArray != nullptr) {
        uint32_t count = GetCountedArraySize(vsArray);
        for (uint32_t i = 0; i < count; ++i) {
            RKC_UPDIB_VS_Release(vsArray + (i * 8));
        }
        FreeCountedArray(vsArray);
    }
    *(void**)(p + 0x08) = nullptr;
    *(long*)(p + 0x04) = 0;
    *(void**)(p + 0x10) = nullptr;
    *(void**)p = nullptr;
}

extern "C" int __thiscall RKC_UPDIB_VSBLOCK_Render(
    void* self,
    RKC_DIB* dib,
    long vsIndex,
    long reverseVsOrder,
    long reversePacketOrder,
    RECT* clip
) {
    OSF_FUNC_TRACE("self=%p, dib=%p, vsIndex=%ld, reverseVsOrder=%ld, reversePacketOrder=%ld, clip=%p",
        self, dib, vsIndex, reverseVsOrder, reversePacketOrder, clip);
    char* p = static_cast<char*>(self);
    const long count = *(long*)(p + 0x04);
    char* screens = *(char**)(p + 0x08);
    if (vsIndex == -1) {
        if (reverseVsOrder == 0) {
            for (long index = count - 1; index >= 0; --index)
                RKC_UPDIB_VS_Render(
                    screens + index * 8, dib, -1, reversePacketOrder, clip);
        } else {
            for (long index = 0; index < count; ++index)
                RKC_UPDIB_VS_Render(
                    screens + index * 8, dib, -1, reversePacketOrder, clip);
        }
    } else if (vsIndex >= 0 && vsIndex < count) {
        RKC_UPDIB_VS_Render(screens + vsIndex * 8, dib, -1, 0, clip);
    }
    return 1;
}

// RKC_UPDIB_VSPACKET
extern "C" void* __thiscall RKC_UPDIB_VSPACKET_operatorAssign(void* self, const void* src) { return self; }
extern "C" int __thiscall RKC_UPDIB_VSPACKET_Render(void* self, RKC_DIB* dib, RECT* clip) {
    OSF_FUNC_TRACE("self=%p, dib=%p, clip=%p", self, dib, clip);
    auto* packet = static_cast<unsigned char*>(self);
    const unsigned long flags = *(unsigned long*)(packet + 0x04);
    if (flags & 0x100)
        return RKC_UPDIB_VSPACKET_RenderPoint(self, dib, clip);
    if (flags & 0x200)
        return RKC_UPDIB_VSPACKET_RenderLine(self, dib, clip);
    if (flags & 0x400)
        return RKC_UPDIB_VSPACKET_RenderBox(self, dib, clip);
    if (flags & 0x800)
        return RKC_UPDIB_VSPACKET_RenderFill(self, dib, clip);

    auto* updib = *(unsigned char**)packet;
    if (flags & 0x1000) {
        auto* source = *(unsigned char**)(packet + 0x50);
        if (!source)
            return 1;
        auto* sourceInfo = *(BITMAPINFOHEADER**)source;
        if (!sourceInfo)
            return 1;
        long destinationX = *(long*)(packet + 0x0c);
        long destinationY = *(long*)(packet + 0x10);
        long sourceX = 0;
        long sourceY = 0;
        long right = destinationX + sourceInfo->biWidth - 1;
        long bottom = destinationY + sourceInfo->biHeight - 1;
        if (flags & 0x20) {
            const auto* packetClip = reinterpret_cast<const RECT*>(packet + 0x40);
            if (destinationX < packetClip->left) {
                sourceX = packetClip->left - destinationX;
                destinationX = packetClip->left;
            }
            if (right > packetClip->right)
                right = packetClip->right;
            if (destinationY < packetClip->top) {
                sourceY = packetClip->top - destinationY;
                destinationY = packetClip->top;
            }
            if (bottom > packetClip->bottom)
                bottom = packetClip->bottom;
        }
        const long width = right - destinationX + 1;
        const long height = bottom - destinationY + 1;
        const long transparentColor = (flags & 1) ? -1 : 0;
        if (*(long*)(packet + 0x1c) == 1000
            && *(long*)(packet + 0x08) == 0
            && (flags & 0x0a) == 0) {
            if (transparentColor == -1)
                DIB_TransferFast(
                    dib, destinationX, destinationY, width, height,
                    source, sourceX, sourceY);
            else
                DIB_Transfer(
                    dib, destinationX, destinationY, width, height,
                    source, sourceX, sourceY, transparentColor);
            return 1;
        }
        long effects = 0;
        if (flags & 2)
            effects |= 4;
        if (flags & 8)
            effects |= 0x10;
        DIB_TransferEx(
            dib, destinationX, destinationY, width, height,
            source, sourceX, sourceY, *(long*)(packet + 8),
            transparentColor, *(long*)(packet + 0x1c), effects,
            updib ? *(void**)(updib + 0x28) : nullptr);
        return 1;
    }

    if (!updib)
        return 0;
    auto* upd = static_cast<unsigned char*>(
        RKC_UPDIB_GetUpd(updib, *(long*)(packet + 0x24)));
    if (!upd)
        return 0;
    auto* pattern = static_cast<unsigned char*>(
        RKC_UPDIB_UPD_GetPattern(upd, *(long*)(packet + 0x28)));
    if (!pattern)
        return 0;

    long paletteNumber = *(long*)(packet + 0x2c);
    if (paletteNumber == -1)
        paletteNumber = *(long*)(pattern + 0x1c);
    auto* selectedPalette = static_cast<RGBQUAD*>(
        RKC_UPDIB_UPD_GetPalette(upd, paletteNumber));
    if (!selectedPalette)
        return 0;

    unsigned char* paletteDib = nullptr;
    if (*(short*)(packet + 0x30) == 1000
        && *(short*)(packet + 0x32) == 1000
        && *(short*)(packet + 0x34) == 1000
        && (flags & 0x10)) {
        paletteDib = reinterpret_cast<unsigned char*>(
            RKC_UPDIB_UPD_GetPaletteDIB(upd, paletteNumber));
        if (!paletteDib)
            return 0;
    } else {
        paletteDib = *(unsigned char**)(updib + 0x2c);
        if (!paletteDib)
            return 0;
    }

    auto* build = reinterpret_cast<RECT*>(pattern + 0x0c);
    if (build->right == 0 || build->bottom == 0)
        return 0;

    const long listCount = *(long*)pattern;
    auto* lists = *(unsigned char**)(pattern + 4);
    for (long listIndex = 0; listIndex < listCount; ++listIndex) {
        auto* list = lists + static_cast<size_t>(listIndex) * 0x1c;
        auto* part = *(unsigned char**)(list + 0x18);
        if (!part)
            return 0;
        auto* info = *(BITMAPINFOHEADER**)paletteDib;
        if (!info)
            return 0;
        info->biBitCount = static_cast<WORD>(*(long*)part);
        info->biWidth = *(long*)(part + 4);
        info->biHeight = *(long*)(part + 8);
        DIB_SetBitmap(paletteDib, *(unsigned char**)(part + 0x0c));

        const long transparentColor = (*(long*)part == 4) ? 0 : -16;
        const long colorCount = transparentColor + 16;
        long flip = 0;
        if ((*(unsigned long*)list ^ flags) & 0x40000000)
            flip |= 1;
        if ((*(unsigned long*)list ^ flags) & 0x80000000)
            flip |= 2;
        unsigned char* sourceDib = paletteDib;
        if (flip) {
            sourceDib = updib + 0x1c;
            DIB_TransferEx(
                sourceDib, 0, 0, paletteDib, 0, -1, 1000, flip,
                *(void**)(updib + 0x28));
        }

        const long partWidth = *(long*)(part + 4);
        const long partHeight = *(long*)(part + 8);
        long relativeX = 0;
        long relativeY = 0;
        if ((flags & 0x40000000) == 0)
            relativeX = *(long*)(list + 4);
        else
            relativeX =
                -(*(long*)(list + 0x10) * partWidth / 1000)
                - *(long*)(list + 4);
        if ((flags & 0x80000000) == 0)
            relativeY = *(long*)(list + 8);
        else
            relativeY =
                -(*(long*)(list + 0x14) * partHeight / 1000)
                - *(long*)(list + 8);

        RECT destination{
            *(long*)(packet + 0x0c)
                + *(long*)(packet + 0x14) * relativeX / 1000,
            *(long*)(packet + 0x10)
                + *(long*)(packet + 0x18) * relativeY / 1000,
            *(long*)(packet + 0x0c)
                + *(long*)(packet + 0x14)
                    * (relativeX + *(long*)(list + 0x10) * partWidth / 1000)
                    / 1000
                - 1,
            *(long*)(packet + 0x10)
                + *(long*)(packet + 0x18)
                    * (relativeY + *(long*)(list + 0x14) * partHeight / 1000)
                    / 1000
                - 1
        };
        RECT source{0, 0, partWidth - 1, partHeight - 1};

        auto applyClip = [&](const RECT& bounds) {
            if (destination.left < bounds.left) {
                source.left += bounds.left - destination.left;
                destination.left = bounds.left;
            }
            if (destination.right > bounds.right) {
                source.right += bounds.right - destination.right;
                destination.right = bounds.right;
            }
            if (destination.top < bounds.top) {
                source.top += bounds.top - destination.top;
                destination.top = bounds.top;
            }
            if (destination.bottom > bounds.bottom) {
                source.bottom += bounds.bottom - destination.bottom;
                destination.bottom = bounds.bottom;
            }
        };
        if (clip && !(flags & 4)
            && *(long*)(packet + 0x14) == 1000
            && *(long*)(packet + 0x18) == 1000)
            applyClip(*clip);
        if ((flags & 0x20)
            && *(long*)(packet + 0x14) == 1000
            && *(long*)(packet + 0x18) == 1000)
            applyClip(*reinterpret_cast<RECT*>(packet + 0x40));

        destination.right = destination.right - destination.left + 1;
        destination.bottom = destination.bottom - destination.top + 1;
        source.right = source.right - source.left + 1;
        source.bottom = source.bottom - source.top + 1;

        const long paletteOffset =
            *(long*)(list + 0x0c) + *(long*)(packet + 8);
        const long transparentIndex = (flags & 1) ? -1 : 0;
        if (destination.right > 0 && destination.bottom > 0
            && source.right > 0 && source.bottom > 0) {
            auto* activePalette = *(RGBQUAD**)(sourceDib + 4);
            if (*(long*)part == 1) {
                if (activePalette) {
                    auto channel = [](short strength) -> BYTE {
                        if (strength < 1000)
                            return 0;
                        return static_cast<BYTE>(
                            ((strength - 1000) * 255) / 1000);
                    };
                    activePalette[1].rgbRed =
                        channel(*(short*)(packet + 0x30));
                    activePalette[1].rgbGreen =
                        channel(*(short*)(packet + 0x32));
                    activePalette[1].rgbBlue =
                        channel(*(short*)(packet + 0x34));
                }
            } else if (!(flags & 0x80)) {
                DIB_SetPalette(sourceDib, selectedPalette);
                activePalette = *(RGBQUAD**)(sourceDib + 4);
                if (activePalette && colorCount > 0) {
                    const short redOffset =
                        *(short*)(packet + 0x30) - 1000;
                    const short greenOffset =
                        *(short*)(packet + 0x32) - 1000;
                    const short blueOffset =
                        *(short*)(packet + 0x34) - 1000;
                    auto adjust = [](BYTE value, short amount) -> BYTE {
                        const int delta = amount < 1
                            ? static_cast<int>(value) * amount / 1000
                            : (255 - static_cast<int>(value)) * amount / 1000;
                        return static_cast<BYTE>(
                            static_cast<int>(value) + delta);
                    };
                    for (long color = 0; color < colorCount; ++color) {
                        activePalette[color].rgbRed =
                            adjust(activePalette[color].rgbRed, redOffset);
                        activePalette[color].rgbGreen =
                            adjust(activePalette[color].rgbGreen, greenOffset);
                        activePalette[color].rgbBlue =
                            adjust(activePalette[color].rgbBlue, blueOffset);
                    }
                    if (flags & 0x10) {
                        for (long color = 0; color < colorCount; ++color) {
                            activePalette[color].rgbRed =
                                255 - activePalette[color].rgbRed;
                            activePalette[color].rgbGreen =
                                255 - activePalette[color].rgbGreen;
                            activePalette[color].rgbBlue =
                                255 - activePalette[color].rgbBlue;
                        }
                    }
                    if (flags & 0x2000) {
                        for (long color = 0; color < colorCount; ++color) {
                            BYTE grey = activePalette[color].rgbRed;
                            if (activePalette[color].rgbGreen > grey)
                                grey = activePalette[color].rgbGreen;
                            if (activePalette[color].rgbBlue > grey)
                                grey = activePalette[color].rgbBlue;
                            activePalette[color].rgbRed = grey;
                            activePalette[color].rgbGreen = grey;
                            activePalette[color].rgbBlue = grey;
                        }
                    }
                }
            } else if (activePalette) {
                activePalette[0] = selectedPalette[0];
                activePalette[2] = selectedPalette[2];
                activePalette[1].rgbRed =
                    static_cast<BYTE>(*(short*)(packet + 0x30));
                activePalette[1].rgbGreen =
                    static_cast<BYTE>(*(short*)(packet + 0x32));
                activePalette[1].rgbBlue =
                    static_cast<BYTE>(*(short*)(packet + 0x34));
            }

            const bool sameSize =
                source.right == destination.right
                && source.bottom == destination.bottom;
            if (sameSize) {
                if (*(long*)(packet + 0x1c) == 1000
                    && paletteOffset == 0 && (flags & 0x0a) == 0) {
                    if (transparentIndex == -1)
                        DIB_TransferFast(
                            dib, destination.left, destination.top,
                            destination.right, destination.bottom, sourceDib,
                            source.left, source.top);
                    else
                        DIB_Transfer(
                            dib, destination.left, destination.top,
                            destination.right, destination.bottom, sourceDib,
                            source.left, source.top, transparentIndex);
                } else {
                    long effects = 0;
                    if (flags & 2)
                        effects |= 4;
                    if (flags & 8)
                        effects |= 0x10;
                    DIB_TransferEx(
                        dib, destination.left, destination.top,
                        destination.right, destination.bottom, sourceDib,
                        source.left, source.top, paletteOffset,
                        transparentIndex, *(long*)(packet + 0x1c), effects,
                        *(void**)(updib + 0x28));
                }
            } else if (*(long*)(packet + 0x1c) == 1000
                && paletteOffset == 0 && (flags & 0x0a) == 0) {
                DIB_ZoomToDIB(
                    dib, &destination, sourceDib, &source, transparentIndex);
            } else {
                long effects = 0;
                if (flags & 2)
                    effects |= 4;
                if (flags & 8)
                    effects |= 0x10;
                DIB_ZoomToDIBEx(
                    dib, &destination, sourceDib, &source, paletteOffset,
                    transparentIndex, *(long*)(packet + 0x1c), effects);
            }
        }
        DIB_SetBitmap(paletteDib, nullptr);
    }
    return 1;
}
extern "C" int __thiscall RKC_UPDIB_VSPACKET_RenderBox(void* self, RKC_DIB* dib, RECT* clip) {
    OSF_FUNC_TRACE("self=%p, dib=%p, clip=%p", self, dib, clip);
    char* p = static_cast<char*>(self);
    unsigned long drawFlags = (*(unsigned long*)(p + 4) & 2) ? 4 : 0;
    if (*(unsigned long*)(p + 4) & 0x40)
        drawFlags |= 8;
    if (*(unsigned long*)(p + 4) & 4)
        clip = nullptr;
    return CallFunctionInDLL<int>(
        "RKC_DIB.dll",
        "?DrawBox@RKC_DIB@@QAEHJJJJEEEJJPAUtagRECT@@@Z",
        dib,
        *(long*)(p + 0x0c), *(long*)(p + 0x10),
        *(long*)(p + 0x14), *(long*)(p + 0x18),
        (unsigned char)*(short*)(p + 0x30),
        (unsigned char)*(short*)(p + 0x32),
        (unsigned char)*(short*)(p + 0x34),
        *(long*)(p + 0x1c), (long)drawFlags, clip);
}
extern "C" int __thiscall RKC_UPDIB_VSPACKET_RenderFill(void* self, RKC_DIB* dib, RECT* clip) {
    OSF_FUNC_TRACE("self=%p, dib=%p, clip=%p", self, dib, clip);
    char* p = static_cast<char*>(self);
    unsigned long drawFlags = (*(unsigned long*)(p + 4) & 2) ? 4 : 0;
    if (*(unsigned long*)(p + 4) & 0x40)
        drawFlags |= 8;
    if (*(unsigned long*)(p + 4) & 4)
        clip = nullptr;
    void* updib = *(void**)p;
    void* highSpeed = updib ? *(void**)((char*)updib + 0x28) : nullptr;
    return CallFunctionInDLL<int>(
        "RKC_DIB.dll",
        "?DrawFill@RKC_DIB@@QAEHJJJJEEEJJPAUtagRECT@@PAVRKC_DIBHISPEEDMODE@@@Z",
        dib,
        *(long*)(p + 0x0c), *(long*)(p + 0x10),
        *(long*)(p + 0x14), *(long*)(p + 0x18),
        (unsigned char)*(short*)(p + 0x30),
        (unsigned char)*(short*)(p + 0x32),
        (unsigned char)*(short*)(p + 0x34),
        *(long*)(p + 0x1c), (long)drawFlags, clip, highSpeed);
}
extern "C" int __thiscall RKC_UPDIB_VSPACKET_RenderLine(void* self, RKC_DIB* dib, RECT* clip) {
    OSF_FUNC_TRACE("self=%p, dib=%p, clip=%p", self, dib, clip);
    char* p = static_cast<char*>(self);
    unsigned long drawFlags = (*(unsigned long*)(p + 4) & 2) ? 4 : 0;
    if (*(unsigned long*)(p + 4) & 0x40)
        drawFlags |= 8;
    if (*(unsigned long*)(p + 4) & 4)
        clip = nullptr;
    return CallFunctionInDLL<int>(
        "RKC_DIB.dll",
        "?DrawLine@RKC_DIB@@QAEHJJJJEEEJJPAUtagRECT@@@Z",
        dib,
        *(long*)(p + 0x0c), *(long*)(p + 0x10),
        *(long*)(p + 0x14), *(long*)(p + 0x18),
        (unsigned char)*(short*)(p + 0x30),
        (unsigned char)*(short*)(p + 0x32),
        (unsigned char)*(short*)(p + 0x34),
        *(long*)(p + 0x1c), (long)drawFlags, clip);
}
extern "C" int __thiscall RKC_UPDIB_VSPACKET_RenderPoint(void* self, RKC_DIB* dib, RECT* clip) {
    OSF_FUNC_TRACE("self=%p, dib=%p, clip=%p", self, dib, clip);
    char* p = static_cast<char*>(self);
    unsigned long drawFlags = (*(unsigned long*)(p + 4) & 2) ? 4 : 0;
    if (*(unsigned long*)(p + 4) & 0x40)
        drawFlags |= 8;
    if (*(unsigned long*)(p + 4) & 4)
        clip = nullptr;
    return CallFunctionInDLL<int>(
        "RKC_DIB.dll",
        "?DrawPoint@RKC_DIB@@QAEHJJEEEJJPAUtagRECT@@@Z",
        dib,
        *(long*)(p + 0x0c), *(long*)(p + 0x10),
        (unsigned char)*(short*)(p + 0x30),
        (unsigned char)*(short*)(p + 0x32),
        (unsigned char)*(short*)(p + 0x34),
        *(long*)(p + 0x1c), (long)drawFlags, clip);
}
extern "C" void* __thiscall RKC_UPDIB_VSPACKET_SetPacket_copy(void* self, void* packet) {
    OSF_FUNC_TRACE("self=%p, packet=%p", self, packet);

    char* dst = (char*)self;
    char* src = (char*)packet;
    *(void**)(dst + 0x00) = *(void**)(src + 0x00);
    *(long*)(dst + 0x04) = *(long*)(src + 0x04);
    *(long*)(dst + 0x08) = *(long*)(src + 0x08);
    *(long*)(dst + 0x0c) = *(long*)(src + 0x0c);
    *(long*)(dst + 0x10) = *(long*)(src + 0x10);
    *(long*)(dst + 0x14) = *(long*)(src + 0x14);
    *(long*)(dst + 0x18) = *(long*)(src + 0x18);
    *(long*)(dst + 0x1c) = *(long*)(src + 0x1c);
    *(long*)(dst + 0x20) = *(long*)(src + 0x20);
    *(long*)(dst + 0x24) = *(long*)(src + 0x24);
    *(long*)(dst + 0x28) = *(long*)(src + 0x28);
    *(long*)(dst + 0x2c) = *(long*)(src + 0x2c);
    *(short*)(dst + 0x30) = *(short*)(src + 0x30);
    *(short*)(dst + 0x32) = *(short*)(src + 0x32);
    *(short*)(dst + 0x34) = *(short*)(src + 0x34);
    *(long*)(dst + 0x40) = *(long*)(src + 0x40);
    *(long*)(dst + 0x44) = *(long*)(src + 0x44);
    *(long*)(dst + 0x48) = *(long*)(src + 0x48);
    *(long*)(dst + 0x4c) = *(long*)(src + 0x4c);
    *(long*)(dst + 0x50) = *(long*)(src + 0x50);
    return self;
}
extern "C" void* __thiscall RKC_UPDIB_VSPACKET_SetPacket_full(
    void* self,
    void* updib,
    long updIndex,
    long patternIndex,
    long paletteIndex,
    long flags,
    long x,
    long y,
    long scaleX,
    long scaleY,
    long alpha,
    long blendMode,
    long userValue,
    short red,
    short green,
    short blue,
    RECT* clip,
    RKC_DIB* dib
) {
    OSF_FUNC_TRACE("self=%p, updib=%p, updIndex=%ld, patternIndex=%ld, paletteIndex=%ld, flags=%ld, x=%ld, y=%ld, scaleX=%ld, scaleY=%ld, alpha=%ld, blendMode=%ld, userValue=%ld, red=%d, green=%d, blue=%d, clip=%p, dib=%p",
        self, updib, updIndex, patternIndex, paletteIndex, flags, x, y, scaleX, scaleY, alpha, blendMode, userValue, red, green, blue, clip, dib);

    char* p = (char*)self;
    *(void**)(p + 0x00) = updib;
    *(long*)(p + 0x08) = userValue;
    *(long*)(p + 0x10) = y;
    *(long*)(p + 0x04) = flags;
    *(long*)(p + 0x18) = scaleY;
    *(long*)(p + 0x0c) = x;
    *(long*)(p + 0x20) = blendMode;
    *(long*)(p + 0x14) = scaleX;
    *(long*)(p + 0x28) = patternIndex;
    *(long*)(p + 0x1c) = alpha;
    *(short*)(p + 0x30) = red;
    *(long*)(p + 0x24) = updIndex;
    *(short*)(p + 0x34) = blue;
    *(long*)(p + 0x2c) = paletteIndex;
    *(short*)(p + 0x32) = green;

    if (clip != nullptr) {
        *(long*)(p + 0x40) = clip->left;
        *(long*)(p + 0x44) = clip->top;
        *(long*)(p + 0x48) = clip->right;
        *(long*)(p + 0x4c) = clip->bottom;
    }

    *(RKC_DIB**)(p + 0x50) = dib;
    return self;
}

extern "C" int __thiscall RKC_UPDIB_SetPacket(
    void* self,
    long vsBlockIndex,
    long vsIndex,
    long updIndex,
    long patternIndex,
    long paletteIndex,
    long flags,
    long x,
    long y,
    long scaleX,
    long scaleY,
    long alpha,
    long blendMode,
    long userValue,
    short red,
    short green,
    short blue,
    RECT* clip,
    RKC_DIB* dib
) {
    OSF_FUNC_TRACE("self=%p, vsBlockIndex=%ld, vsIndex=%ld, updIndex=%ld, patternIndex=%ld, paletteIndex=%ld, flags=%ld, x=%ld, y=%ld, scaleX=%ld, scaleY=%ld, alpha=%ld, blendMode=%ld, userValue=%ld, red=%d, green=%d, blue=%d, clip=%p, dib=%p",
        self, vsBlockIndex, vsIndex, updIndex, patternIndex, paletteIndex, flags, x, y, scaleX, scaleY, alpha, blendMode, userValue, red, green, blue, clip, dib);
    void* block = RKC_UPDIB_GetVSBlock(self, vsBlockIndex);
    if (!block)
        return 0;
    void* screen = RKC_UPDIB_VSBLOCK_GetVScreen(block, vsIndex);
    if (!screen)
        return 0;
    char packet[0x54];
    RKC_UPDIB_VSPACKET_constructor(packet);
    RKC_UPDIB_VSPACKET_SetPacket_full(
        packet, self, updIndex, patternIndex, paletteIndex, flags,
        x, y, scaleX, scaleY, alpha, blendMode, userValue,
        red, green, blue, clip, dib);
    const int result = RKC_UPDIB_VS_SetPacket(screen, 0, packet) != nullptr;
    RKC_UPDIB_VSPACKET_destructor(packet);
    return result;
}

extern "C" int __thiscall RKC_UPDIB_SetStringsPacket(
    void* self,
    long vsBlockIndex,
    long vsIndex,
    long updIndex,
    long x,
    long y,
    char* text,
    unsigned char red,
    unsigned char green,
    unsigned char blue,
    long flags,
    long limit,
    long letterSpacing,
    long lineSpacing,
    long userValue
) {
    OSF_FUNC_TRACE("self=%p, vsBlockIndex=%ld, vsIndex=%ld, updIndex=%ld, x=%ld, y=%ld, text=%s, red=%u, green=%u, blue=%u, flags=%ld, limit=%ld, letterSpacing=%ld, lineSpacing=%ld, userValue=%ld",
        self, vsBlockIndex, vsIndex, updIndex, x, y, text ? text : "(null)", red, green, blue, flags, limit, letterSpacing, lineSpacing, userValue);
    void* block = RKC_UPDIB_GetVSBlock(self, vsBlockIndex);
    void* screen = block
        ? RKC_UPDIB_VSBLOCK_GetVScreen(block, vsIndex) : nullptr;
    void* upd = RKC_UPDIB_GetUpd(self, updIndex);
    char* basePattern = upd
        ? static_cast<char*>(RKC_UPDIB_UPD_GetPattern(upd, 0)) : nullptr;
    if (!screen || !basePattern || !text)
        return 0;

    const long cellWidth = *(long*)(basePattern + 0x14) / 16;
    const long cellHeight = *(long*)(basePattern + 0x18) / 16;
    if (limit == 0)
        limit = 2000000;
    long cursorX = x;
    long logicalCount = 1;
    long byteCount = 2;

    for (const unsigned char* cursor =
             reinterpret_cast<const unsigned char*>(text);
         *cursor;
         ++cursor) {
        const unsigned char first = *cursor;
        const bool shiftJisLead =
            (first >= 0x80 && first <= 0x9f) || first >= 0xe0;
        if (!shiftJisLead) {
            if (logicalCount > limit)
                return 1;
            if (first == '\n') {
                y += cellHeight + lineSpacing;
                cursorX = x;
            } else if (first == ' ') {
                cursorX += cellWidth + letterSpacing;
            } else {
                char* pattern = static_cast<char*>(
                    RKC_UPDIB_UPD_GetPattern(upd, 0));
                void* packet = pattern
                    ? RKC_UPDIB_VS_InsertVSPacket(screen, 0) : nullptr;
                if (packet) {
                    char* p = static_cast<char*>(packet);
                    *(long*)(p + 0x0c) =
                        cursorX - (first & 0x0f) * cellWidth;
                    *(long*)(p + 0x10) =
                        y - (first >> 4) * cellHeight;
                    *(long*)(p + 0x24) = updIndex;
                    *(long*)(p + 0x28) = 0;
                    *(long*)(p + 0x2c) = *(long*)(pattern + 0x1c);
                    *(unsigned short*)(p + 0x30) = red;
                    *(unsigned short*)(p + 0x32) = green;
                    *(unsigned short*)(p + 0x34) = blue;
                    *(unsigned long*)(p + 0x04) =
                        static_cast<unsigned long>(flags) | 0xa0;
                    *(long*)(p + 0x1c) = userValue;
                    *(long*)(p + 0x40) = cursorX;
                    *(long*)(p + 0x44) = y;
                    *(long*)(p + 0x48) = cursorX + cellWidth - 1;
                    *(long*)(p + 0x4c) = y + cellHeight - 1;
                }
                cursorX += cellWidth + letterSpacing;
            }
            ++logicalCount;
            ++byteCount;
            continue;
        }

        if (byteCount > limit || cursor[1] == 0)
            return 1;
        const long patternIndex =
            first >= 0xe0 ? first - 0xbf : first - 0x7f;
        const unsigned char second = cursor[1];
        char* pattern = static_cast<char*>(
            RKC_UPDIB_UPD_GetPattern(upd, patternIndex));
        void* packet = pattern
            ? RKC_UPDIB_VS_InsertVSPacket(screen, 0) : nullptr;
        if (packet) {
            char* p = static_cast<char*>(packet);
            *(long*)(p + 0x0c) =
                cursorX - (second & 0x0f) * cellWidth * 2;
            *(long*)(p + 0x10) =
                y - (second >> 4) * cellHeight;
            *(long*)(p + 0x24) = updIndex;
            *(long*)(p + 0x28) = patternIndex;
            *(long*)(p + 0x2c) = -1;
            *(unsigned short*)(p + 0x30) = red;
            *(unsigned short*)(p + 0x32) = green;
            *(unsigned short*)(p + 0x34) = blue;
            *(unsigned long*)(p + 0x04) =
                static_cast<unsigned long>(flags) | 0xa0;
            *(long*)(p + 0x1c) = userValue;
            *(long*)(p + 0x40) = cursorX;
            *(long*)(p + 0x44) = y;
            *(long*)(p + 0x48) = cursorX + cellWidth * 2 - 1;
            *(long*)(p + 0x4c) = y + cellHeight - 1;
        }
        cursorX += cellWidth * 2 + letterSpacing;
        ++cursor;
        logicalCount += 2;
        byteCount += 2;
    }
    return 1;
}

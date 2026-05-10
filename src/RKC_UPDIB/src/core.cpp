/**
 * RKC_UPDIB - Universal Pattern DIB handling
 * 
 * This DLL provides sprite/pattern rendering functions for ShadowFlare.
 * It manages UPD files (sprite sheets with multiple patterns/animations).
 * 
 * Multiple classes with different layouts are defined here.
 */

#include <windows.h>
#include <cstring>

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
extern "C" void __thiscall RKC_UPDIB_CreateTemporaryDIB(void* self);

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
    CallFunctionInDLL<void>("RKC_UPDIB.dll", "?Release@RKC_UPDIB_UPD@@QAEXXZ", upd);

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
// STUBS FOR UNUSED FUNCTIONS - NOT IMPORTED BY EXE OR OTHER DLLS
// ============================================================================

// RKC_UPDIB_PATTERN stubs
extern "C" void __thiscall RKC_UPDIB_PATTERN_destructor(void* self) {}
extern "C" void* __thiscall RKC_UPDIB_PATTERN_operatorAssign(void* self, const void* src) { return self; }
extern "C" void __thiscall RKC_UPDIB_PATTERN_Release(void* self) {}

// RKC_UPDIB_UPD stubs
extern "C" void* __thiscall RKC_UPDIB_UPD_constructor(void* self) { return self; }
extern "C" void __thiscall RKC_UPDIB_UPD_destructor(void* self) {}
extern "C" void* __thiscall RKC_UPDIB_UPD_operatorAssign(void* self, const void* src) { return self; }
extern "C" RKC_DIB* __thiscall RKC_UPDIB_UPD_GetPaletteDIB(void* self, long index) { return nullptr; }
extern "C" void* __thiscall RKC_UPDIB_UPD_GetPalette(void* self, long index) { return nullptr; }
extern "C" int __thiscall RKC_UPDIB_UPD_Read(void* self, char* filename, long flags) { return 0; }

// RKC_UPDIB stubs
extern "C" void* __thiscall RKC_UPDIB_operatorAssign(void* self, const void* src) { return self; }
extern "C" int __thiscall RKC_UPDIB_CreateUpdBlock(void* self, long count) { return 0; }
extern "C" int __thiscall RKC_UPDIB_DeleteVSBlock(void* self, long index) { return 0; }
extern "C" int __thiscall RKC_UPDIB_ExchangeUpd(void* self, long a, long b) { return 0; }
extern "C" long __thiscall RKC_UPDIB_GetVSBlockCount(void* self) { return 0; }
extern "C" void* __thiscall RKC_UPDIB_InsertVSBlock(void* self, long index) { return nullptr; }
extern "C" void __thiscall RKC_UPDIB_Release(void* self) {}

// RKC_UPDIB_VS stubs
extern "C" void __thiscall RKC_UPDIB_VS_destructor(void* self) {}
extern "C" void* __thiscall RKC_UPDIB_VS_operatorAssign(void* self, const void* src) { return self; }
extern "C" int __thiscall RKC_UPDIB_VS_DeleteVSPacket(void* self, long index) { return 0; }
extern "C" void __thiscall RKC_UPDIB_VS_FlushVSPacket(void* self) {}
extern "C" void* __thiscall RKC_UPDIB_VS_GetVSPacket(void* self, long index) { return nullptr; }
extern "C" void* __thiscall RKC_UPDIB_VS_InsertVSPacket(void* self, long index) { return nullptr; }
extern "C" void __thiscall RKC_UPDIB_VS_Release(void* self) {}
extern "C" int __thiscall RKC_UPDIB_VS_Render(void* self, RKC_DIB* dib, long x, long y, RECT* clip) { return 0; }
extern "C" void* __thiscall RKC_UPDIB_VS_SetPacket(void* self, long index, void* packet) { return nullptr; }

// RKC_UPDIB_VSBLOCK stubs
extern "C" void __thiscall RKC_UPDIB_VSBLOCK_destructor(void* self) {}
extern "C" void* __thiscall RKC_UPDIB_VSBLOCK_operatorAssign(void* self, const void* src) { return self; }
extern "C" int __thiscall RKC_UPDIB_VSBLOCK_CreateVS(void* self, long count) { return 0; }
extern "C" void __thiscall RKC_UPDIB_VSBLOCK_FlushVScreen(void* self) {}
extern "C" long __thiscall RKC_UPDIB_VSBLOCK_GetVSCount(void* self) { return 0; }
extern "C" void __thiscall RKC_UPDIB_VSBLOCK_Release(void* self) {}

// RKC_UPDIB_VSPACKET stubs
extern "C" void* __thiscall RKC_UPDIB_VSPACKET_operatorAssign(void* self, const void* src) { return self; }
extern "C" int __thiscall RKC_UPDIB_VSPACKET_Render(void* self, RKC_DIB* dib, RECT* clip) { return 0; }
extern "C" int __thiscall RKC_UPDIB_VSPACKET_RenderBox(void* self, RKC_DIB* dib, RECT* clip) { return 0; }
extern "C" int __thiscall RKC_UPDIB_VSPACKET_RenderFill(void* self, RKC_DIB* dib, RECT* clip) { return 0; }
extern "C" int __thiscall RKC_UPDIB_VSPACKET_RenderLine(void* self, RKC_DIB* dib, RECT* clip) { return 0; }
extern "C" int __thiscall RKC_UPDIB_VSPACKET_RenderPoint(void* self, RKC_DIB* dib, RECT* clip) { return 0; }
extern "C" void* __thiscall RKC_UPDIB_VSPACKET_SetPacket_copy(void* self, void* packet) { return nullptr; }
extern "C" void* __thiscall RKC_UPDIB_VSPACKET_SetPacket_full(void* self, void* updib, long a, long b, long c, long d, long e, long f, long g, long h, long i, long j, short k, short l, short m, RECT* clip, RKC_DIB* dib) { return nullptr; }

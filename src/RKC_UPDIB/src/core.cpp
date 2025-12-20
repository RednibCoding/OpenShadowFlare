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

// Forward declarations
class RKC_DIB;
class RKC_DIBHISPEEDMODE;
struct RKC_UPDIB_JUDGE;
struct RKC_UPDIB_PARTSLIST;
struct RKC_UPDIB_UPD_PARTS;

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
 * RKC_UPDIB_UPD::GetPatternCount - Get number of patterns
 * USED BY: ShadowFlare.exe, o_RKC_UPDIB.dll (internal)
 */
extern "C" long __thiscall RKC_UPDIB_UPD_GetPatternCount(void* self) {
    return *(long*)((char*)self + 0x14);
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

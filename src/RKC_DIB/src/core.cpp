/**
 * RKC_DIB - Device Independent Bitmap handling
 * 
 * This DLL provides bitmap/image manipulation functions for ShadowFlare.
 * 
 * Class layout (RKC_DIB):
 *   +0x00: BITMAPINFOHEADER* bitmapInfo  - Pointer to bitmap info header
 *   +0x04: RGBQUAD* palette              - Pointer to color palette
 *   +0x08: unsigned char* bitmap         - Pointer to pixel data
 */

#include <windows.h>
#include <cstring>

/**
 * RKC_DIB class structure - 12 bytes
 */
class RKC_DIB {
public:
    BITMAPINFOHEADER* bitmapInfo;  // +0x00
    RGBQUAD* palette;              // +0x04
    unsigned char* bitmap;         // +0x08
};

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

/**
 * RKC_DIB::constructor - Initialize DIB object
 * USED BY: ShadowFlare.exe, o_RKC_FONTMAKER.dll, o_RKC_DBFCONTROL.dll, o_RKC_UPDIB.dll
 */
extern "C" void __thiscall RKC_DIB_constructor(RKC_DIB* self) {
    self->bitmapInfo = nullptr;
    self->palette = nullptr;
    self->bitmap = nullptr;
}

/**
 * RKC_DIB::Release - Free allocated memory and reset pointers
 * USED BY: o_RKC_DBFCONTROL.dll, o_RKC_UPDIB.dll
 */
extern "C" void __thiscall RKC_DIB_Release(RKC_DIB* self) {
    if (self->bitmapInfo) {
        GlobalFree(self->bitmapInfo);
    }
    if (self->bitmap) {
        GlobalFree(self->bitmap);
    }
    self->bitmapInfo = nullptr;
    self->palette = nullptr;
    self->bitmap = nullptr;
}

/**
 * RKC_DIB::~destructor - Destructor, just calls Release
 * USED BY: ShadowFlare.exe, o_RKC_FONTMAKER.dll, o_RKC_DBFCONTROL.dll, o_RKC_UPDIB.dll
 */
extern "C" void __thiscall RKC_DIB_destructor(RKC_DIB* self) {
    RKC_DIB_Release(self);
}

/**
 * RKC_DIB::operator= - Assignment operator (shallow copy of pointers)
 * NOT REFERENCED - stub only, not imported by any module
 * 
 * Copies all 3 pointers from source to this DIB.
 * WARNING: This is a shallow copy - both DIBs will point to the same memory!
 * Returns reference to this DIB.
 */
extern "C" RKC_DIB* __thiscall RKC_DIB_operatorAssign(RKC_DIB* self, const RKC_DIB* source) {
    self->bitmapInfo = source->bitmapInfo;
    self->palette = source->palette;
    self->bitmap = source->bitmap;
    return self;
}

// ============================================================================
// GETTERS
// ============================================================================

/**
 * RKC_DIB::GetBitmapInfo - Get pointer to bitmap info header
 * NOT REFERENCED - stub only, not imported by any module
 */
extern "C" BITMAPINFOHEADER* __thiscall RKC_DIB_GetBitmapInfo(RKC_DIB* self) {
    return self->bitmapInfo;
}

/**
 * RKC_DIB::GetPalette - Get pointer to color palette
 * NOT REFERENCED - stub only, not imported by any module
 */
extern "C" RGBQUAD* __thiscall RKC_DIB_GetPalette(RKC_DIB* self) {
    return self->palette;
}

/**
 * RKC_DIB::GetPaletteCount - Get number of palette entries
 * NOT REFERENCED - stub only, not imported by any module (but used internally)
 * 
 * Returns palette count based on BPP:
 * - BPP 1: biClrUsed or 2
 * - BPP 4: biClrUsed or 16
 * - BPP 8: biClrUsed or 256
 * - BPP 16/24/32: 0 (no palette)
 * - Invalid: -1
 */
extern "C" long __thiscall RKC_DIB_GetPaletteCount(RKC_DIB* self) {
    if (!self->bitmapInfo) {
        return -1;
    }
    
    WORD bpp = self->bitmapInfo->biBitCount;
    DWORD clrUsed = self->bitmapInfo->biClrUsed;
    
    switch (bpp) {
        case 1:
            return (clrUsed != 0) ? clrUsed : 2;
        case 4:
            return (clrUsed != 0) ? clrUsed : 16;
        case 8:
            return (clrUsed != 0) ? clrUsed : 256;
        case 16:
        case 24:
        case 32:
            return 0;
        default:
            return -1;
    }
}

/**
 * RKC_DIB::GetBitmap - Get pointer to pixel data
 * NOT REFERENCED - stub only, not imported by any module
 */
extern "C" unsigned char* __thiscall RKC_DIB_GetBitmap(RKC_DIB* self) {
    return self->bitmap;
}

/**
 * Helper function to align to 4-byte boundary (DWORD alignment)
 */
static inline long alignTo4(long value) {
    return (value + 3) & ~3;
}

/**
 * RKC_DIB::GetAlignWidth - Get row stride in bytes (aligned to 4 bytes)
 * USED BY: o_RKC_FONTMAKER.dll
 * 
 * Calculates the aligned row width in bytes based on BPP:
 * - BPP 1: (width + 7) / 8, aligned to 4
 * - BPP 4: (width + 1) / 2, aligned to 4
 * - BPP 8: width, aligned to 4
 * - BPP 16: width * 2, aligned to 4
 * - BPP 24: width * 3, aligned to 4
 * - Others: returns -1
 */
extern "C" long __thiscall RKC_DIB_GetAlignWidth(RKC_DIB* self) {
    if (!self->bitmapInfo) {
        return -1;
    }
    
    WORD bpp = self->bitmapInfo->biBitCount;
    LONG width = self->bitmapInfo->biWidth;
    
    switch (bpp) {
        case 1:
            // (width + 7) / 8 bytes, aligned to 4
            return alignTo4((width + 7) / 8);
        case 4:
            // (width + 1) / 2 bytes, aligned to 4
            return alignTo4((width + 1) / 2);
        case 8:
            // width bytes, aligned to 4
            return alignTo4(width);
        case 16:
            // width * 2 bytes, aligned to 4
            return alignTo4(width * 2);
        case 24:
            // width * 3 bytes, aligned to 4
            return alignTo4(width * 3);
        default:
            return -1;
    }
}

/**
 * RKC_DIB::FillByte - Fill entire bitmap with a byte value
 * USED BY: ShadowFlare.exe, o_RKC_DBFCONTROL.dll, o_RKC_UPDIB.dll
 * 
 * Fills the entire bitmap buffer with the specified byte value.
 * Uses optimized 4-byte writes followed by remaining single bytes.
 * Returns: 1 on success, 0 if no bitmap
 */
extern "C" int __thiscall RKC_DIB_FillByte(RKC_DIB* self, unsigned char fillValue) {
    if (!self->bitmap) {
        return 0;
    }
    
    // Get aligned width (stride) and calculate total size
    long stride = RKC_DIB_GetAlignWidth(self);
    if (stride <= 0 || !self->bitmapInfo) {
        return 0;
    }
    
    long totalBytes = stride * self->bitmapInfo->biHeight;
    
    // Build 4-byte fill pattern (same byte repeated)
    DWORD fillPattern = fillValue | (fillValue << 8) | (fillValue << 16) | (fillValue << 24);
    
    // Fill using DWORD writes for speed
    DWORD* dst32 = (DWORD*)self->bitmap;
    long dwordCount = totalBytes / 4;
    for (long i = 0; i < dwordCount; i++) {
        dst32[i] = fillPattern;
    }
    
    // Fill remaining bytes
    unsigned char* dst8 = self->bitmap + (dwordCount * 4);
    long remaining = totalBytes & 3;
    for (long i = 0; i < remaining; i++) {
        dst8[i] = fillValue;
    }
    
    return 1;
}

/**
 * RKC_DIB::CopyPalette - Copy palette from another DIB
 * NOT REFERENCED - stub only, not imported by any module (but used internally by Copy)
 * 
 * Copies palette entries from source DIB to this DIB.
 * Both DIBs must have the same palette count.
 * Returns: 1 on success, 0 on failure (mismatched counts)
 */
extern "C" int __thiscall RKC_DIB_CopyPalette(RKC_DIB* self, RKC_DIB* source) {
    // Get source palette count
    long srcCount = RKC_DIB_GetPaletteCount(source);
    if (srcCount <= 0) {
        return 0;  // No source palette
    }
    
    // Get destination palette count
    long dstCount = RKC_DIB_GetPaletteCount(self);
    if (dstCount <= 0) {
        return 0;  // No destination palette
    }
    
    // Counts must match
    if (srcCount != dstCount) {
        return 0;
    }
    
    // Copy palette entries (each RGBQUAD is 4 bytes)
    memcpy(self->palette, source->palette, srcCount * sizeof(RGBQUAD));
    
    return 1;
}

/**
 * RKC_DIB::SetPalette - Set palette from RGBQUAD array
 * USED BY: o_RKC_UPDIB.dll
 * 
 * Copies palette entries from input array to this DIB's palette.
 * Number of entries copied is determined by GetPaletteCount.
 * Returns: 1 on success, 0 if no palette (no return code in original if count is 0)
 */
extern "C" int __thiscall RKC_DIB_SetPalette(RKC_DIB* self, RGBQUAD* sourcePalette) {
    long count = RKC_DIB_GetPaletteCount(self);
    if (count <= 0) {
        return 0;  // No palette
    }
    
    // Copy palette entries
    memcpy(self->palette, sourcePalette, count * sizeof(RGBQUAD));
    
    return 1;
}

// ============================================================================
// SETTERS
// ============================================================================

/**
 * RKC_DIB::SetBitmap - Set pixel data pointer, returns old pointer
 * USED BY: o_RKC_UPDIB.dll
 */
extern "C" unsigned char* __thiscall RKC_DIB_SetBitmap(RKC_DIB* self, unsigned char* newBitmap) {
    unsigned char* oldBitmap = self->bitmap;
    self->bitmap = newBitmap;
    return oldBitmap;
}

/**
 * RKC_DIB::GetRect - Get bitmap dimensions as a RECT
 * NOT REFERENCED - stub only, not imported by any module
 * 
 * Sets rect->left = 0, rect->top = 0
 * Sets rect->right = biWidth, rect->bottom = biHeight (from bitmapInfo)
 * Returns the output rect pointer
 */
extern "C" RECT* __thiscall RKC_DIB_GetRect(RKC_DIB* self, RECT* outRect) {
    outRect->left = 0;
    outRect->top = 0;
    
    if (self->bitmapInfo) {
        outRect->right = self->bitmapInfo->biWidth;
        outRect->bottom = self->bitmapInfo->biHeight;
    } else {
        outRect->right = 0;
        outRect->bottom = 0;
    }
    
    return outRect;
}

// ============================================================================
// DLL ENTRY POINT
// ============================================================================

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

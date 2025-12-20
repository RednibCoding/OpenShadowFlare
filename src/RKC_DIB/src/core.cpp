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

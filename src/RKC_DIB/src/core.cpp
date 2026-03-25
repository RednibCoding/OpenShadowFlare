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
#include <cstdio>
#include "../../utils.h"

/**
 * RKC_DIB class structure - 12 bytes
 */
class RKC_DIB {
public:
    BITMAPINFOHEADER* bitmapInfo;  // +0x00
    RGBQUAD* palette;              // +0x04
    unsigned char* bitmap;         // +0x08
};

/**
 * RKC_DIBHISPEEDMODE class - 289,984 bytes (0x46C00 = 0x468C0 * 4)
 * Contains pre-calculated lookup tables for fast blending operations.
 * The constructor builds complex tables, but destructor is empty.
 */
#define DIBHISPEEDMODE_SIZE 0x46C00

// Forward declarations
extern "C" long __thiscall RKC_DIB_GetAlignWidth(RKC_DIB* self);
extern "C" void __thiscall RKC_DIB_Release(RKC_DIB* self);

// ============================================================================
// RKC_DIBHISPEEDMODE FUNCTIONS
// ============================================================================

/**
 * RKC_DIBHISPEEDMODE::~destructor - Empty destructor
 * USED BY: o_RKC_UPDIB.dll
 * 
 * The lookup tables are embedded in the object (not heap allocated),
 * so there's nothing to free.
 */
extern "C" void __thiscall RKC_DIBHISPEEDMODE_destructor(void* self) {
    // Empty - no cleanup needed
}

/**
 * RKC_DIBHISPEEDMODE::operator= - Copy lookup tables
 * NOT REFERENCED - stub only, not imported by any module
 * 
 * Copies 0x468C0 DWORDs (289,984 bytes) from source to this.
 */
extern "C" void* __thiscall RKC_DIBHISPEEDMODE_operatorAssign(void* self, const void* source) {
    memcpy(self, source, 0x468C0 * sizeof(DWORD));
    return self;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

/**
 * RKC_DIB::constructor - Initialize DIB object
 * USED BY: ShadowFlare.exe, o_RKC_FONTMAKER.dll, o_RKC_DBFCONTROL.dll, o_RKC_UPDIB.dll
 */
extern "C" RKC_DIB* __thiscall RKC_DIB_constructor(RKC_DIB* self) {
    self->bitmapInfo = nullptr;
    self->palette = nullptr;
    self->bitmap = nullptr;
    return self;
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
 * RKC_DIB::Create - Allocate and initialize a new DIB
 * USED BY: ShadowFlare.exe, o_RKC_FONTMAKER.dll, o_RKC_DBFCONTROL.dll, o_RKC_UPDIB.dll
 * 
 * Parameters:
 *   width  - bitmap width in pixels
 *   height - bitmap height in pixels
 *   bpp    - bits per pixel (1, 4, 8, 16, 24)
 *   allocBitmap - if 1, allocate pixel buffer; otherwise just header+palette
 * 
 * Allocates BITMAPINFOHEADER + palette (for paletted modes).
 * If allocBitmap is 1, also allocates the pixel data buffer.
 * Returns: 1 on success, 0 on failure
 */
extern "C" int __thiscall RKC_DIB_Create(RKC_DIB* self, long width, long height, long bpp, int allocBitmap) {
    // Release any existing data first
    RKC_DIB_Release(self);
    
    // Validate: must have valid dimensions unless allocBitmap != 1
    if ((width == 0 || height == 0) && allocBitmap == 1) {
        return 0;
    }
    
    // Calculate palette count based on BPP
    int paletteCount;
    switch (bpp) {
        case 1:
            paletteCount = 2;
            break;
        case 4:
            paletteCount = 16;
            break;
        case 8:
            paletteCount = 256;
            break;
        case 16:
        case 24:
            paletteCount = 0;
            break;
        default:
            return 0;  // Invalid BPP
    }
    
    // Allocate BITMAPINFOHEADER + palette
    // Header is 0x28 (40) bytes, each palette entry is 4 bytes (RGBQUAD)
    SIZE_T headerSize = 0x28 + (paletteCount * 4);
    BITMAPINFOHEADER* pHeader = (BITMAPINFOHEADER*)GlobalAlloc(GPTR, headerSize);
    if (!pHeader) {
        return 0;
    }
    
    self->bitmapInfo = pHeader;
    
    // Set palette pointer (right after header, or NULL if no palette)
    if (paletteCount == 0) {
        self->palette = nullptr;
    } else {
        self->palette = (RGBQUAD*)((char*)pHeader + 0x28);
    }
    
    // Initialize header fields
    pHeader->biSize = 0x28;           // Size of BITMAPINFOHEADER
    pHeader->biWidth = width;
    pHeader->biHeight = height;
    pHeader->biPlanes = 1;
    pHeader->biBitCount = (WORD)bpp;
    pHeader->biCompression = 0;       // BI_RGB
    pHeader->biXPelsPerMeter = 0;
    pHeader->biYPelsPerMeter = 0;
    pHeader->biClrUsed = 0;
    pHeader->biClrImportant = 0;
    
    // Calculate aligned row width and image size
    long alignWidth = RKC_DIB_GetAlignWidth(self);
    if (alignWidth == -1) {
        RKC_DIB_Release(self);
        return 0;
    }
    
    pHeader->biSizeImage = alignWidth * height;
    
    // Allocate pixel buffer if requested
    if (allocBitmap != 1) {
        return 1;  // Header only, no pixel buffer
    }
    
    self->bitmap = (unsigned char*)GlobalAlloc(GMEM_FIXED, pHeader->biSizeImage);
    if (!self->bitmap) {
        RKC_DIB_Release(self);
        return 0;
    }
    
    return 1;
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

// Forward declaration
extern "C" int __thiscall RKC_DIB_FillByte(RKC_DIB* self, unsigned char fillValue);

/**
 * RKC_DIB::Fill - Fill entire bitmap with a color value
 * USED BY: o_RKC_DBFCONTROL.dll
 * 
 * Fills based on bit depth:
 * - 1 bpp: 0xFF if color non-zero, else 0x00
 * - 4 bpp: low nibble duplicated to both nibbles
 * - 8 bpp: low byte directly
 * - 16 bpp: 2-byte color per pixel
 * - 24 bpp: 3-byte BGR color per pixel
 * 
 * Returns: 1 on success, 0 if no bitmap
 */
extern "C" int __thiscall RKC_DIB_Fill(RKC_DIB* self, long color) {
    if (!self->bitmap || !self->bitmapInfo) {
        return 0;
    }
    
    WORD bpp = self->bitmapInfo->biBitCount;
    
    switch (bpp) {
        case 1: {
            // Fill with 0xFF if color non-zero, else 0x00
            unsigned char fillVal = (color != 0) ? 0xFF : 0x00;
            RKC_DIB_FillByte(self, fillVal);
            return 1;
        }
        case 4: {
            // Duplicate low nibble to both nibbles
            unsigned char nibble = (unsigned char)(color & 0x0F);
            unsigned char fillVal = nibble | (nibble << 4);
            RKC_DIB_FillByte(self, fillVal);
            return 1;
        }
        case 8: {
            // Use low byte directly
            RKC_DIB_FillByte(self, (unsigned char)(color & 0xFF));
            return 1;
        }
        case 16: {
            // 16bpp: write 2-byte color per pixel
            long stride = RKC_DIB_GetAlignWidth(self);
            if (stride <= 0) return 0;
            
            unsigned char* dst = self->bitmap;
            long width = self->bitmapInfo->biWidth;
            long height = self->bitmapInfo->biHeight;
            unsigned char b0 = (unsigned char)(color & 0xFF);
            unsigned char b1 = (unsigned char)((color >> 8) & 0xFF);
            
            for (long y = 0; y < height; y++) {
                unsigned char* row = dst;
                for (long x = 0; x < width; x++) {
                    *row++ = b0;
                    *row++ = b1;
                }
                dst += stride;
            }
            return 1;
        }
        case 24: {
            // 24bpp: write 3-byte BGR color per pixel
            long stride = RKC_DIB_GetAlignWidth(self);
            if (stride <= 0) return 0;
            
            unsigned char* dst = self->bitmap;
            long width = self->bitmapInfo->biWidth;
            long height = self->bitmapInfo->biHeight;
            unsigned char b = (unsigned char)(color & 0xFF);         // Blue
            unsigned char g = (unsigned char)((color >> 8) & 0xFF);  // Green
            unsigned char r = (unsigned char)((color >> 16) & 0xFF); // Red
            
            for (long y = 0; y < height; y++) {
                unsigned char* row = dst;
                for (long x = 0; x < width; x++) {
                    *row++ = b;
                    *row++ = g;
                    *row++ = r;
                }
                dst += stride;
            }
            return 1;
        }
        default:
            return 0;
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
// TRANSFER FUNCTIONS - USED BY EXE AND OTHER DLLS
// ============================================================================

/**
 * RKC_DIB::TransferToDDB - Transfer DIB to device context (3-arg version)
 * USED BY: ShadowFlare.exe, o_RKC_DBFCONTROL.dll
 * 
 * Copies the DIB pixels to a device context using SetDIBitsToDevice.
 * Parameters:
 *   hdc - target device context
 *   x   - x position in DC
 *   y   - y position in DC
 * 
 * Returns: 1 on success, 0 if no bitmap
 */
extern "C" int __thiscall RKC_DIB_TransferToDDB(RKC_DIB* self, HDC hdc, long x, long y) {
    if (!self->bitmap || !self->bitmapInfo) {
        return 0;
    }
    
    DWORD width = self->bitmapInfo->biWidth;
    DWORD height = self->bitmapInfo->biHeight;
    
    SetDIBitsToDevice(
        hdc,
        x, y,                    // Destination x, y
        width, height,           // Width, height
        0, 0,                    // Source x, y
        0, height,               // Start scan, num scans
        self->bitmap,            // Pixel data
        (BITMAPINFO*)self->bitmapInfo,  // Bitmap info (includes palette)
        DIB_RGB_COLORS           // Color usage
    );
    
    return 1;
}

/**
 * RKC_DIB::ReadFile - Load a BMP file into this DIB
 * USED BY: ShadowFlare.exe
 * 
 * Reads a Windows BMP file and loads it into this DIB object.
 * Parameters:
 *   filename - path to BMP file
 *   flags    - bit flags controlling which BPPs to accept
 *              bit 0 (0x01): accept 1bpp
 *              bit 1 (0x02): accept 4bpp
 *              bit 2 (0x04): accept 8bpp
 *              bit 3 (0x08): accept 16bpp
 *              bit 4 (0x10): accept 24bpp
 * 
 * Returns: 1 on success, 0 on failure
 */
extern "C" int __thiscall RKC_DIB_ReadFile(RKC_DIB* self, const char* filename, short flags) {
    // Release existing data
    RKC_DIB_Release(self);
    
    // Open the file using RKC_FILE via CallFunctionInDLL
    // Actually, let's use Windows API directly since it's simpler
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, 
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    DWORD bytesRead;
    
    // Read BITMAPFILEHEADER (14 bytes)
    // struct { WORD bfType; DWORD bfSize; WORD bfReserved1, bfReserved2; DWORD bfOffBits; }
    unsigned char fileHeader[14];
    if (!ReadFile(hFile, fileHeader, 14, &bytesRead, NULL) || bytesRead != 14) {
        CloseHandle(hFile);
        return 0;
    }
    
    // Check BMP signature "BM"
    if (fileHeader[0] != 'B' || fileHeader[1] != 'M') {
        CloseHandle(hFile);
        return 0;
    }
    
    // Get pixel data offset from file header (at offset 10, 4 bytes LE)
    DWORD pixelDataOffset = *(DWORD*)(fileHeader + 10);
    
    // Read BITMAPINFOHEADER (40 bytes)
    BITMAPINFOHEADER infoHeader;
    if (!ReadFile(hFile, &infoHeader, sizeof(BITMAPINFOHEADER), &bytesRead, NULL) || 
        bytesRead != sizeof(BITMAPINFOHEADER)) {
        CloseHandle(hFile);
        return 0;
    }
    
    // Check BPP against flags
    WORD bpp = infoHeader.biBitCount;
    bool accepted = false;
    int paletteCount = 0;
    
    switch (bpp) {
        case 1:
            accepted = (flags & 0x01) != 0;
            paletteCount = 2;
            break;
        case 4:
            accepted = (flags & 0x02) != 0;
            paletteCount = 16;
            break;
        case 8:
            accepted = (flags & 0x04) != 0;
            paletteCount = 256;
            break;
        case 16:
            accepted = (flags & 0x08) != 0;
            paletteCount = 0;
            break;
        case 24:
            accepted = (flags & 0x10) != 0;
            paletteCount = 0;
            break;
        default:
            accepted = false;
            break;
    }
    
    if (!accepted) {
        CloseHandle(hFile);
        return 0;
    }
    
    // Allocate header + palette
    SIZE_T headerSize = 0x28 + (paletteCount * 4);
    BITMAPINFOHEADER* pHeader = (BITMAPINFOHEADER*)GlobalAlloc(GPTR, headerSize);
    if (!pHeader) {
        CloseHandle(hFile);
        return 0;
    }
    
    // Copy header
    memcpy(pHeader, &infoHeader, sizeof(BITMAPINFOHEADER));
    pHeader->biClrImportant = 0;  // Original clears this
    
    self->bitmapInfo = pHeader;
    
    // Set palette pointer
    if (paletteCount > 0) {
        self->palette = (RGBQUAD*)((char*)pHeader + 0x28);
        
        // Read palette
        if (!ReadFile(hFile, self->palette, paletteCount * 4, &bytesRead, NULL) ||
            bytesRead != (DWORD)(paletteCount * 4)) {
            RKC_DIB_Release(self);
            CloseHandle(hFile);
            return 0;
        }
    } else {
        self->palette = nullptr;
    }
    
    // Calculate image size
    long alignWidth = RKC_DIB_GetAlignWidth(self);
    if (alignWidth == -1) {
        RKC_DIB_Release(self);
        CloseHandle(hFile);
        return 0;
    }
    
    SIZE_T imageSize = alignWidth * infoHeader.biHeight;
    
    // Seek to pixel data
    SetFilePointer(hFile, pixelDataOffset, NULL, FILE_BEGIN);
    
    // Allocate pixel buffer
    self->bitmap = (unsigned char*)GlobalAlloc(GMEM_FIXED, imageSize);
    if (!self->bitmap) {
        RKC_DIB_Release(self);
        CloseHandle(hFile);
        return 0;
    }
    
    // Read pixel data
    if (!ReadFile(hFile, self->bitmap, (DWORD)imageSize, &bytesRead, NULL) ||
        bytesRead != (DWORD)imageSize) {
        RKC_DIB_Release(self);
        CloseHandle(hFile);
        return 0;
    }
    
    CloseHandle(hFile);
    return 1;
}

// ============================================================================
// TRANSFER FUNCTIONS - BLIT BETWEEN DIBS
// ============================================================================

/**
 * RKC_DIB::TransferToDIBFast - Fast blit from source DIB to this DIB
 * USED BY: ShadowFlare.exe, o_RKC_UPDIB.dll, o_RKC_RPGSCRN.dll
 * 
 * 7-arg version with full coordinates:
 *   destX, destY - destination position in this DIB
 *   width, height - area to copy
 *   srcDIB       - source DIB
 *   srcX, srcY   - source position in srcDIB
 * 
 * Supports 8->8, 8->16, 8->24, 24->24 bit transfers.
 * Returns: 1 on success, 0 on failure
 */
extern "C" int __thiscall RKC_DIB_TransferToDIBFast_7args(
    RKC_DIB* self, long destX, long destY, long width, long height,
    RKC_DIB* srcDIB, long srcX, long srcY)
{
    // Validate bitmaps exist
    if (!srcDIB->bitmap || !self->bitmap) return 0;
    if (!srcDIB->bitmapInfo || !self->bitmapInfo) return 0;
    
    // Get BPP for both
    WORD srcBpp = srcDIB->bitmapInfo->biBitCount;
    WORD destBpp = self->bitmapInfo->biBitCount;
    
    // For 1bpp and 4bpp, we'd need TransferToDIB - not yet implemented
    // For now just return 0 (caller will handle it)
    if (srcBpp == 1 || srcBpp == 4) return 0;
    
    // Only support certain combinations
    if (srcBpp != 8 && srcBpp != 24) return 0;
    if (destBpp != 8 && destBpp != 16 && destBpp != 24) return 0;
    if (destBpp < srcBpp) return 0;  // Can't reduce BPP
    
    // Get dimensions from bitmapInfo
    long destImgW = self->bitmapInfo->biWidth;
    long destImgH = self->bitmapInfo->biHeight;
    long srcImgW = srcDIB->bitmapInfo->biWidth;
    long srcImgH = srcDIB->bitmapInfo->biHeight;
    
    // Clipping - adjust for negative coordinates
    if (destX < 0) { srcX -= destX; width += destX; destX = 0; }
    if (destY < 0) { srcY -= destY; height += destY; destY = 0; }
    if (srcX < 0) { destX -= srcX; width += srcX; srcX = 0; }
    if (srcY < 0) { destY -= srcY; height += srcY; srcY = 0; }
    
    // Bounds checking
    if (destX < 0 || destX >= destImgW) return 0;
    if (srcX < 0 || srcX >= srcImgW) return 0;
    
    // Clip to image bounds
    if (destX + width > destImgW) width = destImgW - destX;
    if (destY + height > destImgH) height = destImgH - destY;
    if (srcX + width > srcImgW) width = srcImgW - srcX;
    if (srcY + height > srcImgH) height = srcImgH - srcY;
    
    if (width <= 0 || height <= 0) return 0;
    
    // Get strides (bytes per row, DWORD aligned)
    long srcStride = RKC_DIB_GetAlignWidth(srcDIB);
    long destStride = RKC_DIB_GetAlignWidth(self);
    
    // Calculate byte offsets for starting position
    // DIBs are bottom-up: row 0 is at the bottom
    // srcY=0 means top row visually = (height-1) in memory
    long srcOffset = ((srcImgH - srcY - 1) * srcStride) + (srcBpp * srcX / 8);
    long destOffset = ((destImgH - destY - 1) * destStride) + (destBpp * destX / 8);
    
    unsigned char* srcBits = srcDIB->bitmap;
    unsigned char* destBits = self->bitmap;
    RGBQUAD* srcPal = srcDIB->palette;
    
    // Copy rows
    for (long row = 0; row < height; row++) {
        unsigned char* src = srcBits + srcOffset;
        unsigned char* dst = destBits + destOffset;
        
        if (srcBpp == 8 && destBpp == 8) {
            // 8->8: Direct copy
            memcpy(dst, src, width);
        }
        else if (srcBpp == 8 && destBpp == 16 && srcPal) {
            // 8->16: Palette lookup to RGB555
            unsigned short* dst16 = (unsigned short*)dst;
            for (long x = 0; x < width; x++) {
                RGBQUAD& c = srcPal[src[x]];
                // RGB555: RRRRRGGGGBBBB (original uses this format)
                dst16[x] = ((c.rgbRed & 0xF8) << 7) | ((c.rgbGreen & 0xF8) << 2) | (c.rgbBlue >> 3);
            }
        }
        else if (srcBpp == 8 && destBpp == 24 && srcPal) {
            // 8->24: Palette lookup to BGR
            for (long x = 0; x < width; x++) {
                RGBQUAD& c = srcPal[src[x]];
                dst[x*3 + 0] = c.rgbBlue;
                dst[x*3 + 1] = c.rgbGreen;
                dst[x*3 + 2] = c.rgbRed;
            }
        }
        else if (srcBpp == 24 && destBpp == 24) {
            // 24->24: Direct copy
            memcpy(dst, src, width * 3);
        }
        
        // Move to previous row (DIBs are bottom-up, we go upward visually)
        srcOffset -= srcStride;
        destOffset -= destStride;
    }
    
    return 1;
}

/**
 * RKC_DIB::TransferToDIBFast - 4-arg version (uses source DIB dimensions)
 * USED BY: ShadowFlare.exe, o_RKC_UPDIB.dll
 * 
 * Simplified version that copies entire source to dest at (destX, destY)
 * For 1bpp and 4bpp sources, delegates to TransferToDIB
 */
extern "C" int __thiscall RKC_DIB_TransferToDIB_8args(
    RKC_DIB* self, long destX, long destY, long width, long height,
    RKC_DIB* srcDIB, long srcX, long srcY, long transColor);

extern "C" int __thiscall RKC_DIB_TransferToDIBFast_4args(
    RKC_DIB* self, long destX, long destY, RKC_DIB* srcDIB)
{
    if (!srcDIB || !srcDIB->bitmapInfo) return 0;
    
    WORD srcBpp = srcDIB->bitmapInfo->biBitCount;
    long srcW = srcDIB->bitmapInfo->biWidth;
    long srcH = srcDIB->bitmapInfo->biHeight;
    
    // For 1bpp and 4bpp, delegate to TransferToDIB with transColor=-1 (no transparency)
    if (srcBpp == 1 || srcBpp == 4) {
        return RKC_DIB_TransferToDIB_8args(self, destX, destY, srcW, srcH, srcDIB, 0, 0, -1);
    }
    
    return RKC_DIB_TransferToDIBFast_7args(self, destX, destY, srcW, srcH, srcDIB, 0, 0);
}

/**
 * RKC_DIB::TransferToDIB - Blit with transparency color check
 * USED BY: o_RKC_UPDIB.dll, o_RKC_RPGSCRN.dll
 * 
 * 8-arg version:
 *   destX, destY - destination position
 *   width, height - area to copy
 *   srcDIB       - source DIB
 *   srcX, srcY   - source position
 *   transColor   - transparency color index (skip pixels matching this)
 * 
 * Supports 1/4/8/24 bpp sources to 8/16/24 bpp destinations.
 * Returns: 1 on success, 0 on failure
 */
extern "C" int __thiscall RKC_DIB_TransferToDIB_8args(
    RKC_DIB* self, long destX, long destY, long width, long height,
    RKC_DIB* srcDIB, long srcX, long srcY, long transColor)
{
    if (!srcDIB->bitmap || !self->bitmap) return 0;
    if (!srcDIB->bitmapInfo || !self->bitmapInfo) return 0;
    
    WORD srcBpp = srcDIB->bitmapInfo->biBitCount;
    WORD destBpp = self->bitmapInfo->biBitCount;
    
    // Source must be 1/4/8/24, dest must be 8/16/24
    if (srcBpp != 1 && srcBpp != 4 && srcBpp != 8 && srcBpp != 24) return 0;
    if (destBpp != 8 && destBpp != 16 && destBpp != 24) return 0;
    if (destBpp < srcBpp) return 0;  // Can't reduce BPP (except 24->16 not supported)
    
    long destImgW = self->bitmapInfo->biWidth;
    long destImgH = self->bitmapInfo->biHeight;
    long srcImgW = srcDIB->bitmapInfo->biWidth;
    long srcImgH = srcDIB->bitmapInfo->biHeight;
    
    // Clipping - adjust for negative coordinates
    if (destX < 0) { srcX -= destX; width += destX; destX = 0; }
    if (destY < 0) { srcY -= destY; height += destY; destY = 0; }
    if (srcX < 0) { destX -= srcX; width += srcX; srcX = 0; }
    if (srcY < 0) { destY -= srcY; height += srcY; srcY = 0; }
    
    if (destX < 0 || destX >= destImgW) return 0;
    if (srcX < 0 || srcX >= srcImgW) return 0;
    
    if (destX + width > destImgW) width = destImgW - destX;
    if (destY + height > destImgH) height = destImgH - destY;
    if (srcX + width > srcImgW) width = srcImgW - srcX;
    if (srcY + height > srcImgH) height = srcImgH - srcY;
    
    if (width <= 0 || height <= 0) return 0;
    
    long srcStride = RKC_DIB_GetAlignWidth(srcDIB);
    long destStride = RKC_DIB_GetAlignWidth(self);
    
    // Calculate byte offsets - DIBs are bottom-up
    long srcOffset = ((srcImgH - srcY - 1) * srcStride) + (srcBpp * srcX / 8);
    long destOffset = ((destImgH - destY - 1) * destStride) + (destBpp * destX / 8);
    
    unsigned char* srcBits = srcDIB->bitmap;
    unsigned char* destBits = self->bitmap;
    RGBQUAD* srcPal = srcDIB->palette;
    
    for (long row = 0; row < height; row++) {
        unsigned char* src = srcBits + srcOffset;
        unsigned char* dst = destBits + destOffset;
        
        if (srcBpp == 8 && destBpp == 8) {
            // 8->8 with transparency
            for (long x = 0; x < width; x++) {
                if (src[x] != transColor) {
                    dst[x] = src[x];
                }
            }
        }
        else if (srcBpp == 8 && destBpp == 16 && srcPal) {
            // 8->16 with transparency (RGB555)
            unsigned short* dst16 = (unsigned short*)dst;
            for (long x = 0; x < width; x++) {
                unsigned char idx = src[x];
                if (idx != transColor) {
                    RGBQUAD& c = srcPal[idx];
                    dst16[x] = ((c.rgbRed & 0xF8) << 7) | ((c.rgbGreen & 0xF8) << 2) | (c.rgbBlue >> 3);
                }
            }
        }
        else if (srcBpp == 8 && destBpp == 24 && srcPal) {
            // 8->24 with transparency
            for (long x = 0; x < width; x++) {
                unsigned char idx = src[x];
                if (idx != transColor) {
                    RGBQUAD& c = srcPal[idx];
                    dst[x*3 + 0] = c.rgbBlue;
                    dst[x*3 + 1] = c.rgbGreen;
                    dst[x*3 + 2] = c.rgbRed;
                }
            }
        }
        else if (srcBpp == 24 && destBpp == 24) {
            // 24->24 with transparency (transColor is packed BGR)
            for (long x = 0; x < width; x++) {
                // Pack BGR into long for comparison
                unsigned long pixel = src[x*3] | (src[x*3+1] << 8) | (src[x*3+2] << 16);
                if (pixel != (unsigned long)transColor) {
                    dst[x*3 + 0] = src[x*3 + 0];
                    dst[x*3 + 1] = src[x*3 + 1];
                    dst[x*3 + 2] = src[x*3 + 2];
                }
            }
        }
        else if (srcBpp == 1 && destBpp == 24 && srcPal) {
            // 1bpp->24 with transparency
            long bitPos = (srcX * srcBpp) & 7;  // Starting bit position in first byte
            for (long x = 0; x < width; x++) {
                unsigned char idx = (src[(srcX + x) / 8] >> (7 - ((srcX + x) & 7))) & 1;
                if (idx != transColor) {
                    RGBQUAD& c = srcPal[idx];
                    dst[x*3 + 0] = c.rgbBlue;
                    dst[x*3 + 1] = c.rgbGreen;
                    dst[x*3 + 2] = c.rgbRed;
                }
            }
        }
        else if (srcBpp == 4 && destBpp == 24 && srcPal) {
            // 4bpp->24 with transparency
            for (long x = 0; x < width; x++) {
                long srcPixelX = srcX + x;
                unsigned char idx = (src[srcPixelX / 2] >> ((1 - (srcPixelX & 1)) * 4)) & 0x0F;
                if (idx != transColor) {
                    RGBQUAD& c = srcPal[idx];
                    dst[x*3 + 0] = c.rgbBlue;
                    dst[x*3 + 1] = c.rgbGreen;
                    dst[x*3 + 2] = c.rgbRed;
                }
            }
        }
        // Note: 1bpp/4bpp to 8bpp/16bpp not implemented - forward to original if needed
        
        srcOffset -= srcStride;
        destOffset -= destStride;
    }
    
    return 1;
}

/**
 * RKC_DIB::TransferToDIB - 4-arg version
 * USED BY: o_RKC_UPDIB.dll
 * 
 * Copies entire source DIB to destination at (destX, destY) with transparency
 */
extern "C" int __thiscall RKC_DIB_TransferToDIB_4args(
    RKC_DIB* self, long destX, long destY, RKC_DIB* srcDIB, long transColor)
{
    if (!srcDIB || !srcDIB->bitmapInfo) return 0;
    
    long srcW = srcDIB->bitmapInfo->biWidth;
    long srcH = srcDIB->bitmapInfo->biHeight;
    return RKC_DIB_TransferToDIB_8args(self, destX, destY, srcW, srcH, srcDIB, 0, 0, transColor);
}

// ============================================================================
// STUBS FOR UNUSED FUNCTIONS - NOT IMPORTED BY EXE OR OTHER DLLS
// ============================================================================


/**
 * RKC_DIB::AddOffset - Add color offset to palette
 * NOT REFERENCED - stub
 */
extern "C" int __thiscall RKC_DIB_AddOffset(RKC_DIB* self, RGBQUAD offset, int flag) {
    return 0;
}

/**
 * RKC_DIB::ClearUnusedArea - Clear unused bitmap area
 * NOT REFERENCED - stub
 */
extern "C" int __thiscall RKC_DIB_ClearUnusedArea(RKC_DIB* self) {
    return 0;
}

/**
 * RKC_DIB::CompareBitmapColor - Compare pixel color at coordinates
 * NOT REFERENCED - stub
 */
extern "C" int __thiscall RKC_DIB_CompareBitmapColor(RKC_DIB* self, long x, long y, RGBQUAD* color) {
    return 0;
}

/**
 * RKC_DIB::Copy - Deep copy from source DIB
 * NOT REFERENCED - stub
 */
extern "C" int __thiscall RKC_DIB_Copy(RKC_DIB* self, RKC_DIB* source) {
    return 0;
}

/**
 * RKC_DIB::PaintArea - Paint rectangular area with color
 * NOT REFERENCED - stub
 */
extern "C" int __thiscall RKC_DIB_PaintArea(RKC_DIB* self, long x, long y, RGBQUAD* color) {
    return 0;
}

/**
 * RKC_DIB::ScreenPaintLineScan - Scanline paint operation
 * NOT REFERENCED - stub
 */
extern "C" void __thiscall RKC_DIB_ScreenPaintLineScan(RKC_DIB* self, POINT* p1, long* a, POINT* p2, RGBQUAD* c1, RGBQUAD* c2) {
}

/**
 * RKC_DIB::TransferToDDB - Transfer to device context (6-arg version)
 * NOT REFERENCED - stub
 */
extern "C" int __thiscall RKC_DIB_TransferToDDB_6args(RKC_DIB* self, HDC hdc, long x, long y, long w, long h, long flags) {
    return 0;
}

// ============================================================================
// ZOOM FUNCTIONS
// ============================================================================

// Helper: DWORD-aligned stride for a given pixel width and BPP
static inline long dibStride(long width, int bpp) {
    switch (bpp) {
        case 8:  return (width + 3) & ~3;
        case 16: return (width * 2 + 3) & ~3;
        case 24: return (width * 3 + 3) & ~3;
        default: return (width + 3) & ~3;
    }
}

// Helper: pack RGBQUAD to RGB555
static inline unsigned short rgbToRgb555(unsigned char b, unsigned char g, unsigned char r) {
    return ((r & 0xF8) << 7) | ((g & 0xF8) << 2) | (b >> 3);
}

/**
 * RKC_DIB::ZoomToDIB - Scale source DIB region to destination DIB region
 * USED BY: o_RKC_UPDIB.dll
 *
 * Nearest-neighbor scaling from srcRect in srcDIB to destRect in this DIB.
 * The RECT params use {left, top, right, bottom} as {x, y, width, height} --
 * that's how the original code treats them.
 *
 * transColor < 0: copy all pixels (no transparency)
 * transColor >= 0: skip source pixels matching this value
 *
 * Supports: 8->8, 8->16, 8->24, 24->24
 */
extern "C" int __thiscall RKC_DIB_ZoomToDIB(
    RKC_DIB* self, RECT* destRect, RKC_DIB* srcDIB, RECT* srcRect, long transColor)
{
    if (!self->bitmap || !srcDIB->bitmap) return 0;
    if (!self->bitmapInfo || !srcDIB->bitmapInfo) return 0;

    // destRect/srcRect are used as {x, y, width, height}
    long destX = destRect->left;
    long destY = destRect->top;
    long destW = destRect->right;   // width
    long destH = destRect->bottom;  // height
    long srcRX = srcRect->left;
    long srcRY = srcRect->top;
    long srcRW = srcRect->right;    // width
    long srcRH = srcRect->bottom;   // height

    // Compute destination end coords
    long destEndX = destX + destW;
    long destEndY = destY + destH;

    long destImgW = self->bitmapInfo->biWidth;
    long destImgH = self->bitmapInfo->biHeight;

    // Bounds validation
    if (destImgW <= destX) return 0;
    if (destImgH <= destY) return 0;
    if (destEndX < 1) return 0;
    if (destEndY < 1) return 0;

    // Clip to image bounds
    long startX = (destX < 0) ? 0 : destX;
    long startY = (destY < 0) ? 0 : destY;
    if (destEndX > destImgW) destEndX = destImgW;
    if (destEndY > destImgH) destEndY = destImgH;

    WORD srcBpp = srcDIB->bitmapInfo->biBitCount;
    WORD destBpp = self->bitmapInfo->biBitCount;
    long srcImgW = srcDIB->bitmapInfo->biWidth;
    long srcImgH = srcDIB->bitmapInfo->biHeight;
    long srcStride = dibStride(srcImgW, srcBpp);
    long destStride = dibStride(destImgW, destBpp);

    unsigned char* srcBits = srcDIB->bitmap;
    unsigned char* destBits = self->bitmap;
    RGBQUAD* srcPal = srcDIB->palette;

    // The no-transparency path (transColor < 0)
    if (transColor < 0) {
        if (srcBpp == 8 && destBpp == 8) {
            for (long y = startY; y < destEndY; y++) {
                long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
                long destRow = destImgH - y - 1;
                unsigned char* dst = destBits + destRow * destStride + startX;
                for (long x = startX; x < destEndX; x++) {
                    long srcCol = (x - destX) * srcRW / destW + srcRX;
                    *dst = srcBits[srcRow * srcStride + srcCol];
                    dst++;
                }
            }
            return 1;
        }
        if (srcBpp == 8 && destBpp == 24 && srcPal) {
            for (long y = startY; y < destEndY; y++) {
                long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
                long destRow = destImgH - y - 1;
                unsigned char* dst = destBits + destRow * destStride + startX * 3;
                for (long x = startX; x < destEndX; x++) {
                    long srcCol = (x - destX) * srcRW / destW + srcRX;
                    unsigned char idx = srcBits[srcRow * srcStride + srcCol];
                    RGBQUAD& c = srcPal[idx];
                    dst[0] = c.rgbBlue;
                    dst[1] = c.rgbGreen;
                    dst[2] = c.rgbRed;
                    dst += 3;
                }
            }
            return 1;
        }
        if (srcBpp == 8 && destBpp == 16 && srcPal) {
            for (long y = startY; y < destEndY; y++) {
                long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
                long destRow = destImgH - y - 1;
                unsigned short* dst = (unsigned short*)(destBits + destRow * destStride) + startX;
                for (long x = startX; x < destEndX; x++) {
                    long srcCol = (x - destX) * srcRW / destW + srcRX;
                    unsigned char idx = srcBits[srcRow * srcStride + srcCol];
                    RGBQUAD& c = srcPal[idx];
                    *dst = rgbToRgb555(c.rgbBlue, c.rgbGreen, c.rgbRed);
                    dst++;
                }
            }
            return 1;
        }
        if (srcBpp == 24 && destBpp == 24) {
            for (long y = startY; y < destEndY; y++) {
                long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
                long destRow = destImgH - y - 1;
                unsigned char* dst = destBits + destRow * destStride + startX * 3;
                for (long x = startX; x < destEndX; x++) {
                    long srcCol = (x - destX) * srcRW / destW + srcRX;
                    unsigned char* src = srcBits + srcRow * srcStride + srcCol * 3;
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                    dst += 3;
                }
            }
            return 1;
        }
        return 0;
    }

    // Transparency path (transColor >= 0)
    if (srcBpp == 8 && destBpp == 8) {
        for (long y = startY; y < destEndY; y++) {
            long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
            long destRow = destImgH - y - 1;
            unsigned char* dst = destBits + destRow * destStride + startX;
            for (long x = startX; x < destEndX; x++) {
                long srcCol = (x - destX) * srcRW / destW + srcRX;
                unsigned char pixel = srcBits[srcRow * srcStride + srcCol];
                if (pixel != (unsigned char)transColor) {
                    *dst = pixel;
                }
                dst++;
            }
        }
        return 1;
    }
    if (srcBpp == 8 && destBpp == 24 && srcPal) {
        for (long y = startY; y < destEndY; y++) {
            long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
            long destRow = destImgH - y - 1;
            unsigned char* dst = destBits + destRow * destStride + startX * 3;
            for (long x = startX; x < destEndX; x++) {
                long srcCol = (x - destX) * srcRW / destW + srcRX;
                unsigned char idx = srcBits[srcRow * srcStride + srcCol];
                if ((unsigned int)idx != (unsigned int)transColor) {
                    RGBQUAD& c = srcPal[idx];
                    dst[0] = c.rgbBlue;
                    dst[1] = c.rgbGreen;
                    dst[2] = c.rgbRed;
                }
                dst += 3;
            }
        }
        return 1;
    }
    if (srcBpp == 8 && destBpp == 16 && srcPal) {
        for (long y = startY; y < destEndY; y++) {
            long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
            long destRow = destImgH - y - 1;
            unsigned short* dst = (unsigned short*)(destBits + destRow * destStride) + startX;
            for (long x = startX; x < destEndX; x++) {
                long srcCol = (x - destX) * srcRW / destW + srcRX;
                unsigned char idx = srcBits[srcRow * srcStride + srcCol];
                if ((unsigned int)idx != (unsigned int)transColor) {
                    RGBQUAD& c = srcPal[idx];
                    *dst = rgbToRgb555(c.rgbBlue, c.rgbGreen, c.rgbRed);
                }
                dst++;
            }
        }
        return 1;
    }
    if (srcBpp == 24 && destBpp == 24) {
        for (long y = startY; y < destEndY; y++) {
            long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
            long destRow = destImgH - y - 1;
            unsigned char* dst = destBits + destRow * destStride + startX * 3;
            for (long x = startX; x < destEndX; x++) {
                long srcCol = (x - destX) * srcRW / destW + srcRX;
                unsigned char* src = srcBits + srcRow * srcStride + srcCol * 3;
                // transColor is packed BGR for 24bpp
                unsigned long pixel = src[0] | (src[1] << 8) | (src[2] << 16);
                if (pixel != (unsigned long)transColor) {
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                }
                dst += 3;
            }
        }
        return 1;
    }

    return 0;
}

// ============================================================================
// ZOOM EXTENDED - with blending, palette offset, and additive modes
// ============================================================================

// Helper: clamp int to byte range
static inline unsigned char clampByte(int val) {
    if (val < 0) return 0;
    if (val > 255) return 255;
    return (unsigned char)val;
}

/**
 * RKC_DIB::ZoomToDIBEx - Scale with blending and palette offset
 * USED BY: o_RKC_UPDIB.dll
 *
 * Extended zoom with additional effects:
 *   palOffset  - added to source palette index (only low byte used)
 *   transColor - transparency color (must be >= 0; if < 0, returns 0 immediately)
 *   blendAmt   - blend factor 0-1000 (1000 = fully opaque source)
 *   flags      - bit 2: additive blending mode
 *
 * Supports: 8->8, 8->16, 8->24, 24->24
 */
extern "C" int __thiscall RKC_DIB_ZoomToDIBEx(
    RKC_DIB* self, RECT* destRect, RKC_DIB* srcDIB, RECT* srcRect,
    long palOffset, long transColor, long blendAmt, long flags)
{
    if (!self->bitmap || !srcDIB->bitmap) return 0;
    if (!self->bitmapInfo || !srcDIB->bitmapInfo) return 0;

    // destRect/srcRect used as {x, y, width, height}
    long destX = destRect->left;
    long destY = destRect->top;
    long destW = destRect->right;
    long destH = destRect->bottom;
    long srcRX = srcRect->left;
    long srcRY = srcRect->top;
    long srcRW = srcRect->right;
    long srcRH = srcRect->bottom;

    long destEndX = destX + destW;
    long destEndY = destY + destH;

    long destImgW = self->bitmapInfo->biWidth;
    long destImgH = self->bitmapInfo->biHeight;

    if (destImgW <= destX) return 0;
    if (destImgH <= destY) return 0;
    if (destEndX < 1) return 0;
    if (destEndY < 1) return 0;

    long startX = (destX < 0) ? 0 : destX;
    long startY = (destY < 0) ? 0 : destY;
    if (destEndX > destImgW) destEndX = destImgW;
    if (destEndY > destImgH) destEndY = destImgH;

    // transColor must be >= 0 for ZoomToDIBEx
    if (transColor < 0) return 0;

    WORD srcBpp = srcDIB->bitmapInfo->biBitCount;
    WORD destBpp = self->bitmapInfo->biBitCount;
    long srcImgW = srcDIB->bitmapInfo->biWidth;
    long srcImgH = srcDIB->bitmapInfo->biHeight;
    long srcStride = dibStride(srcImgW, srcBpp);
    long destStride = dibStride(destImgW, destBpp);

    unsigned char* srcBits = srcDIB->bitmap;
    unsigned char* destBits = self->bitmap;
    RGBQUAD* srcPal = srcDIB->palette;
    unsigned char palOff = (unsigned char)(palOffset & 0xFF);

    // The original checks (flags & 4) to decide between normal and additive blending
    bool additive = (flags & 4) != 0;

    // ========== 8bpp -> 8bpp: palette index shifting, no color blending ==========
    if (srcBpp == 8 && destBpp == 8) {
        for (long y = startY; y < destEndY; y++) {
            long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
            long destRow = destImgH - y - 1;
            unsigned char* dst = destBits + destRow * destStride + startX;
            for (long x = startX; x < destEndX; x++) {
                long srcCol = (x - destX) * srcRW / destW + srcRX;
                unsigned char pixel = srcBits[srcRow * srcStride + srcCol];
                if ((unsigned int)pixel != (unsigned int)transColor) {
                    *dst = pixel + palOff;
                }
                dst++;
            }
        }
        return 1;
    }

    // ========== 8bpp -> 24bpp ==========
    if (srcBpp == 8 && destBpp == 24 && srcPal && !additive) {
        for (long y = startY; y < destEndY; y++) {
            long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
            long destRow = destImgH - y - 1;
            unsigned char* dst = destBits + destRow * destStride + startX * 3;
            for (long x = startX; x < destEndX; x++) {
                long srcCol = (x - destX) * srcRW / destW + srcRX;
                unsigned char idx = srcBits[srcRow * srcStride + srcCol];
                if ((unsigned int)idx != (unsigned int)transColor) {
                    RGBQUAD& c = srcPal[(unsigned int)palOff + (unsigned int)idx];
                    if (blendAmt >= 1000) {
                        dst[0] = c.rgbBlue;
                        dst[1] = c.rgbGreen;
                        dst[2] = c.rgbRed;
                    } else {
                        // Blend: dest = dest * (1000 - blend) / 1000 + src * blend / 1000
                        long inv = 1000 - blendAmt;
                        dst[0] = (unsigned char)((int)dst[0] * inv / 1000 + (int)c.rgbBlue * blendAmt / 1000);
                        dst[1] = (unsigned char)((int)dst[1] * inv / 1000 + (int)c.rgbGreen * blendAmt / 1000);
                        dst[2] = (unsigned char)((int)dst[2] * inv / 1000 + (int)c.rgbRed * blendAmt / 1000);
                    }
                }
                dst += 3;
            }
        }
        return 1;
    }

    // ========== 8bpp -> 16bpp ==========
    if (srcBpp == 8 && destBpp == 16 && srcPal && !additive) {
        for (long y = startY; y < destEndY; y++) {
            long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
            long destRow = destImgH - y - 1;
            unsigned short* dst = (unsigned short*)(destBits + destRow * destStride) + startX;
            for (long x = startX; x < destEndX; x++) {
                long srcCol = (x - destX) * srcRW / destW + srcRX;
                unsigned char idx = srcBits[srcRow * srcStride + srcCol];
                if ((unsigned int)idx != (unsigned int)transColor) {
                    RGBQUAD& c = srcPal[(unsigned int)palOff + (unsigned int)idx];
                    if (blendAmt >= 1000) {
                        *dst = rgbToRgb555(c.rgbBlue, c.rgbGreen, c.rgbRed);
                    } else {
                        // Unpack existing dest RGB555
                        unsigned short existing = *dst;
                        int dR = (existing >> 7) & 0xF8;
                        int dG = (existing >> 2) & 0xF8;
                        int dB = (existing & 0x1F) << 3;
                        long inv = 1000 - blendAmt;
                        int rr = (int)c.rgbRed * blendAmt / 1000 + dR * inv / 1000;
                        int gg = (int)c.rgbGreen * blendAmt / 1000 + dG * inv / 1000;
                        int bb = (int)c.rgbBlue * blendAmt / 1000 + dB * inv / 1000;
                        *dst = rgbToRgb555(clampByte(bb), clampByte(gg), clampByte(rr));
                    }
                }
                dst++;
            }
        }
        return 1;
    }

    // ========== 24bpp -> 24bpp (normal blending) ==========
    if (srcBpp == 24 && destBpp == 24 && !additive) {
        for (long y = startY; y < destEndY; y++) {
            long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
            long destRow = destImgH - y - 1;
            unsigned char* dst = destBits + destRow * destStride + startX * 3;
            for (long x = startX; x < destEndX; x++) {
                long srcCol = (x - destX) * srcRW / destW + srcRX;
                unsigned char* src = srcBits + srcRow * srcStride + srcCol * 3;
                unsigned long pixel = src[0] | (src[1] << 8) | (src[2] << 16);
                if (pixel != (unsigned long)transColor) {
                    if (blendAmt >= 1000) {
                        dst[0] = src[0];
                        dst[1] = src[1];
                        dst[2] = src[2];
                    } else {
                        long inv = 1000 - blendAmt;
                        dst[0] = (unsigned char)((int)dst[0] * inv / 1000 + (int)src[0] * blendAmt / 1000);
                        dst[1] = (unsigned char)((int)dst[1] * inv / 1000 + (int)src[1] * blendAmt / 1000);
                        dst[2] = (unsigned char)((int)dst[2] * inv / 1000 + (int)src[2] * blendAmt / 1000);
                    }
                }
                dst += 3;
            }
        }
        return 1;
    }

    // ========== Additive blending modes (flags & 4) ==========

    // 8bpp -> 16bpp additive
    if (srcBpp == 8 && destBpp == 16 && srcPal && additive) {
        long excess = blendAmt - 1000;
        for (long y = startY; y < destEndY; y++) {
            long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
            long destRow = destImgH - y - 1;
            unsigned short* dst = (unsigned short*)(destBits + destRow * destStride) + startX;
            for (long x = startX; x < destEndX; x++) {
                long srcCol = (x - destX) * srcRW / destW + srcRX;
                unsigned char idx = srcBits[srcRow * srcStride + srcCol];
                if ((unsigned int)idx != (unsigned int)transColor) {
                    RGBQUAD& c = srcPal[(unsigned int)palOff + (unsigned int)idx];
                    unsigned short existing = *dst;
                    int dR = (existing >> 7) & 0xF8;
                    int dG = (existing >> 2) & 0xF8;
                    int dB = (existing & 0x1F) << 3;
                    int rr, gg, bb;
                    if (blendAmt >= 1000) {
                        if (blendAmt == 1000) {
                            // Pure additive: dest + src, clamped
                            rr = dR + (int)c.rgbRed;
                            gg = dG + (int)c.rgbGreen;
                            bb = dB + (int)c.rgbBlue;
                        } else {
                            // Excess additive: dest + src + (255-src)*excess/1000
                            rr = dR + (int)c.rgbRed + (255 - (int)c.rgbRed) * excess / 1000;
                            gg = dG + (int)c.rgbGreen + (255 - (int)c.rgbGreen) * excess / 1000;
                            bb = dB + (int)c.rgbBlue + (255 - (int)c.rgbBlue) * excess / 1000;
                        }
                    } else {
                        // Partial additive: dest + src * blend / 1000
                        rr = dR + (int)c.rgbRed * blendAmt / 1000;
                        gg = dG + (int)c.rgbGreen * blendAmt / 1000;
                        bb = dB + (int)c.rgbBlue * blendAmt / 1000;
                    }
                    *dst = rgbToRgb555(clampByte(bb), clampByte(gg), clampByte(rr));
                }
                dst++;
            }
        }
        return 1;
    }

    // 8bpp -> 24bpp additive
    if (srcBpp == 8 && destBpp == 24 && srcPal && additive) {
        long excess = blendAmt - 1000;
        for (long y = startY; y < destEndY; y++) {
            long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
            long destRow = destImgH - y - 1;
            unsigned char* dst = destBits + destRow * destStride + startX * 3;
            for (long x = startX; x < destEndX; x++) {
                long srcCol = (x - destX) * srcRW / destW + srcRX;
                unsigned char idx = srcBits[srcRow * srcStride + srcCol];
                if ((unsigned int)idx != (unsigned int)transColor) {
                    RGBQUAD& c = srcPal[(unsigned int)palOff + (unsigned int)idx];
                    if (blendAmt >= 1000) {
                        if (blendAmt == 1000) {
                            dst[0] = clampByte((int)dst[0] + c.rgbBlue);
                            dst[1] = clampByte((int)dst[1] + c.rgbGreen);
                            dst[2] = clampByte((int)dst[2] + c.rgbRed);
                        } else {
                            dst[0] = clampByte((int)dst[0] + c.rgbBlue + (255 - (int)c.rgbBlue) * excess / 1000);
                            dst[1] = clampByte((int)dst[1] + c.rgbGreen + (255 - (int)c.rgbGreen) * excess / 1000);
                            dst[2] = clampByte((int)dst[2] + c.rgbRed + (255 - (int)c.rgbRed) * excess / 1000);
                        }
                    } else {
                        dst[0] = clampByte((int)c.rgbBlue * blendAmt / 1000 + (int)dst[0]);
                        dst[1] = clampByte((int)c.rgbGreen * blendAmt / 1000 + (int)dst[1]);
                        dst[2] = clampByte((int)c.rgbRed * blendAmt / 1000 + (int)dst[2]);
                    }
                }
                dst += 3;
            }
        }
        return 1;
    }

    // 24bpp -> 24bpp additive
    if (srcBpp == 24 && destBpp == 24 && additive) {
        long excess = blendAmt - 1000;
        for (long y = startY; y < destEndY; y++) {
            long srcRow = srcImgH - ((y - destY) * srcRH / destH + srcRY) - 1;
            long destRow = destImgH - y - 1;
            unsigned char* dst = destBits + destRow * destStride + startX * 3;
            for (long x = startX; x < destEndX; x++) {
                long srcCol = (x - destX) * srcRW / destW + srcRX;
                unsigned char* src = srcBits + srcRow * srcStride + srcCol * 3;
                unsigned long pixel = src[0] | (src[1] << 8) | (src[2] << 16);
                if (pixel != (unsigned long)transColor) {
                    if (blendAmt >= 1000) {
                        if (blendAmt == 1000) {
                            dst[0] = clampByte((int)dst[0] + src[0]);
                            dst[1] = clampByte((int)dst[1] + src[1]);
                            dst[2] = clampByte((int)dst[2] + src[2]);
                        } else {
                            dst[0] = clampByte((int)dst[0] + src[0] + (255 - (int)src[0]) * excess / 1000);
                            dst[1] = clampByte((int)dst[1] + src[1] + (255 - (int)src[1]) * excess / 1000);
                            dst[2] = clampByte((int)dst[2] + src[2] + (255 - (int)src[2]) * excess / 1000);
                        }
                    } else {
                        dst[0] = clampByte((int)src[0] * blendAmt / 1000 + (int)dst[0]);
                        dst[1] = clampByte((int)src[1] * blendAmt / 1000 + (int)dst[1]);
                        dst[2] = clampByte((int)src[2] * blendAmt / 1000 + (int)dst[2]);
                    }
                }
                dst += 3;
            }
        }
        return 1;
    }

    return 0;
}

// ============================================================================
// STUBS FOR FUNCTIONS NOT IMPORTED BY ANY MODULE
// ============================================================================

/**
 * RKC_DIB::Convert - Convert between BPP modes
 * USED BY: ShadowFlare.exe (save game screenshot mask)
 *
 * Converts src DIB (must be <=8bpp) into dest (this).
 * If dest is uninitialized, creates it with dimensions from src and BPP = mode.
 * If dest already exists, converts in-place using existing dest format.
 *
 * Supported conversions:
 *   8bpp -> 1bpp (pixel != 0 -> bit 1, MSB first)
 *   8bpp -> 4bpp (low nibble packed, high nibble = even pixel)
 *   1/4/8bpp -> 8bpp (unpack to individual bytes, copies palette)
 *   1/4/8bpp -> 24bpp (unpack + palette lookup to BGR)
 */
extern "C" int __thiscall RKC_DIB_Convert(RKC_DIB* self, RKC_DIB* src, long mode) {
    if (!src->bitmap) return 0;

    BITMAPINFOHEADER* srcHdr = src->bitmapInfo;
    WORD srcBpp = srcHdr->biBitCount;
    if (srcBpp > 8) return 0;

    // If dest already has a header, validate compatibility
    if (self->bitmapInfo) {
        WORD destBpp = self->bitmapInfo->biBitCount;
        if (destBpp == 1 && srcBpp != 8) return 0;
        if (destBpp == 8 && srcBpp == 8) return 0;
        if (destBpp == 4 && srcBpp != 8) return 0;
    } else {
        // Dest uninitialized - create based on mode
        // Valid modes: 1, 4, 8, 24
        if (mode == 8 || mode == 24 || mode == 4) {
            if (mode == 1) {
                // mode==1 requires src 8bpp
                if (srcBpp != 8) return 0;
            } else if (mode == 8) {
                if (srcBpp == 8) return 0;
            } else if (mode == 4) {
                if (srcBpp != 8) return 0;
            }
        } else if (mode == 1) {
            if (srcBpp != 8) return 0;
        } else {
            return 0;
        }

        int ret = RKC_DIB_Create(self, srcHdr->biWidth, srcHdr->biHeight, mode, 1);
        if (!ret) return 0;
    }

    long srcStride = RKC_DIB_GetAlignWidth(src);
    long destStride = RKC_DIB_GetAlignWidth(self);

    // Determine src unpacking parameters
    int srcPixelsPerByte;  // how many source pixels fit in one byte
    int srcBitsPerPixel;   // bits per pixel in source
    unsigned char srcMask; // mask for one pixel value
    if (srcBpp == 8) {
        srcPixelsPerByte = 1;
        srcBitsPerPixel = 8;
        srcMask = 0xFF;
    } else if (srcBpp == 4) {
        srcPixelsPerByte = 2;
        srcBitsPerPixel = 4;
        srcMask = 0x0F;
    } else {
        // 1bpp
        srcPixelsPerByte = 8;
        srcBitsPerPixel = 1;
        srcMask = 0x01;
    }

    WORD destBpp = self->bitmapInfo->biBitCount;
    long srcWidth = srcHdr->biWidth;
    long destHeight = self->bitmapInfo->biHeight;

    if (destBpp == 1) {
        // Convert to 1bpp: pack 8 pixels per byte, MSB first
        // Each source pixel != 0 becomes a 1 bit
        int srcOffset = 0;
        unsigned char* destRow = self->bitmap;
        for (int y = 0; y < destHeight; y++) {
            long destWidthPx = self->bitmapInfo->biWidth;
            // Process 8 pixels at a time, aligned to byte boundaries
            int roundedWidth = ((destWidthPx + 7) / 8) * 8;
            int x = 0;
            while (x < roundedWidth) {
                unsigned char outByte = 0;
                // bit 7 (0x80) = pixel x, bit 6 (0x40) = pixel x+1, etc.
                if (x + 0 < destWidthPx && src->bitmap[srcOffset + x + 0] != 0) outByte |= 0x80;
                if (x + 1 < destWidthPx && src->bitmap[srcOffset + x + 1] != 0) outByte |= 0x40;
                if (x + 2 < destWidthPx && src->bitmap[srcOffset + x + 2] != 0) outByte |= 0x20;
                if (x + 3 < destWidthPx && src->bitmap[srcOffset + x + 3] != 0) outByte |= 0x10;
                if (x + 4 < destWidthPx && src->bitmap[srcOffset + x + 4] != 0) outByte |= 0x08;
                if (x + 5 < destWidthPx && src->bitmap[srcOffset + x + 5] != 0) outByte |= 0x04;
                if (x + 6 < destWidthPx && src->bitmap[srcOffset + x + 6] != 0) outByte |= 0x02;
                if (x + 7 < destWidthPx && src->bitmap[srcOffset + x + 7] != 0) outByte |= 0x01;
                destRow[x / 8] = outByte;
                x += 8;
            }
            srcOffset += srcStride;
            destRow += destStride;
        }
        return 1;
    }

    if (destBpp == 4) {
        // Convert 8bpp to 4bpp: pack 2 pixels per byte
        int srcOffset = 0;
        unsigned char* destRow = self->bitmap;
        for (int y = 0; y < destHeight; y++) {
            long width = self->bitmapInfo->biWidth;
            for (int x = 0; x + 2 <= width; x += 2) {
                unsigned char hi = src->bitmap[srcOffset + x] & 0x0F;
                unsigned char lo = src->bitmap[srcOffset + x + 1] & 0x0F;
                destRow[x / 2] = (hi << 4) | lo;
            }
            srcOffset += srcStride;
            destRow += destStride;
        }
        return 1;
    }

    if (destBpp == 8) {
        // Convert 1/4bpp to 8bpp: unpack each pixel to a full byte
        int srcByteOffset = 0;
        int destByteOffset = 0;
        for (int y = 0; y < destHeight; y++) {
            long width = self->bitmapInfo->biWidth;
            int dx = 0;
            while (dx < width) {
                unsigned char srcByte = src->bitmap[dx / srcPixelsPerByte + srcByteOffset];
                // Unpack pixels from high bits to low bits
                int bitsLeft = srcPixelsPerByte;
                int limit = srcPixelsPerByte;
                if (dx + srcPixelsPerByte > width) {
                    limit = width - dx;
                    if (width % srcPixelsPerByte != 0) {
                        limit = width % srcPixelsPerByte;
                    }
                }
                for (int i = srcPixelsPerByte - 1; i >= srcPixelsPerByte - limit; i--) {
                    self->bitmap[destByteOffset + dx + (srcPixelsPerByte - 1 - i)] =
                        (srcByte >> (srcBitsPerPixel * i)) & srcMask;
                }
                dx += srcPixelsPerByte;
            }
            srcByteOffset += srcStride;
            destByteOffset += destStride;
        }

        // Copy palette from source to dest
        long palCount = RKC_DIB_GetPaletteCount(src);
        if (palCount > 0 && src->palette && self->palette) {
            memcpy(self->palette, src->palette, palCount * sizeof(RGBQUAD));
        }
        return 1;
    }

    // destBpp == 24: Convert 1/4/8bpp to 24bpp using palette lookup
    {
        int srcByteOffset = 0;
        int destByteOffset = 0;
        for (int y = 0; y < destHeight; y++) {
            long width = self->bitmapInfo->biWidth;
            int dx = 0;
            int destX = 0;
            while (dx < width) {
                unsigned char srcByte = src->bitmap[dx / srcPixelsPerByte + srcByteOffset];
                int limit = srcPixelsPerByte;
                if (dx + srcPixelsPerByte > width && width % srcPixelsPerByte != 0) {
                    limit = width % srcPixelsPerByte;
                }
                // Unpack from MSB to LSB of the source byte
                for (int i = limit - 1; i >= 0; i--) {
                    int palIdx = (srcByte >> (srcBitsPerPixel * i)) & srcMask;
                    int colorOff = palIdx * 4;
                    unsigned char* destPx = self->bitmap + destByteOffset + destX;
                    destPx[0] = ((unsigned char*)src->palette)[colorOff + 0]; // B
                    destPx[1] = ((unsigned char*)src->palette)[colorOff + 1]; // G
                    destPx[2] = ((unsigned char*)src->palette)[colorOff + 2]; // R
                    destX += 3;
                }
                dx += srcPixelsPerByte;
            }
            srcByteOffset += srcStride;
            destByteOffset += destStride;
        }
    }
    return 1;
}

// -- Drawing helper functions --

static inline int clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

static inline unsigned short packRGB555(int r, int g, int b) {
    return (unsigned short)(((r & 0xF8) << 7) | ((g & 0xF8) << 2) | ((b >> 3) & 0x1F));
}

// Blend a single 16bpp pixel in-place
static void blendPixel16(unsigned short* px, unsigned char r, unsigned char g, unsigned char b,
                         long blend, long flags) {
    unsigned short old = *px;
    int old_r = (old >> 7) & 0xF8;
    int old_g = (old >> 2) & 0xF8;
    int old_b = (old & 0x1F) << 3;

    if (flags & 4) {
        // Additive: add fill color (attenuated) to existing
        *px = packRGB555(clamp255(old_r + (int)r * blend / 1000),
                         clamp255(old_g + (int)g * blend / 1000),
                         clamp255(old_b + (int)b * blend / 1000));
    } else if (flags & 8) {
        if (blend < 1000) {
            // Darken: reduce existing pixel brightness
            int factor = 1000 - blend;
            *px = packRGB555(old_r - old_r * factor / 1000,
                             old_g - old_g * factor / 1000,
                             old_b - old_b * factor / 1000);
        } else if (blend > 1000) {
            // Brighten: push existing pixel towards white
            int factor = blend - 1000;
            *px = packRGB555(old_r + (0xFF - old_r) * factor / 1000,
                             old_g + (0xFF - old_g) * factor / 1000,
                             old_b + (0xFF - old_b) * factor / 1000);
        }
    } else {
        // Alpha blend: lerp between existing and fill color
        int inv = 1000 - blend;
        *px = packRGB555(old_r * inv / 1000 + (int)r * blend / 1000,
                         old_g * inv / 1000 + (int)g * blend / 1000,
                         old_b * inv / 1000 + (int)b * blend / 1000);
    }
}

// Blend a single 24bpp pixel (BGR byte order) in-place
static void blendPixel24(unsigned char* px, unsigned char r, unsigned char g, unsigned char b,
                         long blend, long flags) {
    if (flags & 4) {
        px[0] = clamp255(px[0] + (int)b * blend / 1000);
        px[1] = clamp255(px[1] + (int)g * blend / 1000);
        px[2] = clamp255(px[2] + (int)r * blend / 1000);
    } else if (flags & 8) {
        if (blend < 1000) {
            int factor = 1000 - blend;
            px[0] -= (int)px[0] * factor / 1000;
            px[1] -= (int)px[1] * factor / 1000;
            px[2] -= (int)px[2] * factor / 1000;
        } else if (blend > 1000) {
            int factor = blend - 1000;
            px[0] += (0xFF - px[0]) * factor / 1000;
            px[1] += (0xFF - px[1]) * factor / 1000;
            px[2] += (0xFF - px[2]) * factor / 1000;
        }
    } else {
        int inv = 1000 - blend;
        px[0] = px[0] * inv / 1000 + (int)b * blend / 1000;
        px[1] = px[1] * inv / 1000 + (int)g * blend / 1000;
        px[2] = px[2] * inv / 1000 + (int)r * blend / 1000;
    }
}

/**
 * RKC_DIB::DrawPoint - Draw a single pixel with optional blending
 * USED BY: o_RKC_DIB.dll (called by DrawLine)
 *
 * For 8bpp: 'r' is the palette index, no blending.
 * For 16bpp: RGB555, supports opaque/alpha/additive/darken/brighten.
 * For 24bpp: BGR byte order, same blend modes.
 *
 * blend=1000 means opaque. flags: bit 2=additive, bit 3=darken/brighten.
 */
extern "C" int __thiscall RKC_DIB_DrawPoint(RKC_DIB* self, long x, long y,
    unsigned char r, unsigned char g, unsigned char b, long blend, long flags, RECT* clipRect) {
    BITMAPINFOHEADER* hdr = self->bitmapInfo;
    if (!hdr) return 0;
    WORD bpp = hdr->biBitCount;
    if (bpp != 8 && bpp != 16 && bpp != 24) return 0;

    // Clip rect check
    if (clipRect) {
        if (x < clipRect->left || x > clipRect->right ||
            y < clipRect->top || y > clipRect->bottom) return 0;
    }
    // Bounds check
    if (x < 0 || y < 0 || x >= hdr->biWidth || y >= hdr->biHeight) return 0;

    long stride = RKC_DIB_GetAlignWidth(self);
    // DIBs are bottom-up
    long rowOff = (hdr->biHeight - y - 1) * stride;

    if (bpp == 8) {
        self->bitmap[rowOff + x] = r;
        return 1;
    }
    if (bpp == 16) {
        unsigned short* px = (unsigned short*)(self->bitmap + rowOff + x * 2);
        if (blend == 1000 && (flags & 0x0C) == 0) {
            *px = packRGB555(r, g, b);
        } else {
            blendPixel16(px, r, g, b, blend, flags);
        }
        return 1;
    }
    // 24bpp
    unsigned char* px = self->bitmap + rowOff + x * 3;
    if (blend == 1000 && (flags & 0x0C) == 0) {
        px[0] = b; px[1] = g; px[2] = r;
    } else {
        blendPixel24(px, r, g, b, blend, flags);
    }
    return 1;
}

/**
 * RKC_DIB::DrawLine - Draw line between two points using DDA
 * USED BY: o_RKC_DIB.dll (called by DrawBox)
 *
 * Steps along the longer axis, interpolating the shorter one.
 * Each pixel drawn via DrawPoint which handles clipping and blending.
 */
extern "C" int __thiscall RKC_DIB_DrawLine(RKC_DIB* self, long x1, long y1, long x2, long y2,
    unsigned char r, unsigned char g, unsigned char b, long blend, long flags, RECT* clipRect) {
    BITMAPINFOHEADER* hdr = self->bitmapInfo;
    if (!hdr) return 0;
    WORD bpp = hdr->biBitCount;
    if (bpp != 8 && bpp != 16 && bpp != 24) return 0;

    // Matches the original's DDA approach: step along longer axis,
    // use accumulator/count for the shorter axis interpolation.
    if (x1 < x2) {
        int dx = (int)(x2 - x1) + 1;
        if (y1 < y2) {
            int dy = (int)(y2 - y1) + 1;
            if (dy < dx) {
                int acc = 0;
                for (int i = 0; i < dx; i++) {
                    RKC_DIB_DrawPoint(self, x1 + i, acc / dx + y1, r, g, b, blend, flags, clipRect);
                    acc += dy;
                }
            } else {
                int acc = 0;
                for (int i = 0; i < dy; i++) {
                    RKC_DIB_DrawPoint(self, acc / dy + x1, y1 + i, r, g, b, blend, flags, clipRect);
                    acc += dx;
                }
            }
        } else if (y2 < y1) {
            int dy = (int)(y1 - y2) + 1;
            if (dy < dx) {
                int acc = 0;
                for (int i = 0; i < dx; i++) {
                    RKC_DIB_DrawPoint(self, x1 + i, y1 - acc / dx, r, g, b, blend, flags, clipRect);
                    acc += dy;
                }
            } else {
                int acc = 0;
                for (int i = 0; i < dy; i++) {
                    RKC_DIB_DrawPoint(self, acc / dy + x1, y1 - i, r, g, b, blend, flags, clipRect);
                    acc += dx;
                }
            }
        } else {
            // Horizontal line (y1 == y2)
            for (int i = 0; i < dx; i++)
                RKC_DIB_DrawPoint(self, x1 + i, y1, r, g, b, blend, flags, clipRect);
        }
    } else if (x2 < x1) {
        int dx = (int)(x1 - x2) + 1;
        if (y1 < y2) {
            int dy = (int)(y2 - y1) + 1;
            if (dy < dx) {
                int acc = 0;
                long cx = x1;
                for (int i = 0; i < dx; i++) {
                    RKC_DIB_DrawPoint(self, cx, acc / dx + y1, r, g, b, blend, flags, clipRect);
                    acc += dy;
                    cx--;
                }
            } else {
                int acc = 0;
                for (int i = 0; i < dy; i++) {
                    RKC_DIB_DrawPoint(self, x1 - acc / dy, y1 + i, r, g, b, blend, flags, clipRect);
                    acc += dx;
                }
            }
        } else if (y2 < y1) {
            int dx2 = dx;
            int dy = (int)(y1 - y2) + 1;
            if (dy < dx2) {
                int acc = 0;
                long cx = x1;
                for (int i = 0; i < dx2; i++) {
                    RKC_DIB_DrawPoint(self, cx, y1 - acc / dx2, r, g, b, blend, flags, clipRect);
                    acc += dy;
                    cx--;
                }
            } else {
                int acc = 0;
                long cy = y1;
                for (int i = 0; i < dy; i++) {
                    RKC_DIB_DrawPoint(self, x1 - acc / dy, cy, r, g, b, blend, flags, clipRect);
                    cy--;
                    acc += dx2;
                }
            }
        } else {
            // Horizontal line going left
            long cx = x1;
            for (int i = 0; i < dx; i++) {
                RKC_DIB_DrawPoint(self, cx, y1, r, g, b, blend, flags, clipRect);
                cx--;
            }
        }
    } else {
        // x1 == x2 - vertical line
        if (y1 < y2) {
            int dy = (int)(y2 - y1) + 1;
            for (int i = 0; i < dy; i++)
                RKC_DIB_DrawPoint(self, x1, y1 + i, r, g, b, blend, flags, clipRect);
        } else if (y2 < y1) {
            int dy = (int)(y1 - y2) + 1;
            long cy = y1;
            for (int i = 0; i < dy; i++) {
                RKC_DIB_DrawPoint(self, x1, cy, r, g, b, blend, flags, clipRect);
                cy--;
            }
        } else {
            // Single point
            RKC_DIB_DrawPoint(self, x1, y1, r, g, b, blend, flags, clipRect);
        }
    }
    return 1;
}

/**
 * RKC_DIB::DrawBox - Draw outlined rectangle (4 lines)
 * USED BY: o_RKC_UPDIB.dll (via VSPACKET::RenderBox)
 *
 * Draws 4 edges: top, bottom, left, right. Clips to bitmap bounds
 * before calling DrawLine.
 */
extern "C" int __thiscall RKC_DIB_DrawBox(RKC_DIB* self, long x1, long y1, long x2, long y2,
    unsigned char r, unsigned char g, unsigned char b, long blend, long flags, RECT* clipRect) {
    BITMAPINFOHEADER* hdr = self->bitmapInfo;
    if (!hdr) return 0;
    WORD bpp = hdr->biBitCount;
    if (bpp != 8 && bpp != 16 && bpp != 24) return 0;

    long w = hdr->biWidth;
    long h = hdr->biHeight;

    // Normalize coordinates
    long left = (x1 <= x2) ? x1 : x2;
    long right = (x1 > x2) ? x1 : x2;
    long top = (y1 <= y2) ? y1 : y2;
    long bottom = (y1 > y2) ? y1 : y2;

    // Top edge
    if (left < w && right >= 0 && top < h && top >= 0) {
        long cl = left < 0 ? 0 : left;
        long cr = right >= w ? w - 1 : right;
        RKC_DIB_DrawLine(self, cl, top, cr, top, r, g, b, blend, flags, clipRect);
    }
    // Bottom edge
    if (left < w && right >= 0 && bottom < h && bottom >= 0) {
        long cl = left < 0 ? 0 : left;
        long cr = right >= w ? w - 1 : right;
        RKC_DIB_DrawLine(self, cl, bottom, cr, bottom, r, g, b, blend, flags, clipRect);
    }
    // Left edge
    if (left < w && left >= 0 && top < h && bottom >= 0) {
        long ct = top < 0 ? 0 : top;
        long cb = bottom >= h ? h - 1 : bottom;
        RKC_DIB_DrawLine(self, left, ct, left, cb, r, g, b, blend, flags, clipRect);
    }
    // Right edge
    if (right < w && right >= 0 && top < h && bottom >= 0) {
        long ct = top < 0 ? 0 : top;
        long cb = bottom >= h ? h - 1 : bottom;
        RKC_DIB_DrawLine(self, right, ct, right, cb, r, g, b, blend, flags, clipRect);
    }
    return 1;
}

/**
 * RKC_DIB::DrawFill - Draw filled rectangle with optional blending
 * USED BY: o_RKC_UPDIB.dll (via VSPACKET::RenderFill -> tooltip backgrounds, UI panels)
 *
 * For 8bpp: 'r' is the palette index, no blending (just memset).
 * For 16bpp/24bpp: supports opaque, alpha blend, additive, darken, brighten.
 * blend=1000 means fully opaque. flags: bit 2=additive, bit 3=darken/brighten.
 * hispeed is a lookup table for optimized blending (we fall through to slow path).
 */
extern "C" int __thiscall RKC_DIB_DrawFill(RKC_DIB* self, long x1, long y1, long x2, long y2,
    unsigned char r, unsigned char g, unsigned char b, long blend, long flags, RECT* clipRect,
    void* hispeed) {
    BITMAPINFOHEADER* hdr = self->bitmapInfo;
    if (!hdr) return 0;
    WORD bpp = hdr->biBitCount;
    if (bpp != 8 && bpp != 16 && bpp != 24) return 0;

    long w = hdr->biWidth;
    long h = hdr->biHeight;

    // Normalize coordinates so left <= right, top <= bottom
    long left = (x1 <= x2) ? x1 : x2;
    long top_y = (y1 <= y2) ? y1 : y2;
    long right = (x1 > x2) ? x1 : x2;
    long bottom_y = (y1 > y2) ? y1 : y2;

    // Validate clip rect if provided
    if (clipRect) {
        if (clipRect->left >= w || clipRect->right < 0 ||
            clipRect->top >= h || clipRect->bottom < 0) return 0;
        if (left > clipRect->right || right < clipRect->left ||
            top_y > clipRect->bottom || bottom_y < clipRect->top) return 0;

        // Apply clip
        if (left < clipRect->left) left = clipRect->left;
        if (top_y < clipRect->top) top_y = clipRect->top;
        if (right > clipRect->right) right = clipRect->right;
        if (bottom_y > clipRect->bottom) bottom_y = clipRect->bottom;
    }

    // Visible area check
    if (left >= w || right < 0 || top_y >= h || bottom_y < 0) return 0;

    // Clamp to bitmap bounds
    if (left < 0) left = 0;
    if (top_y < 0) top_y = 0;
    if (right >= w) right = w - 1;
    if (bottom_y >= h) bottom_y = h - 1;
    if (left < 0 || top_y < 0 || right < 0 || bottom_y < 0) return 0;

    long stride = RKC_DIB_GetAlignWidth(self);
    long fillWidth = right - left + 1;

    // -- 8bpp: simple memset, no blending --
    if (bpp == 8) {
        for (long y = top_y; y <= bottom_y; y++) {
            unsigned char* row = self->bitmap + (h - y - 1) * stride + left;
            memset(row, r, fillWidth);
        }
        return 1;
    }

    // -- 16bpp (RGB555) --
    if (bpp == 16) {
        // Opaque fill: no blending, no flags
        if ((flags & 0x0C) == 0 && blend == 1000) {
            unsigned short color = packRGB555(r, g, b);
            for (long y = top_y; y <= bottom_y; y++) {
                unsigned short* row = (unsigned short*)(self->bitmap + (h - y - 1) * stride + left * 2);
                for (long i = 0; i < fillWidth; i++)
                    row[i] = color;
            }
            return 1;
        }
        // Blended fill: process each pixel
        for (long y = top_y; y <= bottom_y; y++) {
            unsigned short* row = (unsigned short*)(self->bitmap + (h - y - 1) * stride + left * 2);
            for (long i = 0; i < fillWidth; i++)
                blendPixel16(&row[i], r, g, b, blend, flags);
        }
        return 1;
    }

    // -- 24bpp (BGR byte order) --
    // Opaque fill
    if ((flags & 0x0C) == 0 && blend == 1000) {
        for (long y = top_y; y <= bottom_y; y++) {
            unsigned char* row = self->bitmap + (h - y - 1) * stride + left * 3;
            for (long i = 0; i < fillWidth; i++) {
                row[i * 3]     = b;
                row[i * 3 + 1] = g;
                row[i * 3 + 2] = r;
            }
        }
        return 1;
    }
    // Blended fill
    for (long y = top_y; y <= bottom_y; y++) {
        unsigned char* row = self->bitmap + (h - y - 1) * stride + left * 3;
        for (long i = 0; i < fillWidth; i++)
            blendPixel24(&row[i * 3], r, g, b, blend, flags);
    }
    return 1;
}

/**
 * RKC_DIB::TransferToDIBEx - Extended blit with blending (12 stack params version)
 * USED BY: o_RKC_UPDIB.dll, ShadowFlare.exe
 *
 * Blits src DIB region onto dest (this) with palette lookup, transparency,
 * alpha blending, and optional horizontal/vertical flipping.
 *
 * Params:
 *   destX, destY: destination position
 *   width, height: region size
 *   srcDIB: source bitmap
 *   srcX, srcY: source position
 *   palOff: palette offset added to 8bpp src indices (& 0xFF)
 *   transColor: transparent color index (skip if src pixel == transColor)
 *   alpha: blend factor 0-1000 (1000 = opaque), or 0-2000 if additive
 *   flags: bit 0-1 = flip mode (0=normal, 1=hflip, 2=vflip, 3=hvflip)
 *          bit 2 = additive blending
 *          bit 4 = use hispeed lookup tables
 */
extern "C" int __thiscall RKC_DIB_TransferToDIBEx_11args(
    RKC_DIB* self, long destX, long destY, long width, long height,
    RKC_DIB* srcDIB, long srcX, long srcY, long palOff, long transColor, long alpha, long flags,
    void* hispeed) {

    // Validate alpha range
    bool additive = (flags & 4) != 0;
    if (alpha < 0) return 0;
    if (!additive && alpha > 1000) return 0;
    if (additive && alpha > 2000) return 0;

    if (!srcDIB->bitmap || !self->bitmap) return 0;

    BITMAPINFOHEADER* srcHdr = srcDIB->bitmapInfo;
    BITMAPINFOHEADER* dstHdr = self->bitmapInfo;
    WORD srcBpp = srcHdr->biBitCount;
    WORD dstBpp = dstHdr->biBitCount;

    // Source must be 1, 4, 8, or 24 bpp
    if (srcBpp != 1 && srcBpp != 4 && srcBpp != 8 && srcBpp != 24) return 0;
    // Dest must be 4, 8, 16, or 24 bpp
    if (dstBpp != 4 && dstBpp != 8 && dstBpp != 16 && dstBpp != 24) return 0;
    // Dest bpp must be >= src bpp
    if (dstBpp < srcBpp) return 0;

    long srcW = srcHdr->biWidth;
    long srcH = srcHdr->biHeight;
    long dstW = dstHdr->biWidth;
    long dstH = dstHdr->biHeight;

    // Clip negative dest coords
    if (destX < 0) { srcX -= destX; destX = 0; }
    if (destY < 0) { srcY -= destY; destY = 0; }
    if (srcX < 0) { destX -= srcX; srcX = 0; }
    if (srcY < 0) { destY -= srcY; srcY = 0; }

    if (destX < 0 || destX >= dstW || srcX < 0 || srcX >= srcW) return 0;

    // Clip region to both source and dest bounds
    if (dstW < width + destX) width = dstW - destX;
    if (dstH < destY + height) height = dstH - destY;
    if (srcW < width + srcX) width = srcW - srcX;
    if (srcH < srcY + height) height = srcH - srcY;

    if (width < 1 || height < 1) return 0;

    long srcStride = RKC_DIB_GetAlignWidth(srcDIB);
    long dstStride = RKC_DIB_GetAlignWidth(self);

    // Flip mode from bottom 2 bits
    int flipMode = flags & 3;

    // Calculate initial source pixel offset (bottom-up DIB)
    // For flip modes, source traversal direction changes
    int srcBytesPerPixel = srcBpp / 8;  // works for 8 and 24
    int dstBytesPerPixel = dstBpp / 8;

    // ---- 8bpp source -> 8bpp dest (palette index copy w/ offset) ----
    if (srcBpp == 8 && dstBpp == 8) {
        if (additive) {
            // Additive needs hispeed LUT for 8bpp->8bpp
            if (!hispeed) return 1;
        }
        for (long row = 0; row < height; row++) {
            // Source row (bottom-up): for flipMode 0,1: row from top, for 2,3: row from bottom
            long sy = (flipMode & 2) ? srcY + row : (srcH - srcY - row - 1);
            long dy = dstH - destY - row - 1;

            unsigned char* srcRow = srcDIB->bitmap + sy * srcStride;
            unsigned char* dstRow = self->bitmap + dy * dstStride;

            for (long col = 0; col < width; col++) {
                long sx = (flipMode & 1) ? (srcX + width - 1 - col) : (srcX + col);
                unsigned char srcPx = srcRow[sx];
                if ((long)(unsigned)srcPx == transColor) continue;
                unsigned char idx = (unsigned char)(srcPx + palOff);
                if (!additive) {
                    dstRow[destX + col] = idx;
                } else {
                    // hispeed LUT-based blending for 8bpp
                    unsigned char* lut = (unsigned char*)hispeed;
                    unsigned char dst = dstRow[destX + col];
                    dstRow[destX + col] = lut[(unsigned)dst + ((unsigned)idx + 0xFA3) * 256];
                }
            }
        }
        return 1;
    }

    // ---- 8bpp source -> 24bpp dest (palette lookup + blending) ----
    if (srcBpp == 8 && dstBpp == 24) {
        RGBQUAD* palette = srcDIB->palette;
        if (!palette) return 0;

        for (long row = 0; row < height; row++) {
            long sy = (flipMode & 2) ? srcY + row : (srcH - srcY - row - 1);
            long dy = dstH - destY - row - 1;

            unsigned char* srcRow = srcDIB->bitmap + sy * srcStride;
            unsigned char* dstRow = self->bitmap + dy * dstStride;

            for (long col = 0; col < width; col++) {
                long sx = (flipMode & 1) ? (srcX + width - 1 - col) : (srcX + col);
                unsigned char srcPx = srcRow[sx];
                if ((long)(unsigned)srcPx == transColor) continue;

                unsigned char idx = (unsigned char)(srcPx + palOff);
                unsigned char* color = (unsigned char*)&palette[idx];
                unsigned char* dst = dstRow + (destX + col) * 3;

                if (alpha == 1000 && !additive && !(flags & 0x10)) {
                    // Opaque copy - BGR from palette
                    dst[0] = color[0]; // B
                    dst[1] = color[1]; // G
                    dst[2] = color[2]; // R
                } else if (additive) {
                    // Additive blending
                    dst[0] = (unsigned char)clamp255(dst[0] + (int)color[0] * alpha / 1000);
                    dst[1] = (unsigned char)clamp255(dst[1] + (int)color[1] * alpha / 1000);
                    dst[2] = (unsigned char)clamp255(dst[2] + (int)color[2] * alpha / 1000);
                } else {
                    // Alpha blend
                    int inv = 1000 - alpha;
                    dst[0] = (unsigned char)((int)color[0] * alpha / 1000 + (int)dst[0] * inv / 1000);
                    dst[1] = (unsigned char)((int)color[1] * alpha / 1000 + (int)dst[1] * inv / 1000);
                    dst[2] = (unsigned char)((int)color[2] * alpha / 1000 + (int)dst[2] * inv / 1000);
                }
            }
        }
        return 1;
    }

    // ---- 8bpp source -> 16bpp dest (RGB555 palette lookup + blending) ----
    if (srcBpp == 8 && dstBpp == 16) {
        RGBQUAD* palette = srcDIB->palette;
        if (!palette) return 0;

        for (long row = 0; row < height; row++) {
            long sy = (flipMode & 2) ? srcY + row : (srcH - srcY - row - 1);
            long dy = dstH - destY - row - 1;

            unsigned char* srcRow = srcDIB->bitmap + sy * srcStride;
            unsigned short* dstRow = (unsigned short*)(self->bitmap + dy * dstStride);

            for (long col = 0; col < width; col++) {
                long sx = (flipMode & 1) ? (srcX + width - 1 - col) : (srcX + col);
                unsigned char srcPx = srcRow[sx];
                if ((long)(unsigned)srcPx == transColor) continue;

                unsigned char idx = (unsigned char)(srcPx + palOff);
                unsigned char* color = (unsigned char*)&palette[idx];
                unsigned short* dst = &dstRow[destX + col];

                if (alpha == 1000 && !additive) {
                    *dst = packRGB555(color[2], color[1], color[0]);
                } else if (!additive) {
                    blendPixel16(dst, color[2], color[1], color[0], alpha, 0);
                } else {
                    blendPixel16(dst, color[2], color[1], color[0], alpha, 4);
                }
            }
        }
        return 1;
    }

    // ---- 24bpp source -> 24bpp dest ----
    if (srcBpp == 24 && dstBpp == 24) {
        for (long row = 0; row < height; row++) {
            long sy = (flipMode & 2) ? srcY + row : (srcH - srcY - row - 1);
            long dy = dstH - destY - row - 1;

            unsigned char* srcRow = srcDIB->bitmap + sy * srcStride;
            unsigned char* dstRow = self->bitmap + dy * dstStride;

            for (long col = 0; col < width; col++) {
                long sx = (flipMode & 1) ? (srcX + width - 1 - col) : (srcX + col);
                unsigned char* src = srcRow + sx * 3;
                unsigned char* dst = dstRow + (destX + col) * 3;

                // For 24bpp, transColor check is on the full pixel
                // In practice this path is rarely used with transparency

                if (alpha == 1000 && !additive) {
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                } else {
                    blendPixel24(dst, src[2], src[1], src[0], alpha, additive ? 4 : 0);
                }
            }
        }
        return 1;
    }

    // ---- 1bpp/4bpp source -> higher bpp dest ----
    // These are less common paths. For now handle the basic case.
    if ((srcBpp == 1 || srcBpp == 4) && dstBpp >= 8) {
        RGBQUAD* palette = srcDIB->palette;

        int pixelsPerByte = (srcBpp == 1) ? 8 : 2;
        int bitsPerPixel = srcBpp;
        unsigned char mask = (srcBpp == 1) ? 0x01 : 0x0F;

        for (long row = 0; row < height; row++) {
            long sy = (flipMode & 2) ? srcY + row : (srcH - srcY - row - 1);
            long dy = dstH - destY - row - 1;

            unsigned char* srcRow = srcDIB->bitmap + sy * srcStride;
            unsigned char* dstRow = self->bitmap + dy * dstStride;

            for (long col = 0; col < width; col++) {
                long sx = (flipMode & 1) ? (srcX + width - 1 - col) : (srcX + col);
                int byteIdx = sx / pixelsPerByte;
                int bitIdx = (pixelsPerByte - 1 - (sx % pixelsPerByte)) * bitsPerPixel;
                unsigned char srcPx = (srcRow[byteIdx] >> bitIdx) & mask;

                if ((long)(unsigned)srcPx == transColor) continue;

                if (dstBpp == 8) {
                    dstRow[destX + col] = (unsigned char)(srcPx + palOff);
                } else if (dstBpp == 24 && palette) {
                    unsigned char* color = (unsigned char*)&palette[srcPx];
                    unsigned char* dst = dstRow + (destX + col) * 3;
                    if (alpha == 1000 && !additive) {
                        dst[0] = color[0]; dst[1] = color[1]; dst[2] = color[2];
                    } else {
                        blendPixel24(dst, color[2], color[1], color[0], alpha, additive ? 4 : 0);
                    }
                } else if (dstBpp == 16 && palette) {
                    unsigned char* color = (unsigned char*)&palette[srcPx];
                    unsigned short* dst = (unsigned short*)(dstRow + (destX + col) * 2);
                    if (alpha == 1000 && !additive) {
                        *dst = packRGB555(color[2], color[1], color[0]);
                    } else {
                        blendPixel16(dst, color[2], color[1], color[0], alpha, additive ? 4 : 0);
                    }
                }
            }
        }
        return 1;
    }

    return 0;
}

/**
 * RKC_DIB::TransferToDIBEx - Extended blit with flipping/mirroring (8 stack params version)
 * USED BY: o_RKC_UPDIB.dll
 *
 * Convenience wrapper that uses the full source dimensions.
 * Expands to the 12-arg version with srcX=0, srcY=0, width=srcW, height=srcH.
 */
extern "C" int __thiscall RKC_DIB_TransferToDIBEx_8args(
    RKC_DIB* self, long destX, long destY,
    RKC_DIB* srcDIB, long palOff, long transColor, long alpha, long flags,
    void* hispeed) {
    if (!srcDIB || !srcDIB->bitmapInfo) return 0;
    long srcW = srcDIB->bitmapInfo->biWidth;
    long srcH = srcDIB->bitmapInfo->biHeight;
    return RKC_DIB_TransferToDIBEx_11args(self, destX, destY, srcW, srcH,
        srcDIB, 0, 0, palOff, transColor, alpha, flags, hispeed);
}

/**
 * RKC_DIB::WriteFile - Write DIB to BMP file
 * NOT REFERENCED - stub only, not imported by any module
 */
extern "C" int __thiscall RKC_DIB_WriteFile(RKC_DIB* self, const char* filename) {
    return 0;
}

/**
 * RKC_DIBHISPEEDMODE::constructor - Build blending lookup tables
 * NOT REFERENCED - stub only, not imported by any module
 *
 * The full constructor builds ~290KB of pre-calculated tables.
 * Since nobody imports this, we just zero-init.
 */
extern "C" void* __thiscall RKC_DIBHISPEEDMODE_constructor(void* self) {
    memset(self, 0, DIBHISPEEDMODE_SIZE);
    return self;
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

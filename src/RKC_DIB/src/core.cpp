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

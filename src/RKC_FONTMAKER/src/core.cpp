/**
 * RKC_FONTMAKER - Font bitmap generation
 * 
 * Creates font bitmaps for text rendering. Generates normal and double-size fonts.
 * USED BY: ShadowFlare.exe (constructor, destructor, Initialize, SaveNJPFile)
 * 
 * Class layout (from disassembly):
 * +0x00: m_fontWidth (int32_t)
 * +0x04: m_fontHeight (int32_t)
 * +0x08: m_font (HFONT)
 * +0x0c: m_bitmapInfoNormal (BITMAPINFO* - GlobalAlloc)
 * +0x10: m_normalDIBitmap (uint8_t* - from CreateDIBSection, don't free)
 * +0x14: m_normalDDBitmap (HBITMAP - DeleteObject)
 * +0x18: m_strideNormal (int32_t)
 * +0x1c: m_bitmapInfoDouble (BITMAPINFO* - GlobalAlloc)
 * +0x20: m_doubleDIBitmap (uint8_t* - from CreateDIBSection, don't free)
 * +0x24: m_doubleDDBitmap (HBITMAP - DeleteObject)
 * +0x28: m_strideDouble (int32_t)
 */

#include <iostream>
#include <windows.h>
#include <cstdint>
#include "../../utils.h"

class RKC_FONTMAKER
{
public:
    int32_t m_fontWidth;        // +0x00
    int32_t m_fontHeight;       // +0x04
    HFONT m_font;               // +0x08
    BITMAPINFO* m_bitmapInfoNormal;  // +0x0c (GlobalAlloc)
    uint8_t* m_normalDIBitmap;  // +0x10 (from CreateDIBSection, DON'T free)
    HBITMAP m_normalDDBitmap;   // +0x14 (DeleteObject)
    int32_t m_strideNormal;     // +0x18
    BITMAPINFO* m_bitmapInfoDouble;  // +0x1c (GlobalAlloc)
    uint8_t* m_doubleDIBitmap;  // +0x20 (from CreateDIBSection, DON'T free)
    HBITMAP m_doubleDDBitmap;   // +0x24 (DeleteObject)
    int32_t m_strideDouble;     // +0x28
};

extern "C"
{
    /**
     * Constructor - initialize all members to zero/null
     * USED BY: ShadowFlare.exe
     */
    void __thiscall RKC_FONTMAKER_constructor(RKC_FONTMAKER* self)
    {
        self->m_fontWidth = 0;
        self->m_fontHeight = 0;
        self->m_font = nullptr;
        self->m_bitmapInfoNormal = nullptr;
        self->m_normalDIBitmap = nullptr;
        self->m_normalDDBitmap = nullptr;
        self->m_strideNormal = 0;
        self->m_bitmapInfoDouble = nullptr;
        self->m_doubleDIBitmap = nullptr;
        self->m_doubleDDBitmap = nullptr;
        self->m_strideDouble = 0;
    }

    /**
     * Release all font resources
     * Order from disassembly:
     * 1. DeleteObject(normalDDBitmap) at +0x14
     * 2. DeleteObject(doubleDDBitmap) at +0x24
     * 3. GlobalFree(bitmapInfoNormal) at +0x0c
     * 4. GlobalFree(bitmapInfoDouble) at +0x1c
     * 5. DeleteObject(font) at +0x08
     * Note: DIBitmap pointers are NOT freed - they're part of the DIBSection
     */
    void __thiscall Release(RKC_FONTMAKER* self)
    {
        // Delete normal DDB bitmap
        if (self->m_normalDDBitmap != nullptr)
        {
            DeleteObject(self->m_normalDDBitmap);
            self->m_normalDDBitmap = nullptr;
        }

        // Delete double DDB bitmap
        if (self->m_doubleDDBitmap != nullptr)
        {
            DeleteObject(self->m_doubleDDBitmap);
            self->m_doubleDDBitmap = nullptr;
        }

        // Free normal BITMAPINFO (allocated with GlobalAlloc)
        if (self->m_bitmapInfoNormal != nullptr)
        {
            GlobalFree(self->m_bitmapInfoNormal);
            self->m_bitmapInfoNormal = nullptr;
        }

        // Free double BITMAPINFO (allocated with GlobalAlloc)
        if (self->m_bitmapInfoDouble != nullptr)
        {
            GlobalFree(self->m_bitmapInfoDouble);
            self->m_bitmapInfoDouble = nullptr;
        }

        // Delete font object
        if (self->m_font != nullptr)
        {
            DeleteObject(self->m_font);
            self->m_font = nullptr;
        }

        // DIBitmap pointers are NOT freed - memory is freed when HBITMAP is deleted
        self->m_normalDIBitmap = nullptr;
        self->m_doubleDIBitmap = nullptr;
    }

    /**
     * Destructor - just calls Release
     * USED BY: ShadowFlare.exe
     */
    void __thiscall RKC_FONTMAKER_deconstructor(RKC_FONTMAKER* self)
    {
        Release(self);
    }

    /**
     * Create device independent bitmaps for font rendering
     * Creates both normal-size and double-size DIB sections with grayscale palette.
     * 
     * From disassembly (0x100012b0):
     * 1. Allocate BITMAPINFO for normal (0x428 bytes with GlobalAlloc(GPTR, 0x428))
     * 2. Allocate BITMAPINFO for double (0x428 bytes)
     * 3. Set up BITMAPINFOHEADER for normal: width x height, 8bpp
     * 4. Set up BITMAPINFOHEADER for double: width*2 x height, 8bpp
     * 5. Calculate strides: ((width + 3) & ~3), ((width*2 + 3) & ~3)
     * 6. Create DIB sections with CreateDIBSection
     * 7. Set up grayscale palette (0=black, 255=white)
     */
    int __thiscall CreateDIB(RKC_FONTMAKER* self, HDC hdc)
    {
        // Allocate BITMAPINFO structures (0x428 = 1064 bytes = sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD))
        // GPTR = GMEM_FIXED | GMEM_ZEROINIT = 0x40
        self->m_bitmapInfoNormal = (BITMAPINFO*)GlobalAlloc(GPTR, 0x428);
        if (self->m_bitmapInfoNormal == nullptr)
            return 0;

        self->m_bitmapInfoDouble = (BITMAPINFO*)GlobalAlloc(GPTR, 0x428);
        if (self->m_bitmapInfoDouble == nullptr)
            return 0;

        // Set up BITMAPINFOHEADER for normal size
        BITMAPINFOHEADER* biNormal = &self->m_bitmapInfoNormal->bmiHeader;
        biNormal->biSize = sizeof(BITMAPINFOHEADER);  // 0x28 = 40
        biNormal->biWidth = self->m_fontWidth;
        biNormal->biHeight = self->m_fontHeight;
        biNormal->biPlanes = 1;
        biNormal->biBitCount = 8;
        biNormal->biCompression = BI_RGB;  // 0
        biNormal->biSizeImage = 0;
        biNormal->biXPelsPerMeter = 0;
        biNormal->biYPelsPerMeter = 0;
        biNormal->biClrUsed = 0;
        biNormal->biClrImportant = 0;

        // Calculate stride for normal: ((width + 3) / 4) * 4
        self->m_strideNormal = ((self->m_fontWidth + 3) >> 2) << 2;

        // Set up BITMAPINFOHEADER for double size (2x width)
        BITMAPINFOHEADER* biDouble = &self->m_bitmapInfoDouble->bmiHeader;
        biDouble->biSize = sizeof(BITMAPINFOHEADER);
        biDouble->biWidth = self->m_fontWidth * 2;
        biDouble->biHeight = self->m_fontHeight;
        biDouble->biPlanes = 1;
        biDouble->biBitCount = 8;
        biDouble->biCompression = BI_RGB;
        biDouble->biSizeImage = 0;
        biDouble->biXPelsPerMeter = 0;
        biDouble->biYPelsPerMeter = 0;
        biDouble->biClrUsed = 0;
        biDouble->biClrImportant = 0;

        // Calculate stride for double: (((width*2) + 3) / 4) * 4
        self->m_strideDouble = (((self->m_fontWidth * 2) + 3) >> 2) << 2;

        // Create DIB section for normal size
        self->m_normalDDBitmap = CreateDIBSection(
            hdc,
            self->m_bitmapInfoNormal,
            DIB_RGB_COLORS,
            (void**)&self->m_normalDIBitmap,
            nullptr,
            0
        );
        if (self->m_normalDDBitmap == nullptr)
            return 0;

        // Create DIB section for double size
        self->m_doubleDDBitmap = CreateDIBSection(
            hdc,
            self->m_bitmapInfoDouble,
            DIB_RGB_COLORS,
            (void**)&self->m_doubleDIBitmap,
            nullptr,
            0
        );
        if (self->m_doubleDDBitmap == nullptr)
            return 0;

        // Set up grayscale palette for both DIBs
        // From disasm: palette has 256 entries, 0=black (0,0,0), 255=white (255,255,255)
        // Also sets some specific entries: [1] = (64,64,64), [2] = (128,128,128)
        RGBQUAD palette[256];
        memset(palette, 0, sizeof(palette));

        // Entry 0: black (0,0,0) - already zero from memset
        // Entry 1: dark gray (64,64,64) 
        palette[1].rgbRed = 64;
        palette[1].rgbGreen = 64;
        palette[1].rgbBlue = 64;
        // Entry 2: medium gray (128,128,128)
        palette[2].rgbRed = 128;
        palette[2].rgbGreen = 128;
        palette[2].rgbBlue = 128;
        // Entry 255: white (255,255,255)
        palette[255].rgbRed = 255;
        palette[255].rgbGreen = 255;
        palette[255].rgbBlue = 255;
        // Other entries remain black (0,0,0)

        // Apply palette to both DIBs via a compatible DC
        HDC memDC = CreateCompatibleDC(hdc);
        if (memDC)
        {
            // Set palette for normal DIB
            HGDIOBJ oldBitmap = SelectObject(memDC, self->m_normalDDBitmap);
            SetDIBColorTable(memDC, 0, 256, palette);
            SelectObject(memDC, self->m_doubleDDBitmap);
            SetDIBColorTable(memDC, 0, 256, palette);
            SelectObject(memDC, oldBitmap);
            DeleteDC(memDC);
        }

        return 1;
    }

    /**
     * Draw double-size font character
     * Args: hdc = device context, charCode = character to draw (SJIS or ASCII)
     * NOT REFERENCED - not directly imported, but used internally by SaveNJPFile
     * 
     * Renders a single character to m_doubleDIBitmap using GDI TextOut.
     * Will be implemented when SaveNJPFile is implemented.
     */
    bool __thiscall DrawDoubleFont(RKC_FONTMAKER* self, HDC* hdc, unsigned char* charCode)
    {
        bool result = CallFunctionInDLL<bool>(
            "o_RKC_FONTMAKER.dll",
            "?DrawDoubleFont@RKC_FONTMAKER@@QAEHPAUHDC__@@PAE@Z",
            self, hdc, charCode);
        return result;
    }

    /**
     * Draw normal-size font character
     * Args: hdc = device context, charCode = character to draw (SJIS or ASCII)
     * NOT REFERENCED - not directly imported, but used internally by SaveNJPFile
     * 
     * Renders a single character to m_normalDIBitmap using GDI TextOut.
     * Will be implemented when SaveNJPFile is implemented.
     */
    bool __thiscall DrawNormalFont(RKC_FONTMAKER* self, HDC* hdc, unsigned char* charCode)
    {
        bool result = CallFunctionInDLL<bool>(
            "o_RKC_FONTMAKER.dll",
            "?DrawNormalFont@RKC_FONTMAKER@@QAEHPAUHDC__@@E@Z",
            self, hdc, charCode);
        return result;
    }

    /**
     * Get double-size device dependent bitmap
     * NOT REFERENCED - not imported by any module
     */
    HBITMAP __thiscall GetDoubleDDBitmap(RKC_FONTMAKER* self)
    {
        return self->m_doubleDDBitmap;
    }

    /**
     * Get double-size device independent bitmap
     * NOT REFERENCED - not imported by any module
     */
    uint8_t* __thiscall GetDoubleDIBitmap(RKC_FONTMAKER* self)
    {
        return self->m_doubleDIBitmap;
    }

    /**
     * Get normal-size device dependent bitmap
     * NOT REFERENCED - not imported by any module
     */
    HBITMAP __thiscall GetNormalDDBitmap(RKC_FONTMAKER* self)
    {
        return self->m_normalDDBitmap;
    }

    /**
     * Get normal-size device independent bitmap
     * NOT REFERENCED - not imported by any module
     */
    uint8_t* __thiscall GetNormalDIBitmap(RKC_FONTMAKER* self)
    {
        return self->m_normalDIBitmap;
    }

    /**
     * Initialize font with given parameters
     * Args: hdc = device context, width = font width, height = font height, fontName = font face name
     * USED BY: ShadowFlare.exe
     * 
     * From disassembly:
     * 1. Call Release() to clean up existing resources
     * 2. Store width at [esi], height at [esi+4]
     * 3. Create font with CreateFontA(height, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH, fontName)
     * 4. Store font handle at [esi+8]
     * 5. Call CreateDIB(hdc)
     * 6. Return 1 on success, 0 on failure (negated result of CreateDIB)
     */
    int32_t __thiscall Initialize(RKC_FONTMAKER* self, HDC hdc, long width, long height, char* fontName)
    {
        // Clean up any existing resources
        Release(self);

        // Store dimensions
        self->m_fontWidth = width;
        self->m_fontHeight = height;

        // Create font with fixed pitch for monospace rendering
        // Parameters from disassembly: height, 0, 0, 0, 400(FW_NORMAL), 0, 0, 0, 1(DEFAULT_CHARSET), 0, 0, 0, 1(FIXED_PITCH), fontName
        self->m_font = CreateFontA(
            height,           // nHeight
            0,                // nWidth (0 = auto)
            0,                // nEscapement
            0,                // nOrientation  
            FW_NORMAL,        // fnWeight = 400
            FALSE,            // fdwItalic
            FALSE,            // fdwUnderline
            FALSE,            // fdwStrikeOut
            DEFAULT_CHARSET,  // fdwCharSet = 1
            OUT_DEFAULT_PRECIS,  // fdwOutputPrecision = 0
            CLIP_DEFAULT_PRECIS, // fdwClipPrecision = 0
            DEFAULT_QUALITY,  // fdwQuality = 0
            FIXED_PITCH,      // fdwPitchAndFamily = 1
            fontName          // lpszFace
        );

        // Create DIB sections for normal and double-size fonts
        int result = CreateDIB(self, hdc);

        // Return 1 on success, 0 on failure
        // Original does: neg eax; sbb eax, eax; which converts non-zero to -1 and zero to 0, then increments
        return result ? 1 : 0;
    }

    /**
     * Assignment operator
     * NOT REFERENCED - not imported by any module
     * Note: Forwards to original DLL
     */
    RKC_FONTMAKER* __thiscall EqualsOperator(RKC_FONTMAKER* self, const RKC_FONTMAKER& other)
    {
        RKC_FONTMAKER* result = CallFunctionInDLL<RKC_FONTMAKER*>(
            "o_RKC_FONTMAKER.dll",
            "??4RKC_FONTMAKER@@QAEAAV0@ABV0@@Z",
            self, other);
        return result;
    }

    /**
     * Save font bitmap to NJP file
     * Args: hdc = device context, filename = output file path
     * USED BY: ShadowFlare.exe
     * 
     * DEFERRED IMPLEMENTATION:
     * This function is too complex to implement now. From decompilation (0x100014e0),
     * it has dependencies on:
     * - RKC_FILE (Create, Write, Close) - already implemented
     * - RKC_DIB (constructor, destructor, Create, TransferToDIB, GetAlignWidth) - NOT implemented
     * - DrawNormalFont() - renders each character to the normal DIB
     * - DrawDoubleFont() - renders each character to the double-size DIB  
     * - RK_LzEncodeMemoryToMemory() - already implemented
     * 
     * The function generates NJP font sprite files by:
     * 1. Creating RKC_DIB instances for normal (16x width) and double (32x width) fonts
     * 2. Looping through character ranges (0x00-0x0F for ASCII, 0x20-0x9F for extended)
     * 3. Drawing each character using DrawNormalFont/DrawDoubleFont
     * 4. Compressing the bitmap data with LZSS
     * 5. Writing NJP format with "NJudgeUniPat002" header
     * 
     * To implement this, we need to first implement RKC_DIB or at minimum the subset
     * of functions used here. For now, forward to original DLL.
     */
    int32_t __thiscall SaveNJPFile(RKC_FONTMAKER* self, HDC* hdc, char* filename)
    {
        int32_t result = CallFunctionInDLL<int32_t>(
            "o_RKC_FONTMAKER.dll",
            "?SaveNJPFile@RKC_FONTMAKER@@QAEHPAUHDC__@@PAD@Z",
            self, hdc, filename);
        return result;
    }

}
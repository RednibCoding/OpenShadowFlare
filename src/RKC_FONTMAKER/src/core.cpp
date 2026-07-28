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

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <vector>
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
        self->m_font = nullptr;
        self->m_bitmapInfoNormal = nullptr;
        self->m_normalDIBitmap = nullptr;
        self->m_normalDDBitmap = nullptr;
        self->m_bitmapInfoDouble = nullptr;
        self->m_doubleDIBitmap = nullptr;
        self->m_doubleDDBitmap = nullptr;
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

        palette[0].rgbRed = 64;
        palette[0].rgbGreen = 128;
        palette[0].rgbBlue = 64;
        palette[1].rgbRed = 255;
        palette[1].rgbGreen = 255;
        palette[1].rgbBlue = 255;

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
    bool __thiscall DrawDoubleFont(
        RKC_FONTMAKER* self, HDC hdc, unsigned char* charCode)
    {
        if (!charCode)
            return false;
        const unsigned char lead = charCode[0];
        const unsigned char trail = charCode[1];
        const bool validLead =
            (lead > 0x80 && lead < 0xa0)
            || (lead > 0xdf && lead < 0xfd);
        const bool validTrail =
            (trail > 0x3f && trail < 0x7f)
            || (trail > 0x7f && trail < 0xfd);
        if (!validLead || !validTrail)
            return false;

        HDC memory = CreateCompatibleDC(hdc);
        if (!memory)
            return false;
        SelectObject(memory, self->m_doubleDDBitmap);
        SelectObject(memory, self->m_font);
        SetBkMode(memory, TRANSPARENT);
        std::memset(
            self->m_doubleDIBitmap, 0,
            static_cast<std::size_t>(self->m_strideDouble)
                * self->m_fontHeight);
        GdiFlush();
        TextOutA(
            memory, 0, 0, reinterpret_cast<const char*>(charCode), 2);
        GdiFlush();
        for (int y = 0; y < self->m_fontHeight; ++y) {
            unsigned char* row =
                self->m_doubleDIBitmap + self->m_strideDouble * y;
            for (int x = 0; x < self->m_fontWidth * 2; ++x)
                if (row[x] != 0 && row[x] != 0x7b)
                    row[x] = 1;
        }
        DeleteDC(memory);
        return true;
    }

    /**
     * Draw normal-size font character
     * Args: hdc = device context, charCode = character to draw (SJIS or ASCII)
     * NOT REFERENCED - not directly imported, but used internally by SaveNJPFile
     * 
     * Renders a single character to m_normalDIBitmap using GDI TextOut.
     * Will be implemented when SaveNJPFile is implemented.
     */
    bool __thiscall DrawNormalFont(
        RKC_FONTMAKER* self, HDC hdc, unsigned char charCode)
    {
        HDC memory = CreateCompatibleDC(hdc);
        if (!memory)
            return false;
        SelectObject(memory, self->m_normalDDBitmap);
        SelectObject(memory, self->m_font);
        SetBkMode(memory, TRANSPARENT);
        std::memset(
            self->m_normalDIBitmap, 0,
            static_cast<std::size_t>(self->m_strideNormal)
                * self->m_fontHeight);
        GdiFlush();
        const char character = static_cast<char>(charCode);
        TextOutA(memory, 0, 0, &character, 1);
        GdiFlush();
        for (int y = 0; y < self->m_fontHeight; ++y) {
            unsigned char* row =
                self->m_normalDIBitmap + self->m_strideNormal * y;
            for (int x = 0; x < self->m_fontWidth; ++x)
                if (row[x] != 0 && row[x] != 0x7b)
                    row[x] = 1;
        }
        DeleteDC(memory);
        return true;
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
        std::memcpy(self, &other, sizeof(*self));
        return self;
    }

    int32_t __thiscall SaveNJPFile(
        RKC_FONTMAKER* self, HDC hdc, char* filename)
    {
        if (!self || !hdc || !filename || self->m_fontWidth <= 0
            || self->m_fontHeight <= 0 || !self->m_normalDIBitmap
            || !self->m_doubleDIBitmap)
            return 0;

        HANDLE file = CreateFileA(
            filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return 0;
        auto write = [&](const void* memory, DWORD size) {
            DWORD amount = 0;
            return WriteFile(file, memory, size, &amount, nullptr)
                && amount == size;
        };
        auto writeLong = [&](LONG value) {
            return write(&value, sizeof(value));
        };
        auto copyGlyph = [](
            std::vector<unsigned char>& atlas, int atlasWidth,
            int atlasHeight, const unsigned char* glyph, int glyphStride,
            int glyphWidth, int glyphHeight, int destinationX,
            int destinationY) {
            for (int y = 0; y < glyphHeight; ++y) {
                unsigned char* destination =
                    atlas.data()
                    + (atlasHeight - destinationY - y - 1) * atlasWidth
                    + destinationX;
                const unsigned char* source =
                    glyph + (glyphHeight - y - 1) * glyphStride;
                std::memcpy(destination, source, glyphWidth);
            }
        };

        const char header[16] = "NJudgeUniPat002";
        bool valid = write(header, sizeof(header)) && writeLong(65);

        const int normalWidth = self->m_fontWidth * 16;
        const int atlasHeight = self->m_fontHeight * 16;
        std::vector<unsigned char> atlas(
            static_cast<std::size_t>(normalWidth) * atlasHeight);
        for (int row = 0; valid && row < 16; ++row) {
            for (int column = 0; valid && column < 16; ++column) {
                const unsigned char character =
                    static_cast<unsigned char>(row * 16 + column);
                if (DrawNormalFont(self, hdc, character)) {
                    copyGlyph(
                        atlas, normalWidth, atlasHeight,
                        self->m_normalDIBitmap, self->m_strideNormal,
                        self->m_fontWidth, self->m_fontHeight,
                        column * self->m_fontWidth,
                        row * self->m_fontHeight);
                }
            }
        }
        valid = valid && writeLong(8) && writeLong(normalWidth)
            && writeLong(atlasHeight) && writeLong(0)
            && write(atlas.data(), static_cast<DWORD>(atlas.size()));

        using Encode = int (__cdecl*)(
            const void*, int, void*, int, void*);
        HMODULE rkModule = LoadLibraryA("RK_FUNCTION.dll");
        Encode encode = rkModule
            ? reinterpret_cast<Encode>(
                GetProcAddress(rkModule, "RK_LzEncodeMemoryToMemory"))
            : nullptr;

        const int doubleWidth = self->m_fontWidth * 32;
        atlas.assign(
            static_cast<std::size_t>(doubleWidth) * atlasHeight, 0);
        for (int rangeEnd = 0x9f; valid && rangeEnd < 0x15f;
            rangeEnd += 0x60) {
            for (int lead = rangeEnd - 0x1f; valid && lead <= rangeEnd;
                 ++lead) {
                for (int row = 0; row < 16; ++row) {
                    for (int column = 0; column < 16; ++column) {
                        unsigned char character[2] = {
                            static_cast<unsigned char>(lead),
                            static_cast<unsigned char>(row * 16 + column)
                        };
                        if (DrawDoubleFont(self, hdc, character)) {
                            copyGlyph(
                                atlas, doubleWidth, atlasHeight,
                                self->m_doubleDIBitmap, self->m_strideDouble,
                                self->m_fontWidth * 2, self->m_fontHeight,
                                column * self->m_fontWidth * 2,
                                row * self->m_fontHeight);
                        }
                    }
                }

                valid = writeLong(8) && writeLong(doubleWidth)
                    && writeLong(atlasHeight);
                std::vector<unsigned char> encoded(
                    atlas.size() + (atlas.size() + 7) / 8 + 32);
                unsigned char compressionHeader[16]{};
                const int encodedResult = encode
                    ? encode(
                        atlas.data(), static_cast<int>(atlas.size()),
                        encoded.data(), static_cast<int>(encoded.size()),
                        compressionHeader)
                    : 0;
                if (encodedResult == 1) {
                    const LONG compressedSize =
                        *reinterpret_cast<LONG*>(compressionHeader + 12);
                    valid = valid && writeLong(1)
                        && write(
                            encoded.data(),
                            static_cast<DWORD>(compressedSize + 16));
                } else {
                    valid = valid && writeLong(0)
                        && write(
                            atlas.data(),
                            static_cast<DWORD>(atlas.size()));
                }
            }
        }
        if (rkModule)
            FreeLibrary(rkModule);

        valid = valid && writeLong(65);
        for (LONG index = 0; valid && index < 65; ++index) {
            RECT rectangle{
                0, 0, index == 0 ? normalWidth : doubleWidth, atlasHeight
            };
            const LONG one = 1;
            const LONG zero = 0;
            const LONG duration = 1000;
            POINT position{0, 0};
            valid = writeLong(one)
                && write(&rectangle, sizeof(rectangle))
                && writeLong(zero) && writeLong(zero) && writeLong(index)
                && write(&position, sizeof(position))
                && writeLong(zero)
                && writeLong(duration) && writeLong(duration);
        }

        valid = valid && writeLong(1);
        RGBQUAD palette[256]{};
        palette[1].rgbBlue = 255;
        palette[1].rgbGreen = 255;
        palette[1].rgbRed = 255;
        valid = valid && write(palette, sizeof(palette));
        CloseHandle(file);
        return valid ? 1 : 0;
    }

}

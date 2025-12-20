#include <iostream>
#include <windows.h>
#include <cstdint>
#include "../../utils.h"

class RKC_FONTMAKER
{
public:
    int32_t m_fontWidth;
    int32_t m_fontHeight;
    HFONT m_font;
    BITMAPINFO* m_bitmapInfoNormal;
    uint8_t* m_normalDIBitmap;
    HBITMAP m_normalDDBitmap;
    int32_t m_fontSizeNormal;
    BITMAPINFO* m_bitmapInfoDouble;
    uint8_t* m_doubleDIBitmap;
    HBITMAP m_doubleDDBitmap;
    int32_t m_fontSizeDouble;
};

extern "C"
{
    void __thiscall RKC_FONTMAKER_constructor(RKC_FONTMAKER* self)
    {
        self->m_fontWidth = 0;
        self->m_fontHeight = 0;
        self->m_font = nullptr;
        self->m_bitmapInfoNormal = nullptr;
        self->m_normalDIBitmap = nullptr;
        self->m_normalDDBitmap = nullptr;
        self->m_fontSizeNormal = 0;
        self->m_bitmapInfoDouble = nullptr;
        self->m_doubleDIBitmap = nullptr;
        self->m_doubleDDBitmap = nullptr;
        self->m_fontSizeDouble = 0;
    }

    void __thiscall RKC_FONTMAKER_deconstructor(RKC_FONTMAKER* self)
    {
        // TODO: Implement
        CallFunctionInDLL<VOID>("o_RKC_FONTMAKER.dll",
            "??1RKC_FONTMAKER@@QAE@XZ",
            self);
        return;
    }

    bool __thiscall CreateDIB(RKC_FONTMAKER* self, HDC* hdc)
    {
        // TODO: Implement
        bool result = CallFunctionInDLL<bool>(
            "o_RKC_FONTMAKER.dll",
            "?CreateDIB@RKC_FONTMAKER@@QAEHPAUHDC__@@@Z",
            self, hdc);
        return result;
    }

    bool __thiscall DrawDoubleFont(RKC_FONTMAKER* self, HDC* hdc, unsigned char* ucharParam)
    {
        // TODO: Implement
        bool result = CallFunctionInDLL<bool>(
            "o_RKC_FONTMAKER.dll",
            "?DrawDoubleFont@RKC_FONTMAKER@@QAEHPAUHDC__@@PAE@Z",
            self, hdc, ucharParam);
        return result;
    }

    bool __thiscall DrawNormalFont(RKC_FONTMAKER* self, HDC* hdc, unsigned char* ucharParam)
    {
        // TODO: Implement
        bool result = CallFunctionInDLL<bool>(
            "o_RKC_FONTMAKER.dll",
            "?DrawNormalFont@RKC_FONTMAKER@@QAEHPAUHDC__@@E@Z",
            self, hdc, ucharParam);
        return result;
    }

    HBITMAP __thiscall GetDoubleDDBitmap(RKC_FONTMAKER* self)
    {
        return self->m_doubleDDBitmap;
    }

    uint8_t* __thiscall GetDoubleDIBitmap(RKC_FONTMAKER* self)
    {
        return self->m_doubleDIBitmap;
    }

    HBITMAP __thiscall GetNormalDDBitmap(RKC_FONTMAKER* self)
    {
        return self->m_normalDDBitmap;
    }

    uint8_t* __thiscall GetNormalDIBitmap(RKC_FONTMAKER* self)
    {
        return self->m_normalDIBitmap;
    }

    int32_t __thiscall Initialize(RKC_FONTMAKER* self, HDC* hdc, long param_2, long param_3, char* param_4)
    {
        // TODO: Implement
        int32_t result = CallFunctionInDLL<int32_t>(
            "o_RKC_FONTMAKER.dll",
            "?Initialize@RKC_FONTMAKER@@QAEHPAUHDC__@@JJPAD@Z",
            self, hdc, param_2, param_3, param_4);
        return result;
    }

    RKC_FONTMAKER* __thiscall EqualsOperator(RKC_FONTMAKER* self, const RKC_FONTMAKER& lhs)
    {
        // TODO: Implement
        RKC_FONTMAKER* result = CallFunctionInDLL<RKC_FONTMAKER*>(
            "o_RKC_FONTMAKER.dll",
            "??4RKC_FONTMAKER@@QAEAAV0@ABV0@@Z",
            self, lhs);
        return result;
    }

    void __thiscall Release(RKC_FONTMAKER* self)
    {
        self->m_fontWidth = 0;
        self->m_fontHeight = 0;
        self->m_fontSizeNormal = 0;
        self->m_fontSizeDouble = 0;

        if (self->m_font != nullptr)
        {
            DeleteObject(self->m_font);
            self->m_font = nullptr;
        }

        if (self->m_bitmapInfoNormal != nullptr)
        {
            delete[] self->m_bitmapInfoNormal;
            self->m_bitmapInfoNormal = nullptr;
        }

        if (self->m_normalDIBitmap != nullptr)
        {
            delete[] self->m_normalDIBitmap;
            self->m_normalDIBitmap = nullptr;
        }

        if (self->m_normalDDBitmap != nullptr)
        {
            DeleteObject(self->m_normalDDBitmap);
            self->m_normalDDBitmap = nullptr;
        }

        if (self->m_bitmapInfoDouble != nullptr)
        {
            delete[] self->m_bitmapInfoDouble;
            self->m_bitmapInfoDouble = nullptr;
        }

        if (self->m_doubleDIBitmap != nullptr)
        {
            delete[] self->m_doubleDIBitmap;
            self->m_doubleDIBitmap = nullptr;
        }

        if (self->m_doubleDDBitmap != nullptr)
        {
            DeleteObject(self->m_doubleDDBitmap);
            self->m_doubleDDBitmap = nullptr;
        }
    }

    int32_t __thiscall SaveNJPFile(RKC_FONTMAKER* self, HDC* hdc, char* param_2)
    {
        // TODO: Implement
        int32_t result = CallFunctionInDLL<int32_t>(
            "o_RKC_FONTMAKER.dll",
            "?SaveNJPFile@RKC_FONTMAKER@@QAEHPAUHDC__@@PAD@Z",
            self, hdc, param_2);
        return result;
    }

}
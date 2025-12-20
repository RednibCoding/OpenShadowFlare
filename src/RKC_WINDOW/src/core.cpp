/**
 * RKC_WINDOW - Window management class
 * 
 * Wrapper for window operations. Most functions still forward to original DLL.
 * USED BY: ShadowFlare.exe (constructor and destructor only)
 */

#include <iostream>
#include <windows.h>
#include <cstdint>
#include "../../utils.h"

class RKC_WINDOW {
public:
    int member1;         // or could be a pointer
    int member2;
    RKC_WINDOW* m_hWnd;     // this[8], seems to be a self reference or pointer to similar type

    // The space between member3 and memberArray is not described.
    // It could be empty, or there could be other members.
    int memberArray[10]; // Starting from offset 0x508

    int member4;         // Starting from offset 0x530
    int member5;
    int member6;
    int member7;
    int member8;
    int member9;
    int member10;
    int member11;
};

// Pointers to the original functions
// These can be deleted once the DLL is entirely rewritten
// typedef VOID(WINAPI* hScrollPtr)(RKC_WINDOW* self, uint32_t param1, uint32_t param2);
// typedef VOID(WINAPI* vScrollPtr)(RKC_WINDOW* self, uint32_t param1, uint32_t param2);
// typedef VOID(WINAPI* resizePtr)(RKC_WINDOW* self);
// typedef VOID(WINAPI* showPtr)(RKC_WINDOW* self, int32_t param_1);

extern "C" {
    /**
     * Constructor - initialize all members to zero
     * USED BY: ShadowFlare.exe
     */
    void __thiscall RKC_Window_constructor(RKC_WINDOW* self)
    {
        self->member1 = 0;
        self->member2 = 0;
        self->m_hWnd = 0x0;
        *(self->memberArray) = 0x0;

        for (int i = 0; i < 10; i++) {
            self->memberArray[i] = 0;
        }

        self->member4 = 0;
        self->member5 = 0;
        self->member6 = 0;
        self->member7 = 0;
        self->member8 = 0;
        self->member9 = 0;
        self->member10 = 0;
        self->member11 = 0;
    }

    /**
     * Destructor - empty
     * USED BY: ShadowFlare.exe
     */
    void __thiscall RKC_Window_deconstructor(RKC_WINDOW* self)
    {
        return;
    }

    /**
     * Assignment operator - shallow copy
     * NOT REFERENCED - not imported by any module
     */
    RKC_WINDOW& __thiscall EqualsOperator(RKC_WINDOW* self, const RKC_WINDOW& other)
    {
        self->m_hWnd = other.m_hWnd;
        return *self;
    }

    /**
     * Horizontal scroll handler
     * NOT REFERENCED - not imported by any module
     * Note: Forwards to original DLL
     */
    void __thiscall HScroll(RKC_WINDOW* self, uint32_t param_1, uint64_t param_2)
    {
        CallFunctionInDLL<void>(
            "o_RKC_WINDOW.dll",
            "?HScroll@RKC_WINDOW@@QAEXIJ",
            self, param_1, param_2);
    }

    /**
     * Vertical scroll handler
     * NOT REFERENCED - not imported by any module
     * Note: Forwards to original DLL
     */
    void __thiscall VScroll(RKC_WINDOW* self, uint32_t param_1, uint64_t param_2)
    {
        CallFunctionInDLL<void>(
            "o_RKC_WINDOW.dll",
            "?VScroll@RKC_WINDOW@@QAEXIJ@Z",
            self, param_1, param_2);
    }

    /**
     * Resize handler
     * NOT REFERENCED - not imported by any module
     * Note: Forwards to original DLL
     */
    void __thiscall Resize(RKC_WINDOW* self)
    {
        CallFunctionInDLL<void>(
            "o_RKC_WINDOW.dll",
            "?Resize@RKC_WINDOW@@QAEXX",
            self);
    }

    /**
     * Show/hide window
     * NOT REFERENCED - not imported by any module
     * Note: Forwards to original DLL
     */
    void __thiscall Show(RKC_WINDOW* self, int param_1)
    {
        CallFunctionInDLL<void>(
            "o_RKC_WINDOW.dll",
            "?Show@RKC_WINDOW@@QAEXH@",
            self, param_1);
    }
}


bool __stdcall DllMain(LPVOID, std::uint32_t call_reason, LPVOID)
{
    return true;
}
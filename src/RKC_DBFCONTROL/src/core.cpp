/**
 * RKC_DBFCONTROL - Double Buffered Frame Control (incremental implementation)
 * 
 * Manages double-buffered rendering with DirectDraw.
 */

#include <windows.h>
#include <ddraw.h>
#include <cstdint>

extern "C" {

// ============================================================================
// RKC_DBF Class Layout (from decompilation)
// ============================================================================
// Offset  Field
// 0x00    DWORD unknown1
// 0x04    DWORD vsBlockNo
// 0x08    RKC_DIB dib (embedded object, constructor called here)
// Total size: ~0x24 bytes (36 bytes, including embedded RKC_DIB)

/**
 * RKC_DBF::GetDIBitmap - Get pointer to embedded DIB object
 * Returns pointer to RKC_DIB at offset 0x08
 * USED BY: o_RKC_DBFCONTROL.dll (internal)
 */
void* __thiscall RKC_DBF_GetDIBitmap(void* self) {
    return (char*)self + 0x08;
}

/**
 * RKC_DBF::GetVSBlockNo - Get VS block number
 * USED BY: ShadowFlare.exe
 */
long __thiscall RKC_DBF_GetVSBlockNo(void* self) {
    return *(long*)((char*)self + 0x04);
}

// ============================================================================
// RKC_DBFCONTROL Class Layout (from decompilation)
// ============================================================================
// Offset  Field
// 0x00    DWORD unknown1
// 0x04    DWORD unknown2
// 0x08    DWORD unknown3
// 0x0c    DWORD currentDBFIndex
// 0x10    DWORD unknown4
// 0x14    DWORD threadDrawFlag
// 0x1c    HWND  windowHandle
// 0x20    RKC_DBF dbf[2] (array of 2 embedded RKC_DBF objects)
// 0x68    DWORD drawCount
// 0x6c    DWORD unknown5
// 0x70    DWORD framePerSecond
// 0x74    ... more fields ...
// 0x84    HANDLE drawThreadHandle
// 0x120   DWORD screenWidth (640)
// 0x124   DWORD screenHeight (480)
// 0x13c   HANDLE mutex
// Total size: ~0x144 bytes

/**
 * RKC_DBFCONTROL::GetCurrentDBF - Get current DBF object
 * Calculation: self + 0x20 + (currentDBFIndex * 0x24)
 * USED BY: ShadowFlare.exe
 */
void* __thiscall RKC_DBFCONTROL_GetCurrentDBF(void* self) {
    char* p = (char*)self;
    long index = *(long*)(p + 0x0c);
    // Each RKC_DBF is 0x24 bytes (36), multiply by 9 and 4 = 0x24
    return p + 0x20 + (index * 0x24);
}

/**
 * RKC_DBFCONTROL::GetDrawCount - Get draw count
 * USED BY: o_RKC_DBFCONTROL.dll (internal)
 */
long __thiscall RKC_DBFCONTROL_GetDrawCount(void* self) {
    return *(long*)((char*)self + 0x68);
}

/**
 * RKC_DBFCONTROL::GetFramePerSecond - Get FPS value
 * USED BY: ShadowFlare.exe
 */
long __thiscall RKC_DBFCONTROL_GetFramePerSecond(void* self) {
    return *(long*)((char*)self + 0x70);
}

/**
 * RKC_DBFCONTROL::GetWindowHandle - Get window handle
 * NOT REFERENCED - stub only
 */
HWND __thiscall RKC_DBFCONTROL_GetWindowHandle(void* self) {
    return *(HWND*)((char*)self + 0x1c);
}

/**
 * RKC_DBFCONTROL::GetDrawThreadHandle - Get draw thread handle
 * NOT REFERENCED - stub only
 */
void* __thiscall RKC_DBFCONTROL_GetDrawThreadHandle(void* self) {
    return *(void**)((char*)self + 0x84);
}

/**
 * RKC_DBFCONTROL::GetThreadDrawFlag - Get thread draw flag
 * Note: This function is more complex in original (waits on mutex)
 * but this simple version returns the flag value directly
 * NOT REFERENCED - stub only
 */
int __thiscall RKC_DBFCONTROL_GetThreadDrawFlag(void* self) {
    return *(int*)((char*)self + 0x14);
}

/**
 * RKC_DBFCONTROL::GetStyle - Get window style based on DBF index
 * Original logic:
 *   if (arg == -1) arg = [ecx+0x6c]
 *   dec eax; neg eax; sbb eax,eax -> eax = (arg==0) ? -1 : 0
 *   and eax, 0x7f360000
 *   add eax, 0xca0000
 * If arg == 0: returns 0x7f360000 + 0xca0000 = 0x80000000 (WS_POPUP)
 * If arg != 0: returns 0 + 0xca0000 = 0x00CA0000 (WS_CAPTION|WS_SYSMENU)
 * USED BY: ShadowFlare.exe
 */
long __thiscall RKC_DBFCONTROL_GetStyle(void* self, long arg) {
    char* p = (char*)self;
    if (arg == -1) {
        arg = *(long*)(p + 0x6c);
    }
    if (arg == 0) {
        return (long)0x80000000L;  // WS_POPUP
    } else {
        return (long)0xca0000;  // WS_CAPTION | WS_SYSMENU
    }
}

/**
 * RKC_DBFCONTROL::GetExStyle - Get extended window style based on DBF index
 * Original logic similar to GetStyle but with different constants
 *   and al, 0xf8; add eax, 0x10
 * If arg == 0: returns 0xf8 + 0x10 = 0x108 (but masked as -1 & 0xf8 = 0xf8 + 0x10 = 0x108)
 * Actually: sbb eax,eax gives -1 if arg!=0 after dec+neg, else 0
 * If arg == 0: eax = -1, and -1 & 0xf8 = 0xf8, + 0x10 = 0x108 -> WS_EX_TOPMOST?
 * If arg != 0: eax = 0, and 0 & 0xf8 = 0, + 0x10 = 0x10 -> WS_EX_ACCEPTFILES
 * Wait, let me re-analyze: dec; neg; sbb:
 *   if original == 0: dec=-1, neg=1, sbb(CF=1)=-1 -> wrong
 *   if original == 1: dec=0, neg=0, sbb(CF=0)=0
 *   if original == 2: dec=1, neg=-1, sbb(CF=1)=-1
 * So: arg==1 -> 0, else -> -1
 * USED BY: ShadowFlare.exe
 */
long __thiscall RKC_DBFCONTROL_GetExStyle(void* self, long arg) {
    char* p = (char*)self;
    if (arg == -1) {
        arg = *(long*)(p + 0x6c);
    }
    long result;
    if (arg == 1) {
        result = 0;
    } else {
        result = -1;
    }
    result = result & 0xf8;
    result = result + 0x10;
    return result;
}

// ============================================================================
// STUBS FOR UNUSED FUNCTIONS - NOT IMPORTED BY EXE OR OTHER DLLS
// ============================================================================

// RKC_DBF - NOT USED BY EXE
void* __thiscall RKC_DBF_constructor(void* self) { return self; }
void __thiscall RKC_DBF_destructor(void* self) {}
void* __thiscall RKC_DBF_operatorAssign(void* self, const void* src) { return self; }
void __thiscall RKC_DBF_Draw(void* self) {}
void __thiscall RKC_DBF_Flush(void* self) {}
void __thiscall RKC_DBF_GetClipRect(void* self, void* rect) {}
void __thiscall RKC_DBF_Release(void* self) {}

// RKC_DBFCONTROL - NOT USED
void* __thiscall RKC_DBFCONTROL_operatorAssign(void* self, const void* src) { return self; }
void __thiscall RKC_DBFCONTROL_DisableDraw(void* self) {}
void __thiscall RKC_DBFCONTROL_DrawFunction(void* self) {}
void* __thiscall RKC_DBFCONTROL_Draw(void* self) { return nullptr; }
void __thiscall RKC_DBFCONTROL_EnableDraw(void* self) {}
void __thiscall RKC_DBFCONTROL_FlushDrawCount(void* self) {}
void __thiscall RKC_DBFCONTROL_GetClipRect(void* self, void* rect, long arg) {}
int __thiscall RKC_DBFCONTROL_Redraw(void* self) { return 0; }
void __thiscall RKC_DBFCONTROL_Release(void* self) {}

} // extern "C"

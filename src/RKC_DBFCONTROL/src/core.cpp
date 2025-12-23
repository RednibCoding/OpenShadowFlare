/**
 * RKC_DBFCONTROL - Double Buffered Frame Control (incremental implementation)
 * 
 * Manages double-buffered rendering with DirectDraw.
 * 
 * In windowed mode, we hook Paint() to use OpenGL instead of GDI BitBlt.
 * This allows cross-platform rendering while keeping original DLL logic.
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <ddraw.h>
#include <GL/gl.h>
#include <cstdint>
#include <cstdio>
#include "../../utils.h"

// GL_BGRA_EXT constant (not always defined in MinGW headers)
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

// Global OpenGL context for windowed mode rendering
static HDC g_hdc = nullptr;
static HGLRC g_hglrc = nullptr;
static GLuint g_texture = 0;
static int g_texWidth = 0;
static int g_texHeight = 0;
static bool g_glInitialized = false;

// Debug logging
static FILE* g_logFile = nullptr;

static void DBF_LOG_INIT() {
    g_logFile = fopen("dbfcontrol_log.txt", "w");
    if (g_logFile) {
        fprintf(g_logFile, "=== RKC_DBFCONTROL log started ===\n");
        fflush(g_logFile);
    }
}

static void DBF_LOG_SHUTDOWN() {
    if (g_logFile) {
        fprintf(g_logFile, "=== RKC_DBFCONTROL log ended ===\n");
        fclose(g_logFile);
        g_logFile = nullptr;
    }
}

#define DBF_LOG(fmt, ...) do { \
    if (g_logFile) { fprintf(g_logFile, "[DBF] " fmt "\n", ##__VA_ARGS__); fflush(g_logFile); } \
} while(0)

// Initialize OpenGL context on the given window
static bool InitOpenGL(HWND hwnd, int width, int height) {
    if (g_glInitialized) return true;
    
    DBF_LOG("InitOpenGL: hwnd=%p, %dx%d", hwnd, width, height);
    
    g_hdc = GetDC(hwnd);
    if (!g_hdc) {
        DBF_LOG("ERROR: GetDC failed");
        return false;
    }
    
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;
    
    int format = ChoosePixelFormat(g_hdc, &pfd);
    if (!format) {
        DBF_LOG("ERROR: ChoosePixelFormat failed");
        return false;
    }
    
    if (!SetPixelFormat(g_hdc, format, &pfd)) {
        DBF_LOG("ERROR: SetPixelFormat failed");
        return false;
    }
    
    g_hglrc = wglCreateContext(g_hdc);
    if (!g_hglrc) {
        DBF_LOG("ERROR: wglCreateContext failed");
        return false;
    }
    
    wglMakeCurrent(g_hdc, g_hglrc);
    
    // Set up 2D orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glClearColor(0, 0, 0, 1);
    
    // Create texture for framebuffer
    glGenTextures(1, &g_texture);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, nullptr);
    
    g_texWidth = width;
    g_texHeight = height;
    g_glInitialized = true;
    
    DBF_LOG("OpenGL initialized: %s", (const char*)glGetString(GL_VERSION));
    return true;
}

static void ShutdownOpenGL() {
    if (g_texture) {
        glDeleteTextures(1, &g_texture);
        g_texture = 0;
    }
    if (g_hglrc) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(g_hglrc);
        g_hglrc = nullptr;
    }
    // Note: don't release g_hdc here - the window still owns it
    g_hdc = nullptr;
    g_glInitialized = false;
}

// Present pixels to screen using OpenGL
static void PresentOpenGL(void* pixels, int width, int height) {
    if (!g_glInitialized) return;
    
    wglMakeCurrent(g_hdc, g_hglrc);
    
    // Upload pixels to texture
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
    
    // Draw fullscreen quad
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f((float)width, 0);
    glTexCoord2f(1, 1); glVertex2f((float)width, (float)height);
    glTexCoord2f(0, 1); glVertex2f(0, (float)height);
    glEnd();
    
    SwapBuffers(g_hdc);
}

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

/**
 * RKC_DBF::SetClipRect - Set clip rectangle
 * Copies RECT to offset 0x14 within RKC_DBF
 * USED BY: ShadowFlare.exe
 */
void __thiscall RKC_DBF_SetClipRect(void* self, void* rect) {
    char* p = (char*)self;
    uint32_t* r = (uint32_t*)rect;
    *(uint32_t*)(p + 0x14) = r[0];  // left
    *(uint32_t*)(p + 0x18) = r[1];  // top
    *(uint32_t*)(p + 0x1c) = r[2];  // right
    *(uint32_t*)(p + 0x20) = r[3];  // bottom
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
 * RKC_DBFCONTROL::GetDrawingFlag - Get drawing flag (mutex protected in original)
 * Returns flag at offset 0x04 - indicates if drawing is in progress
 * Original uses mutex but we simplify since no threading.
 * USED BY: ShadowFlare.exe
 */
int __thiscall RKC_DBFCONTROL_GetDrawingFlag(void* self) {
    return *(int*)((char*)self + 0x04);
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

/**
 * RKC_DBFCONTROL::GetPosition - Get window position based on mode
 * Returns CW_USEDEFAULT for windowed mode, (0,0) for fullscreen
 * Note: Returns struct by value via hidden first param (MSVC convention)
 * USED BY: ShadowFlare.exe
 */
void __thiscall RKC_DBFCONTROL_GetPosition(void* self, long* outPoint, long arg) {
    char* p = (char*)self;
    if (arg == -1) {
        arg = *(long*)(p + 0x6c);
    }
    if (arg == 1) {
        // Windowed mode: CW_USEDEFAULT
        outPoint[0] = (long)0x80000000L;
        outPoint[1] = (long)0x80000000L;
    } else {
        // Fullscreen: top-left corner
        outPoint[0] = 0;
        outPoint[1] = 0;
    }
}

/**
 * RKC_DBFCONTROL::GetSurface - Get DirectDraw surface
 * Returns primary surface (arg=0) or back buffer (arg=1)
 * In windowed mode these are NULL since we use GDI/OpenGL.
 * USED BY: ShadowFlare.exe
 */
void* __thiscall RKC_DBFCONTROL_GetSurface(void* self, int arg) {
    char* p = (char*)self;
    // Offset 0x130 = primary surface, 0x12c = back buffer
    if (arg != 0) {
        return *(void**)(p + 0x12c);  // Back buffer
    }
    return *(void**)(p + 0x130);  // Primary surface
}

// ============================================================================
// OpenGL Paint Hook - replaces BitBlt in windowed mode
// ============================================================================

// Forward declaration for DrawEnd (we'll call original)
typedef void (__thiscall *DrawEnd_t)(void* self);
static DrawEnd_t g_origDrawEnd = nullptr;

// Forward declaration for RKC_DIB::TransferToDDB
// The original function copies DIB pixels to a device context.
// Signature: int __thiscall TransferToDDB(RKC_DIB* this, HDC hdc, long x, long y)
// Returns nonzero on success
typedef int (__thiscall *TransferToDDB_t)(void* self, HDC hdc, long x, long y);
static TransferToDDB_t g_origTransferToDDB = nullptr;

// Forward declaration for RKC_DBF::GetDIBitmap  
typedef void* (__thiscall *GetDIBitmap_t)(void* self);
static GetDIBitmap_t g_origGetDIBitmap = nullptr;

// Helper to load function from original DLL
static void* LoadOrigFunc(const char* dll, const char* name) {
    HMODULE mod = GetModuleHandleA(dll);
    if (!mod) mod = LoadLibraryA(dll);
    if (!mod) return nullptr;
    return (void*)GetProcAddress(mod, name);
}

// Initialize function pointers from original DLLs
static void InitOriginalFunctions() {
    static bool initialized = false;
    if (initialized) return;
    
    // Load DrawEnd from original RKC_DBFCONTROL
    g_origDrawEnd = (DrawEnd_t)LoadOrigFunc("o_RKC_DBFCONTROL.dll", 
        "?DrawEnd@RKC_DBFCONTROL@@QAEXXZ");
    
    // Load TransferToDDB from original RKC_DIB
    // Note: return type is int (H in mangling), not void
    g_origTransferToDDB = (TransferToDDB_t)LoadOrigFunc("o_RKC_DIB.dll",
        "?TransferToDDB@RKC_DIB@@QAEHPAUHDC__@@JJ@Z");
    
    // Load GetDIBitmap from original RKC_DBFCONTROL (it's in our dll but we can use ours)
    // Actually we have our own implementation above
    
    DBF_LOG("InitOriginalFunctions: DrawEnd=%p, TransferToDDB=%p", 
            g_origDrawEnd, g_origTransferToDDB);
    
    initialized = true;
}

/**
 * RKC_DBFCONTROL::Paint - Paint the current frame
 * 
 * In fullscreen mode (this+0x6c == 0): Uses DirectDraw surfaces
 * In windowed mode (this+0x6c != 0): Uses GDI BitBlt -> we hook with OpenGL
 * 
 * param_1: HDC of the window (for windowed mode)
 * param_2: some flag
 * 
 * USED BY: ShadowFlare.exe (WM_PAINT handler)
 */
void __thiscall RKC_DBFCONTROL_Paint(void* self, HDC param_1, int param_2) {
    char* p = (char*)self;
    
    InitOriginalFunctions();
    
    // Check state flag at offset 0x00
    if (*(int*)p != 1) {
        if (g_origDrawEnd) g_origDrawEnd(self);
        return;
    }
    
    // Get current DBF index and calculate DBF pointer
    // Original: this + (currentDBFIndex * -0x24) + 0x44
    // Which is: this + 0x44 - (index * 0x24)
    // Wait, the decompilation shows: this + *(int*)(this + 0xc) * -0x24 + 0x44
    // Let me recalculate: index at 0x0c, DBF array starts at 0x20, each DBF is 0x24
    // Standard: dbf = this + 0x20 + (index * 0x24)
    // The weird negative means it's using (1 - index) to get the "other" buffer
    int index = *(int*)(p + 0x0c);
    void* dbf = p + 0x20 + (index * 0x24);
    
    // Get the DIB bitmap from the DBF (offset 0x08 in DBF)
    void* dib = (char*)dbf + 0x08;
    if (!dib) {
        if (g_origDrawEnd) g_origDrawEnd(self);
        return;
    }
    
    // Check windowed vs fullscreen mode
    int mode = *(int*)(p + 0x6c);
    int screenWidth = *(int*)(p + 0x120);
    int screenHeight = *(int*)(p + 0x124);
    HWND hwnd = *(HWND*)(p + 0x1c);
    
    DBF_LOG("Paint: mode=%d, %dx%d, hwnd=%p", mode, screenWidth, screenHeight, hwnd);
    
    if (mode != 0) {
        // Windowed mode - use OpenGL instead of BitBlt
        
        // Initialize OpenGL on first call
        if (!g_glInitialized && hwnd) {
            InitOpenGL(hwnd, screenWidth, screenHeight);
        }
        
        if (g_glInitialized) {
            // Create a temporary DC and bitmap to get the pixel data
            HDC memDC = CreateCompatibleDC(param_1);
            
            // Get the HBITMAP from this+0x140
            HBITMAP hBitmap = *(HBITMAP*)(p + 0x140);
            if (hBitmap) {
                HGDIOBJ oldBmp = SelectObject(memDC, hBitmap);
                
                // Call original TransferToDDB to render into our bitmap
                if (g_origTransferToDDB) {
                    g_origTransferToDDB(dib, memDC, 0, 0);
                }
                
                // Call optional paint callback at this+0x138
                void (*paintCallback)(HDC) = *(void (**)(HDC))(p + 0x138);
                if (paintCallback) {
                    paintCallback(memDC);
                }
                
                // Get bitmap bits for OpenGL upload
                BITMAP bm;
                GetObject(hBitmap, sizeof(BITMAP), &bm);
                
                // Allocate buffer for pixel data
                int stride = ((bm.bmWidth * 4 + 3) & ~3);
                void* pixels = malloc(stride * bm.bmHeight);
                if (pixels) {
                    BITMAPINFOHEADER bi = {};
                    bi.biSize = sizeof(bi);
                    bi.biWidth = bm.bmWidth;
                    bi.biHeight = -bm.bmHeight;  // top-down
                    bi.biPlanes = 1;
                    bi.biBitCount = 32;
                    bi.biCompression = BI_RGB;
                    
                    GetDIBits(memDC, hBitmap, 0, bm.bmHeight, pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
                    
                    // Present to screen with OpenGL
                    PresentOpenGL(pixels, screenWidth, screenHeight);
                    
                    free(pixels);
                }
                
                SelectObject(memDC, oldBmp);
            }
            DeleteDC(memDC);
        } else {
            // Fallback to original BitBlt path if OpenGL failed
            HDC memDC = CreateCompatibleDC(param_1);
            HBITMAP hBitmap = *(HBITMAP*)(p + 0x140);
            if (hBitmap) {
                SelectObject(memDC, hBitmap);
                if (g_origTransferToDDB) {
                    g_origTransferToDDB(dib, memDC, 0, 0);
                }
                void (*paintCallback)(HDC) = *(void (**)(HDC))(p + 0x138);
                if (paintCallback) {
                    paintCallback(memDC);
                }
                BitBlt(param_1, 0, 0, screenWidth, screenHeight, memDC, 0, 0, SRCCOPY);
            }
            DeleteDC(memDC);
        }
    } else {
        // Fullscreen mode - forward to original implementation
        // This uses DirectDraw which our ddraw_wrapper handles
        typedef void (__thiscall *Paint_t)(void*, HDC, int);
        static Paint_t origPaint = nullptr;
        if (!origPaint) {
            origPaint = (Paint_t)LoadOrigFunc("o_RKC_DBFCONTROL.dll",
                "?Paint@RKC_DBFCONTROL@@QAEXPAUHDC__@@H@Z");
        }
        if (origPaint) {
            origPaint(self, param_1, param_2);
            return;  // Original calls DrawEnd itself
        }
    }
    
    if (g_origDrawEnd) g_origDrawEnd(self);
}

// DLL entry point for cleanup
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DBF_LOG_INIT();
            DBF_LOG("RKC_DBFCONTROL.dll loaded (OpenGL hook)");
            break;
        case DLL_PROCESS_DETACH:
            ShutdownOpenGL();
            DBF_LOG("RKC_DBFCONTROL.dll unloaded");
            DBF_LOG_SHUTDOWN();
            break;
    }
    return TRUE;
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

// ============================================================================
// IMPLEMENTED FUNCTIONS - USED BY EXE
// ============================================================================

/**
 * RKC_DBFCONTROL::DrawEnd - Called after each frame is painted
 * Increments draw count and clears the "drawing in progress" flag.
 * Original uses mutex but we simplify since no threading.
 * USED BY: ShadowFlare.exe
 */
void __thiscall RKC_DBFCONTROL_DrawEnd(void* self) {
    char* p = (char*)self;
    // Increment draw count at offset 0x68
    *(int*)(p + 0x68) = *(int*)(p + 0x68) + 1;
    // Clear drawing flag at offset 0x04 if it was 1
    if (*(int*)(p + 0x04) == 1) {
        *(int*)(p + 0x04) = 0;
    }
}

/**
 * RKC_DBFCONTROL::SetPaintFunction - Store paint callback pointer
 * USED BY: ShadowFlare.exe
 */
void __thiscall RKC_DBFCONTROL_SetPaintFunction(void* self, void* callback) {
    *(void**)((char*)self + 0x138) = callback;
}

/**
 * RKC_DBFCONTROL::SetScreenClear - Set screen clear flag and color
 * USED BY: ShadowFlare.exe
 */
void __thiscall RKC_DBFCONTROL_SetScreenClear(void* self, int flag, void* rgbquad) {
    char* p = (char*)self;
    *(int*)(p + 0x7c) = flag;
    if (rgbquad != nullptr) {
        *(uint32_t*)(p + 0x80) = *(uint32_t*)rgbquad;
    } else {
        *(uint32_t*)(p + 0x80) = 0;
    }
}

/**
 * RKC_DBFCONTROL::SetClipRect - Set clip rect for both DBF objects
 * Copies RECT to offsets 0x34 and 0x58 (the two embedded RKC_DBF clip rects)
 * USED BY: ShadowFlare.exe
 */
void __thiscall RKC_DBFCONTROL_SetClipRect(void* self, void* rect) {
    char* p = (char*)self;
    uint32_t* r = (uint32_t*)rect;
    // Copy to first DBF clip rect at 0x34
    *(uint32_t*)(p + 0x34) = r[0];  // left
    *(uint32_t*)(p + 0x38) = r[1];  // top
    *(uint32_t*)(p + 0x3c) = r[2];  // right
    *(uint32_t*)(p + 0x40) = r[3];  // bottom
    // Copy to second DBF clip rect at 0x58
    *(uint32_t*)(p + 0x58) = r[0];
    *(uint32_t*)(p + 0x5c) = r[1];
    *(uint32_t*)(p + 0x60) = r[2];
    *(uint32_t*)(p + 0x64) = r[3];
}

/**
 * RKC_DBFCONTROL::Clear - Clear both buffers with color or zero
 * Calls RKC_DIB::Fill or RKC_DIB::FillByte on both embedded DIBs
 * USED BY: ShadowFlare.exe
 */
int __thiscall RKC_DBFCONTROL_Clear(void* self, void* rgbquad) {
    char* p = (char*)self;
    // The two embedded RKC_DIB objects are at offsets 0x28 and 0x4c
    // (actually inside RKC_DBF which starts at 0x20, and RKC_DIB is at +0x08 within RKC_DBF)
    void* dib1 = p + 0x28;  // First DBF's DIB
    void* dib2 = p + 0x4c;  // Second DBF's DIB
    
    if (rgbquad != nullptr) {
        // Fill with color - RGBQUAD is 4 bytes (B,G,R,reserved)
        // RKC_DIB::Fill takes long color value (BGR format)
        uint32_t color = *(uint32_t*)rgbquad & 0x00FFFFFF;  // Mask off reserved byte
        CallFunctionInDLL<int>("RKC_DIB.dll", "?Fill@RKC_DIB@@QAEHJ@Z", dib1, (long)color);
        CallFunctionInDLL<int>("RKC_DIB.dll", "?Fill@RKC_DIB@@QAEHJ@Z", dib2, (long)color);
    } else {
        // Fill with zero bytes
        CallFunctionInDLL<int>("RKC_DIB.dll", "?FillByte@RKC_DIB@@QAEHE@Z", dib1, (unsigned char)0);
        CallFunctionInDLL<int>("RKC_DIB.dll", "?FillByte@RKC_DIB@@QAEHE@Z", dib2, (unsigned char)0);
    }
    return 1;
}

// ============================================================================
// STUBS - NOT USED BY EXE OR OTHER DLLS
// ============================================================================

// RKC_DBFCONTROL - NOT USED
void* __thiscall RKC_DBFCONTROL_operatorAssign(void* self, const void* src) { return self; }
void __thiscall RKC_DBFCONTROL_DisableDraw(void* self) {}
void __thiscall RKC_DBFCONTROL_DrawFunction(void* self) {}
void* __thiscall RKC_DBFCONTROL_Draw(void* self) { return nullptr; }
void __thiscall RKC_DBFCONTROL_EnableDraw(void* self) {}
void __thiscall RKC_DBFCONTROL_FlushDrawCount(void* self) { *(int*)((char*)self + 0x68) = 0; }
void __thiscall RKC_DBFCONTROL_GetClipRect(void* self, void* rect, long arg) {}
int __thiscall RKC_DBFCONTROL_Redraw(void* self) { return 0; }
void __thiscall RKC_DBFCONTROL_Release(void* self) {}

} // extern "C"

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
#include <mmsystem.h>
#include <ddraw.h>
#include <GL/gl.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
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
static WNDPROC g_origWndProc = nullptr;
static HWND g_hookedHwnd = nullptr;
static int g_viewX = 0;
static int g_viewY = 0;
static int g_viewW = 640;
static int g_viewH = 480;
static int g_virtualW = 640;
static int g_virtualH = 480;

// Debug logging
static FILE* g_logFile = nullptr;

static void DBF_LOG_INIT() {
    char enabledValue[8]{};
    DWORD enabledLength = GetEnvironmentVariableA(
        "OSF_DBF_LOG", enabledValue, static_cast<DWORD>(sizeof(enabledValue)));
    if (enabledLength == 0 || enabledValue[0] == '0')
        return;
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

static LRESULT CALLBACK DBF_WndProcHook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP: {
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);

            if (g_viewW > 0 && g_viewH > 0 && g_virtualW > 0 && g_virtualH > 0) {
                int vx = (x - g_viewX) * g_virtualW / g_viewW;
                int vy = (y - g_viewY) * g_virtualH / g_viewH;

                if (vx < 0) vx = 0;
                if (vy < 0) vy = 0;
                if (vx >= g_virtualW) vx = g_virtualW - 1;
                if (vy >= g_virtualH) vy = g_virtualH - 1;

                lParam = MAKELPARAM((SHORT)vx, (SHORT)vy);
            }
            break;
        }
    }

    if (g_origWndProc) {
        return CallWindowProcA(g_origWndProc, hwnd, msg, wParam, lParam);
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

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

    if (hwnd) {
        LONG style = GetWindowLongA(hwnd, GWL_STYLE);
        style |= (WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
        style &= ~WS_POPUP;
        SetWindowLongA(hwnd, GWL_STYLE, style);
        SetWindowPos(
            hwnd,
            nullptr,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED
        );
    }

    if (hwnd && !g_origWndProc) {
        g_origWndProc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)DBF_WndProcHook);
        g_hookedHwnd = hwnd;
        DBF_LOG("WndProc hook installed: hwnd=%p orig=%p", hwnd, g_origWndProc);
    }
    
    DBF_LOG("OpenGL initialized: %s", (const char*)glGetString(GL_VERSION));
    return true;
}

static void ShutdownOpenGL() {
    if (g_hookedHwnd && g_origWndProc) {
        SetWindowLongPtrA(g_hookedHwnd, GWLP_WNDPROC, (LONG_PTR)g_origWndProc);
        g_origWndProc = nullptr;
        g_hookedHwnd = nullptr;
    }

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

    HWND hwnd = WindowFromDC(g_hdc);
    RECT rc = {};
    int viewportW = width;
    int viewportH = height;
    if (hwnd && GetClientRect(hwnd, &rc)) {
        int cw = rc.right - rc.left;
        int ch = rc.bottom - rc.top;
        if (cw > 0 && ch > 0) {
            viewportW = cw;
            viewportH = ch;
        }
    }

    float sx = (float)viewportW / (float)width;
    float sy = (float)viewportH / (float)height;
    float scale = (sx < sy) ? sx : sy;
    int drawW = (int)(width * scale);
    int drawH = (int)(height * scale);
    int drawX = (viewportW - drawW) / 2;
    int drawY = (viewportH - drawH) / 2;

    g_viewX = drawX;
    g_viewY = drawY;
    g_viewW = drawW;
    g_viewH = drawH;
    g_virtualW = width;
    g_virtualH = height;

    glViewport(0, 0, viewportW, viewportH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (double)viewportW, (double)viewportH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Upload pixels to texture
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
    
    // Draw centered quad with preserved aspect ratio.
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f((float)drawX, (float)drawY);
    glTexCoord2f(1, 0); glVertex2f((float)(drawX + drawW), (float)drawY);
    glTexCoord2f(1, 1); glVertex2f((float)(drawX + drawW), (float)(drawY + drawH));
    glTexCoord2f(0, 1); glVertex2f((float)drawX, (float)(drawY + drawH));
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
    char* p = static_cast<char*>(self);
    HANDLE mutex = *(HANDLE*)(p + 0x13c);
    if (mutex)
        WaitForSingleObject(mutex, INFINITE);
    const int value = *(int*)(p + 0x04);
    if (mutex)
        ReleaseMutex(mutex);
    return value;
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
    if (arg == 1) {
        return (long)0xca0000;  // WS_CAPTION | WS_SYSMENU
    }
    return (long)0x80000000L;  // WS_POPUP
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
    result = result & static_cast<long>(0xfffffff8UL);
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
void __thiscall RKC_DBFCONTROL_DrawEnd(void* self);
void __thiscall RKC_DBFCONTROL_SetClipRect(void* self, void* rect);
int __thiscall RKC_DBFCONTROL_Redraw(void* self);
void __thiscall RKC_DBFCONTROL_DrawFunction(void* self);
void __thiscall RKC_DBFCONTROL_EnableDraw(void* self);
void __thiscall RKC_DBFCONTROL_DisableDraw(void* self);

// Initialize function pointers from original DLLs
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
    
    // Check state flag at offset 0x00
    if (*(int*)p != 1) {
        RKC_DBFCONTROL_DrawEnd(self);
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
        RKC_DBFCONTROL_DrawEnd(self);
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
                CallFunctionInDLL<int>(
                    "RKC_DIB.dll",
                    "?TransferToDDB@RKC_DIB@@QAEHPAUHDC__@@JJ@Z",
                    dib, memDC, 0L, 0L);
                
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
                CallFunctionInDLL<int>(
                    "RKC_DIB.dll",
                    "?TransferToDDB@RKC_DIB@@QAEHPAUHDC__@@JJ@Z",
                    dib, memDC, 0L, 0L);
                void (*paintCallback)(HDC) = *(void (**)(HDC))(p + 0x138);
                if (paintCallback) {
                    paintCallback(memDC);
                }
                BitBlt(param_1, 0, 0, screenWidth, screenHeight, memDC, 0, 0, SRCCOPY);
            }
            DeleteDC(memDC);
        }
    } else {
        IDirectDrawSurface* primary =
            *(IDirectDrawSurface**)(p + 0x12c);
        IDirectDrawSurface* back =
            *(IDirectDrawSurface**)(p + 0x130);
        if (primary && back) {
            if (param_2 == 0) {
                HDC surfaceDc = nullptr;
                HRESULT result = back->GetDC(&surfaceDc);
                if (result == DDERR_SURFACELOST) {
                    back->Restore();
                    result = back->GetDC(&surfaceDc);
                }
                if (SUCCEEDED(result) && surfaceDc) {
                    CallFunctionInDLL<int>(
                        "RKC_DIB.dll",
                        "?TransferToDDB@RKC_DIB@@QAEHPAUHDC__@@JJ@Z",
                        dib, surfaceDc, 0L, 0L);
                    void (*paintCallback)(HDC) =
                        *(void (**)(HDC))(p + 0x138);
                    if (paintCallback)
                        paintCallback(surfaceDc);
                    back->ReleaseDC(surfaceDc);
                    result = primary->Flip(nullptr, DDFLIP_WAIT);
                    if (result == DDERR_SURFACELOST)
                        primary->Restore();
                }
            } else {
                IDirectDraw* directDraw = *(IDirectDraw**)(p + 0x128);
                if (directDraw)
                    directDraw->FlipToGDISurface();
                CallFunctionInDLL<int>(
                    "RKC_DIB.dll",
                    "?TransferToDDB@RKC_DIB@@QAEHPAUHDC__@@JJ@Z",
                    dib, param_1, 0L, 0L);
                void (*paintCallback)(HDC) =
                    *(void (**)(HDC))(p + 0x138);
                if (paintCallback)
                    paintCallback(param_1);
            }
        }
    }
    
    RKC_DBFCONTROL_DrawEnd(self);
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

void* __thiscall RKC_DBF_constructor(void* self) {
    CallFunctionInDLL<void*>(
        "RKC_DIB.dll", "??0RKC_DIB@@QAE@XZ", (char*)self + 8);
    *(long*)((char*)self + 0) = 0;
    *(long*)((char*)self + 4) = 0;
    return self;
}
void __thiscall RKC_DBF_Release(void* self) {
    CallFunctionInDLL<void>(
        "RKC_DIB.dll", "?Release@RKC_DIB@@QAEXXZ", (char*)self + 8);
    *(long*)((char*)self + 0) = 0;
    *(long*)((char*)self + 4) = 0;
}
void __thiscall RKC_DBF_destructor(void* self) {
    RKC_DBF_Release(self);
    CallFunctionInDLL<void>(
        "RKC_DIB.dll", "??1RKC_DIB@@QAE@XZ", (char*)self + 8);
}
void* __thiscall RKC_DBF_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 0x24);
    return self;
}
void __thiscall RKC_DBF_Draw(void* self) {
    CallFunctionInDLL<int>(
        "RKC_UPDIB.dll",
        "?Render@RKC_UPDIB@@QAEHPAVRKC_DIB@@JJJJPAUtagRECT@@@Z",
        *(void**)self, (char*)self + 8, *(long*)((char*)self + 4),
        0L, 0L, 0L, (char*)self + 0x14);
}
void __thiscall RKC_DBF_Flush(void* self) {
    CallFunctionInDLL<void>(
        "RKC_UPDIB.dll",
        "?FlushVSBlock@RKC_UPDIB@@QAEXJ@Z",
        *(void**)self, *(long*)((char*)self + 4));
}
void __thiscall RKC_DBF_GetClipRect(void* self, void* rect) {
    std::memcpy(rect, (char*)self + 0x14, sizeof(RECT));
}

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
    HANDLE mutex = *(HANDLE*)(p + 0x13c);
    if (mutex)
        WaitForSingleObject(mutex, INFINITE);
    *(int*)(p + 0x68) = *(int*)(p + 0x68) + 1;
    if (*(int*)(p + 0x04) == 1) {
        *(int*)(p + 0x04) = 0;
    }
    if (mutex)
        ReleaseMutex(mutex);
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

void* __thiscall RKC_DBFCONTROL_constructor(void* self) {
    char* p = static_cast<char*>(self);
    RKC_DBF_constructor(p + 0x20);
    RKC_DBF_constructor(p + 0x44);
    *(long*)(p + 0x0c) = 0;
    *(long*)(p + 0x04) = 0;
    *(void**)(p + 0x128) = nullptr;
    *(void**)(p + 0x12c) = nullptr;
    *(void**)(p + 0x130) = nullptr;
    *(long*)(p + 0x00) = 0;
    *(long*)(p + 0x08) = 0;
    *(long*)(p + 0x10) = 0;
    *(long*)(p + 0x78) = 0;
    *(long*)(p + 0x120) = 640;
    *(long*)(p + 0x124) = 480;
    *(void**)(p + 0x138) = nullptr;
    *(long*)(p + 0x88) = 0;
    *(void**)(p + 0x140) = nullptr;
    *(void**)(p + 0x84) = nullptr;
    *(void**)(p + 0x74) = nullptr;
    *(long*)(p + 0x14) = 0;
    *(HANDLE*)(p + 0x13c) = CreateMutexA(nullptr, FALSE, nullptr);
    return self;
}

void* __thiscall RKC_DBFCONTROL_operatorAssign(void* self, const void* src) {
    std::memcpy(self, src, 0x144);
    return self;
}

int __thiscall RKC_DBFCONTROL_Initialize(
    void* self,
    HWND window,
    void* updib,
    long firstVsBlock,
    long secondVsBlock,
    long mode,
    int (__cdecl *frameFunction)(),
    long width,
    long height,
    RECT* clip,
    int screenClear,
    RGBQUAD* clearColor,
    long)
{
    char* p = static_cast<char*>(self);
    *(HWND*)(p + 0x1c) = window;
    *(void**)(p + 0x20) = updib;
    *(void**)(p + 0x44) = updib;
    *(void**)(p + 0x134) =
        reinterpret_cast<void*>(frameFunction);
    *(long*)(p + 0x24) = firstVsBlock;
    *(long*)(p + 0x48) = secondVsBlock;
    CallFunctionInDLL<int>(
        "RKC_DIB.dll", "?Create@RKC_DIB@@QAEHJJJH@Z",
        p + 0x28, width, height, 24L, 1);
    CallFunctionInDLL<int>(
        "RKC_DIB.dll", "?Create@RKC_DIB@@QAEHJJJH@Z",
        p + 0x4c, width, height, 24L, 1);
    *(long*)(p + 0x120) = width;
    *(long*)(p + 0x124) = height;
    if (clip)
        RKC_DBFCONTROL_SetClipRect(self, clip);
    *(long*)(p + 0x7c) = screenClear;
    *(RGBQUAD*)(p + 0x80) = clearColor ? *clearColor : RGBQUAD{};
    *(long*)(p + 0x6c) = mode;

    if (mode == 0) {
        IDirectDraw* directDraw = nullptr;
        if (FAILED(DirectDrawCreate(nullptr, &directDraw, nullptr)))
            return 0;
        *(IDirectDraw**)(p + 0x128) = directDraw;
        if (FAILED(directDraw->SetCooperativeLevel(
                window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN))
            || FAILED(directDraw->SetDisplayMode(width, height, 16)))
            return 0;
        DDSURFACEDESC description{};
        description.dwSize = sizeof(description);
        description.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        description.ddsCaps.dwCaps =
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
        description.dwBackBufferCount = 1;
        IDirectDrawSurface* primary = nullptr;
        if (FAILED(directDraw->CreateSurface(
                &description, &primary, nullptr)))
            return 0;
        *(IDirectDrawSurface**)(p + 0x12c) = primary;
        DDSCAPS capabilities{};
        capabilities.dwCaps = DDSCAPS_BACKBUFFER;
        IDirectDrawSurface* back = nullptr;
        if (FAILED(primary->GetAttachedSurface(&capabilities, &back)))
            return 0;
        *(IDirectDrawSurface**)(p + 0x130) = back;
    } else {
        HDC dc = GetDC(window);
        *(HBITMAP*)(p + 0x140) =
            CreateCompatibleBitmap(dc, width, height);
        ReleaseDC(window, dc);
    }
    return 1;
}

static void CALLBACK DBF_TimerCallback(
    UINT, UINT, DWORD_PTR user, DWORD_PTR, DWORD_PTR)
{
    ++*reinterpret_cast<volatile long*>(
        static_cast<char*>(reinterpret_cast<void*>(user)) + 0x10);
}

static DWORD WINAPI DBF_FrameThread(void* parameter)
{
    char* p = static_cast<char*>(parameter);
    long previousTick = *(volatile long*)(p + 0x10);
    long fpsTick = previousTick;
    while (*(volatile long*)(p + 0x08) == 1) {
        auto frameFunction =
            reinterpret_cast<int (__cdecl*)()>(*(void**)(p + 0x134));
        if (!frameFunction || frameFunction() == 1) {
            const long currentTick = *(volatile long*)(p + 0x10);
            if (currentTick - fpsTick >= 1000) {
                fpsTick = currentTick;
                *(long*)(p + 0x70) = *(long*)(p + 0x68);
                *(long*)(p + 0x68) = 0;
            }
            RKC_DBFCONTROL_Redraw(parameter);
        }
        while (*(volatile long*)(p + 0x08) == 1
            && *(volatile long*)(p + 0x10) - previousTick < 33)
            Sleep(1);
        previousTick = *(volatile long*)(p + 0x10);
    }
    *(long*)(p + 0x18) = 1;
    return 0;
}

static DWORD WINAPI DBF_DrawThread(void* parameter)
{
    char* p = static_cast<char*>(parameter);
    while (*(volatile long*)(p + 0x08) == 1) {
        if (*(volatile long*)(p + 0x14) == 1) {
            RKC_DBFCONTROL_DrawFunction(parameter);
            PostMessageA(*(HWND*)(p + 0x1c), WM_USER, 0, 0);
        } else {
            Sleep(1);
        }
    }
    return 0;
}

void __thiscall RKC_DBFCONTROL_StartAll(void* self)
{
    char* p = static_cast<char*>(self);
    if (*(long*)(p + 0x08) != 0)
        return;
    *(long*)(p + 0x08) = 1;
    *(long*)(p + 0x18) = 0;
    if (!*(HANDLE*)(p + 0x74)) {
        *(HANDLE*)(p + 0x74) = CreateThread(
            nullptr, 0, DBF_FrameThread, self, 0, nullptr);
        if (*(HANDLE*)(p + 0x74))
            SetThreadPriority(
                *(HANDLE*)(p + 0x74), THREAD_PRIORITY_HIGHEST);
    }
    if (!*(HANDLE*)(p + 0x84))
        *(HANDLE*)(p + 0x84) = CreateThread(
            nullptr, 0, DBF_DrawThread, self, 0, nullptr);
    if (*(UINT*)(p + 0x78) == 0)
        *(UINT*)(p + 0x78) = timeSetEvent(
            1, 1, DBF_TimerCallback,
            reinterpret_cast<DWORD_PTR>(self), TIME_PERIODIC);
    RKC_DBFCONTROL_EnableDraw(self);
}

void __thiscall RKC_DBFCONTROL_StopAll(void* self)
{
    char* p = static_cast<char*>(self);
    if (*(long*)(p + 0x08) != 1)
        return;
    *(long*)(p + 0x08) = 0;
    if (*(UINT*)(p + 0x78)) {
        timeKillEvent(*(UINT*)(p + 0x78));
        *(UINT*)(p + 0x78) = 0;
    }
    HANDLE threads[2] = {
        *(HANDLE*)(p + 0x74), *(HANDLE*)(p + 0x84)
    };
    for (HANDLE& thread : threads) {
        if (thread) {
            WaitForSingleObject(thread, 2000);
            CloseHandle(thread);
            thread = nullptr;
        }
    }
    *(HANDLE*)(p + 0x74) = threads[0];
    *(HANDLE*)(p + 0x84) = threads[1];
    RKC_DBFCONTROL_DisableDraw(self);
}

void __thiscall RKC_DBFCONTROL_DisableDraw(void* self) {
    *(int*)self = 0;
    while (RKC_DBFCONTROL_GetDrawingFlag(self) == 1)
        Sleep(1);
}

void* __thiscall RKC_DBFCONTROL_Draw(void* self) {
    char* p = static_cast<char*>(self);
    const long current = *(long*)(p + 0x0c);
    void* drawDib = p + 0x4c - current * 0x24;
    void* drawDbf = p + 0x44 - current * 0x24;
    if (*(long*)(p + 0x7c) == 1) {
        const unsigned long color =
            (unsigned char)p[0x80]
            | ((unsigned long)(unsigned char)p[0x81] << 8)
            | ((unsigned long)(unsigned char)p[0x82] << 16);
        CallFunctionInDLL<int>(
            "RKC_DIB.dll", "?Fill@RKC_DIB@@QAEHJ@Z",
            drawDib, (long)color);
    }
    RKC_DBF_Draw(drawDbf);
    return drawDib;
}

void __thiscall RKC_DBFCONTROL_DrawFunction(void* self) {
    if (*(long*)self == 1)
        RKC_DBFCONTROL_Draw(self);
    *(long*)((char*)self + 0x14) = 0;
}

void __thiscall RKC_DBFCONTROL_EnableDraw(void* self) { *(int*)((char*)self) = 1; }
void __thiscall RKC_DBFCONTROL_FlushDrawCount(void* self) { *(int*)((char*)self + 0x68) = 0; }
void __thiscall RKC_DBFCONTROL_GetClipRect(void* self, void* rect, long arg) {
    std::memcpy(
        rect, (char*)self + 0x34 + arg * 0x24, sizeof(RECT));
}
int __thiscall RKC_DBFCONTROL_Redraw(void* self) {
    char* p = static_cast<char*>(self);
    if (RKC_DBFCONTROL_GetDrawingFlag(self) != 1
        && *(long*)(p + 0x14) == 0) {
        *(long*)(p + 0x0c) = 1 - *(long*)(p + 0x0c);
        HANDLE mutex = *(HANDLE*)(p + 0x13c);
        WaitForSingleObject(mutex, INFINITE);
        *(long*)(p + 0x04) = 1;
        ReleaseMutex(mutex);
        *(long*)(p + 0x14) = 1;
    }
    RKC_DBF_Flush(p + 0x20 + *(long*)(p + 0x0c) * 0x24);
    return 1;
}
void __thiscall RKC_DBFCONTROL_Release(void* self) {
    char* p = static_cast<char*>(self);
    if (*(HANDLE*)(p + 0x74)) {
        CloseHandle(*(HANDLE*)(p + 0x74));
        *(HANDLE*)(p + 0x74) = nullptr;
    }
    if (*(HANDLE*)(p + 0x84)) {
        CloseHandle(*(HANDLE*)(p + 0x84));
        *(HANDLE*)(p + 0x84) = nullptr;
    }
    if (*(UINT*)(p + 0x78))
        timeKillEvent(*(UINT*)(p + 0x78));
    *(UINT*)(p + 0x78) = 0;
    if (*(IDirectDrawSurface**)(p + 0x12c)) {
        (*(IDirectDrawSurface**)(p + 0x12c))->Release();
        *(IDirectDrawSurface**)(p + 0x12c) = nullptr;
    }
    if (*(IDirectDraw**)(p + 0x128)) {
        (*(IDirectDraw**)(p + 0x128))->Release();
        *(IDirectDraw**)(p + 0x128) = nullptr;
    }
    *(IDirectDrawSurface**)(p + 0x130) = nullptr;
    if (*(HGDIOBJ*)(p + 0x140)) {
        DeleteObject(*(HGDIOBJ*)(p + 0x140));
        *(HGDIOBJ*)(p + 0x140) = nullptr;
    }
}
void __thiscall RKC_DBFCONTROL_destructor(void* self) {
    char* p = static_cast<char*>(self);
    RKC_DBFCONTROL_Release(self);
    if (*(HANDLE*)(p + 0x13c)) {
        CloseHandle(*(HANDLE*)(p + 0x13c));
        *(HANDLE*)(p + 0x13c) = nullptr;
    }
    RKC_DBF_destructor(p + 0x44);
    RKC_DBF_destructor(p + 0x20);
}

} // extern "C"

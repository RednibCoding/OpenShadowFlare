/*
 * ddraw_wrapper.cpp - DirectDraw replacement using OpenGL
 * 
 * This implements the minimal DirectDraw interface used by ShadowFlare's
 * RKC_DBFCONTROL.dll. The game only uses DirectDraw for:
 * 
 *   1. Fullscreen mode / display mode switching
 *   2. Page flipping (double buffering) 
 *   3. Getting GDI DC from surface for BitBlt operations
 * 
 * All actual rendering is done through GDI (RKC_DIB uses StretchDIBits/SetDIBitsToDevice),
 * so we just need to provide a software backbuffer and flip it to screen with OpenGL.
 * 
 * The game does NOT use Blt/BltFast - all rendering goes through GDI GetDC.
 * 
 * Build (MinGW cross-compile for 32-bit):
 *   i686-w64-mingw32-g++ -shared -static ddraw_wrapper.cpp -o ddraw.dll ddraw.def -lopengl32 -lgdi32 -luser32
 */

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>
#include <cstdio>
#include <cstring>

// Debug logging - writes to file and stderr
static FILE* g_logFile = nullptr;

static void DDRAW_LOG_INIT() {
    g_logFile = fopen("ddraw_log.txt", "w");
    if (g_logFile) {
        fprintf(g_logFile, "=== ddraw.dll wrapper log started ===\n");
        fflush(g_logFile);
    }
}

static void DDRAW_LOG_SHUTDOWN() {
    if (g_logFile) {
        fprintf(g_logFile, "=== ddraw.dll wrapper log ended ===\n");
        fclose(g_logFile);
        g_logFile = nullptr;
    }
}

#define DDRAW_LOG(fmt, ...) do { \
    fprintf(stderr, "[ddraw] " fmt "\n", ##__VA_ARGS__); \
    if (g_logFile) { fprintf(g_logFile, "[ddraw] " fmt "\n", ##__VA_ARGS__); fflush(g_logFile); } \
} while(0)

// GL_BGRA_EXT constant (not always defined in MinGW headers)
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

/*==============================================================================
 * COM Wrapper Structures
 * 
 * DirectDraw uses COM, so objects must start with a vtable pointer.
 *============================================================================*/

struct DDWrapper;
struct DDSurfaceWrapper;

struct DDWrapper {
    void** vtbl;
    ULONG refCount;
    HWND hwnd;
    int displayWidth;
    int displayHeight;
    int displayBpp;
    bool fullscreen;
    HDC hdc;
    HGLRC hglrc;
    GLuint texture;
    DDSurfaceWrapper* primarySurface;
};

struct DDSurfaceWrapper {
    void** vtbl;
    ULONG refCount;
    DDWrapper* parent;
    bool isPrimary;
    bool hasBackBuffer;
    int width;
    int height;
    int bpp;
    HBITMAP hBitmap;
    HDC hMemDC;
    void* pixels;
    DDSurfaceWrapper* backBuffer;
};

// Static vtable arrays
static void* g_ddVtblArray[32];
static void* g_ddsVtblArray[40];
static bool g_vtblsInitialized = false;

// Forward declarations
static DDSurfaceWrapper* CreateSurfaceInternal(DDWrapper* dd, int w, int h, bool primary, bool withBackBuffer);

/*==============================================================================
 * IDirectDraw Methods
 *============================================================================*/

static HRESULT STDMETHODCALLTYPE DD_QueryInterface(DDWrapper* self, REFIID riid, void** ppv) {
    (void)riid;
    DDRAW_LOG("DirectDraw::QueryInterface");
    if (!ppv) return E_POINTER;
    *ppv = self;
    self->refCount++;
    return S_OK;
}

static ULONG STDMETHODCALLTYPE DD_AddRef(DDWrapper* self) {
    return ++self->refCount;
}

static ULONG STDMETHODCALLTYPE DD_Release(DDWrapper* self) {
    ULONG count = --self->refCount;
    if (count == 0) {
        DDRAW_LOG("DirectDraw destroyed");
        if (self->texture) glDeleteTextures(1, &self->texture);
        if (self->hglrc) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(self->hglrc);
        }
        if (self->hdc && self->hwnd) ReleaseDC(self->hwnd, self->hdc);
        delete self;
    }
    return count;
}

static HRESULT STDMETHODCALLTYPE DD_Compact(DDWrapper*) { return S_OK; }
static HRESULT STDMETHODCALLTYPE DD_CreateClipper(DDWrapper*, DWORD, void**, void*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DD_CreatePalette(DDWrapper*, DWORD, void*, void**, void*) { return E_NOTIMPL; }

static HRESULT STDMETHODCALLTYPE DD_CreateSurface(DDWrapper* self, void* desc, DDSurfaceWrapper** surface, void*) {
    if (!surface) return E_POINTER;
    
    // Parse DDSURFACEDESC - caps at offset 0x6c for 32-bit
    DWORD* flags = (DWORD*)((char*)desc + 4);
    DWORD* backBufCount = (DWORD*)((char*)desc + 0x14);
    DWORD* caps = (DWORD*)((char*)desc + 0x6c);
    
    bool isPrimary = (*caps & 0x200) != 0;  // DDSCAPS_PRIMARYSURFACE
    bool hasBackBuffer = (*flags & 0x20) && (*backBufCount > 0);
    
    DDRAW_LOG("CreateSurface: primary=%d, backbuf=%d (flags=0x%x, caps=0x%x)", 
              isPrimary, hasBackBuffer, *flags, *caps);
    
    auto* surf = CreateSurfaceInternal(self, self->displayWidth, self->displayHeight, isPrimary, hasBackBuffer);
    *surface = surf;
    
    if (isPrimary) self->primarySurface = surf;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DD_DuplicateSurface(DDWrapper*, DDSurfaceWrapper*, DDSurfaceWrapper**) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DD_EnumDisplayModes(DDWrapper*, DWORD, void*, void*, void*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DD_EnumSurfaces(DDWrapper*, DWORD, void*, void*, void*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DD_FlipToGDISurface(DDWrapper*) { return S_OK; }
static HRESULT STDMETHODCALLTYPE DD_GetCaps(DDWrapper*, void*, void*) { return S_OK; }
static HRESULT STDMETHODCALLTYPE DD_GetDisplayMode(DDWrapper*, void*) { return S_OK; }
static HRESULT STDMETHODCALLTYPE DD_GetFourCCCodes(DDWrapper*, DWORD*, DWORD*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DD_GetGDISurface(DDWrapper*, DDSurfaceWrapper**) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DD_GetMonitorFrequency(DDWrapper*, DWORD* f) { if (f) *f = 60; return S_OK; }
static HRESULT STDMETHODCALLTYPE DD_GetScanLine(DDWrapper*, DWORD* s) { if (s) *s = 0; return S_OK; }
static HRESULT STDMETHODCALLTYPE DD_GetVerticalBlankStatus(DDWrapper*, BOOL* s) { if (s) *s = TRUE; return S_OK; }
static HRESULT STDMETHODCALLTYPE DD_Initialize(DDWrapper*, GUID*) { return S_OK; }
static HRESULT STDMETHODCALLTYPE DD_RestoreDisplayMode(DDWrapper*) { return S_OK; }

static HRESULT STDMETHODCALLTYPE DD_SetCooperativeLevel(DDWrapper* self, HWND hwnd, DWORD flags) {
    DDRAW_LOG("SetCooperativeLevel(hwnd=%p, flags=0x%x)", hwnd, flags);
    self->hwnd = hwnd;
    self->fullscreen = (flags & 0x11) != 0;  // DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE
    return S_OK;
}

static bool DD_InitOpenGL(DDWrapper* self) {
    if (self->hglrc) return true;
    
    self->hdc = GetDC(self->hwnd);
    if (!self->hdc) {
        DDRAW_LOG("ERROR: GetDC failed");
        return false;
    }
    
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;
    
    int format = ChoosePixelFormat(self->hdc, &pfd);
    if (!format) {
        DDRAW_LOG("ERROR: ChoosePixelFormat failed");
        return false;
    }
    
    if (!SetPixelFormat(self->hdc, format, &pfd)) {
        DDRAW_LOG("ERROR: SetPixelFormat failed");
        return false;
    }
    
    self->hglrc = wglCreateContext(self->hdc);
    if (!self->hglrc) {
        DDRAW_LOG("ERROR: wglCreateContext failed");
        return false;
    }
    
    wglMakeCurrent(self->hdc, self->hglrc);
    
    // Set up orthographic projection for 2D rendering
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, self->displayWidth, self->displayHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    
    // Create texture for framebuffer upload
    glGenTextures(1, &self->texture);
    glBindTexture(GL_TEXTURE_2D, self->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->displayWidth, self->displayHeight, 
                 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, nullptr);
    
    DDRAW_LOG("OpenGL initialized: %s", glGetString(GL_VERSION));
    return true;
}

static HRESULT STDMETHODCALLTYPE DD_SetDisplayMode(DDWrapper* self, DWORD w, DWORD h, DWORD bpp) {
    DDRAW_LOG("SetDisplayMode(%d x %d x %d)", w, h, bpp);
    self->displayWidth = w;
    self->displayHeight = h;
    self->displayBpp = bpp;
    return DD_InitOpenGL(self) ? S_OK : E_FAIL;
}

static HRESULT STDMETHODCALLTYPE DD_WaitForVerticalBlank(DDWrapper*, DWORD, HANDLE) { return S_OK; }

/*==============================================================================
 * IDirectDrawSurface Methods
 *============================================================================*/

static HRESULT STDMETHODCALLTYPE DDS_QueryInterface(DDSurfaceWrapper* self, REFIID, void** ppv) {
    if (!ppv) return E_POINTER;
    *ppv = self;
    self->refCount++;
    return S_OK;
}

static ULONG STDMETHODCALLTYPE DDS_AddRef(DDSurfaceWrapper* self) {
    return ++self->refCount;
}

static ULONG STDMETHODCALLTYPE DDS_Release(DDSurfaceWrapper* self) {
    ULONG count = --self->refCount;
    if (count == 0) {
        DDRAW_LOG("Surface destroyed");
        if (self->backBuffer) DDS_Release(self->backBuffer);
        if (self->hBitmap) DeleteObject(self->hBitmap);
        if (self->hMemDC) DeleteDC(self->hMemDC);
        delete self;
    }
    return count;
}

static HRESULT STDMETHODCALLTYPE DDS_AddAttachedSurface(DDSurfaceWrapper*, DDSurfaceWrapper*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_AddOverlayDirtyRect(DDSurfaceWrapper*, RECT*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_Blt(DDSurfaceWrapper*, RECT*, DDSurfaceWrapper*, RECT*, DWORD, void*) { 
    DDRAW_LOG("WARNING: Blt called - not implemented!");
    return E_NOTIMPL; 
}
static HRESULT STDMETHODCALLTYPE DDS_BltBatch(DDSurfaceWrapper*, void*, DWORD, DWORD) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_BltFast(DDSurfaceWrapper*, DWORD, DWORD, DDSurfaceWrapper*, RECT*, DWORD) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_DeleteAttachedSurface(DDSurfaceWrapper*, DWORD, DDSurfaceWrapper*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_EnumAttachedSurfaces(DDSurfaceWrapper*, void*, void*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_EnumOverlayZOrders(DDSurfaceWrapper*, DWORD, void*, void*) { return E_NOTIMPL; }

static void DD_Present(DDWrapper* dd, void* pixels, int w, int h) {
    if (!dd->hglrc) return;
    
    wglMakeCurrent(dd->hdc, dd->hglrc);
    
    // Upload pixels to texture
    glBindTexture(GL_TEXTURE_2D, dd->texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);
    
    // Draw fullscreen quad
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f((float)w, 0);
    glTexCoord2f(1, 1); glVertex2f((float)w, (float)h);
    glTexCoord2f(0, 1); glVertex2f(0, (float)h);
    glEnd();
    
    SwapBuffers(dd->hdc);
}

static HRESULT STDMETHODCALLTYPE DDS_Flip(DDSurfaceWrapper* self, DDSurfaceWrapper*, DWORD) {
    // Present the back buffer to screen using OpenGL
    DDSurfaceWrapper* src = self->backBuffer ? self->backBuffer : self;
    if (src->pixels && self->parent) {
        DD_Present(self->parent, src->pixels, src->width, src->height);
    }
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DDS_GetAttachedSurface(DDSurfaceWrapper* self, void*, DDSurfaceWrapper** surf) {
    if (!surf) return E_POINTER;
    if (self->backBuffer) {
        *surf = self->backBuffer;
        self->backBuffer->refCount++;
        DDRAW_LOG("GetAttachedSurface -> %p", self->backBuffer);
        return S_OK;
    }
    DDRAW_LOG("GetAttachedSurface: no back buffer");
    return E_FAIL;
}

static HRESULT STDMETHODCALLTYPE DDS_GetBltStatus(DDSurfaceWrapper*, DWORD) { return S_OK; }
static HRESULT STDMETHODCALLTYPE DDS_GetCaps(DDSurfaceWrapper*, void*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_GetClipper(DDSurfaceWrapper*, void**) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_GetColorKey(DDSurfaceWrapper*, DWORD, void*) { return E_NOTIMPL; }

static HRESULT STDMETHODCALLTYPE DDS_GetDC(DDSurfaceWrapper* self, HDC* hdc) {
    if (!hdc) return E_POINTER;
    *hdc = self->hMemDC;
    DDRAW_LOG("GetDC -> %p (surface %p)", self->hMemDC, self);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DDS_GetFlipStatus(DDSurfaceWrapper*, DWORD) { return S_OK; }
static HRESULT STDMETHODCALLTYPE DDS_GetOverlayPosition(DDSurfaceWrapper*, LONG*, LONG*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_GetPalette(DDSurfaceWrapper*, void**) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_GetPixelFormat(DDSurfaceWrapper*, void*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_GetSurfaceDesc(DDSurfaceWrapper*, void*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_Initialize(DDSurfaceWrapper*, DDWrapper*, void*) { return S_OK; }
static HRESULT STDMETHODCALLTYPE DDS_IsLost(DDSurfaceWrapper*) { return S_OK; }
static HRESULT STDMETHODCALLTYPE DDS_Lock(DDSurfaceWrapper*, RECT*, void*, DWORD, HANDLE) { return E_NOTIMPL; }

static HRESULT STDMETHODCALLTYPE DDS_ReleaseDC(DDSurfaceWrapper*, HDC) {
    DDRAW_LOG("ReleaseDC");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DDS_Restore(DDSurfaceWrapper*) {
    DDRAW_LOG("Restore");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE DDS_SetClipper(DDSurfaceWrapper*, void*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_SetColorKey(DDSurfaceWrapper*, DWORD, void*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_SetOverlayPosition(DDSurfaceWrapper*, LONG, LONG) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_SetPalette(DDSurfaceWrapper*, void*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_Unlock(DDSurfaceWrapper*, void*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_UpdateOverlay(DDSurfaceWrapper*, RECT*, DDSurfaceWrapper*, RECT*, DWORD, void*) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_UpdateOverlayDisplay(DDSurfaceWrapper*, DWORD) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE DDS_UpdateOverlayZOrder(DDSurfaceWrapper*, DWORD, DDSurfaceWrapper*) { return E_NOTIMPL; }

/*==============================================================================
 * VTable Initialization
 *============================================================================*/

static void initVtables() {
    if (g_vtblsInitialized) return;
    
    // IDirectDraw vtable - must match COM order
    int i = 0;
    g_ddVtblArray[i++] = (void*)DD_QueryInterface;
    g_ddVtblArray[i++] = (void*)DD_AddRef;
    g_ddVtblArray[i++] = (void*)DD_Release;
    g_ddVtblArray[i++] = (void*)DD_Compact;
    g_ddVtblArray[i++] = (void*)DD_CreateClipper;
    g_ddVtblArray[i++] = (void*)DD_CreatePalette;
    g_ddVtblArray[i++] = (void*)DD_CreateSurface;
    g_ddVtblArray[i++] = (void*)DD_DuplicateSurface;
    g_ddVtblArray[i++] = (void*)DD_EnumDisplayModes;
    g_ddVtblArray[i++] = (void*)DD_EnumSurfaces;
    g_ddVtblArray[i++] = (void*)DD_FlipToGDISurface;
    g_ddVtblArray[i++] = (void*)DD_GetCaps;
    g_ddVtblArray[i++] = (void*)DD_GetDisplayMode;
    g_ddVtblArray[i++] = (void*)DD_GetFourCCCodes;
    g_ddVtblArray[i++] = (void*)DD_GetGDISurface;
    g_ddVtblArray[i++] = (void*)DD_GetMonitorFrequency;
    g_ddVtblArray[i++] = (void*)DD_GetScanLine;
    g_ddVtblArray[i++] = (void*)DD_GetVerticalBlankStatus;
    g_ddVtblArray[i++] = (void*)DD_Initialize;
    g_ddVtblArray[i++] = (void*)DD_RestoreDisplayMode;
    g_ddVtblArray[i++] = (void*)DD_SetCooperativeLevel;
    g_ddVtblArray[i++] = (void*)DD_SetDisplayMode;
    g_ddVtblArray[i++] = (void*)DD_WaitForVerticalBlank;
    
    // IDirectDrawSurface vtable - must match COM order
    i = 0;
    g_ddsVtblArray[i++] = (void*)DDS_QueryInterface;
    g_ddsVtblArray[i++] = (void*)DDS_AddRef;
    g_ddsVtblArray[i++] = (void*)DDS_Release;
    g_ddsVtblArray[i++] = (void*)DDS_AddAttachedSurface;
    g_ddsVtblArray[i++] = (void*)DDS_AddOverlayDirtyRect;
    g_ddsVtblArray[i++] = (void*)DDS_Blt;
    g_ddsVtblArray[i++] = (void*)DDS_BltBatch;
    g_ddsVtblArray[i++] = (void*)DDS_BltFast;
    g_ddsVtblArray[i++] = (void*)DDS_DeleteAttachedSurface;
    g_ddsVtblArray[i++] = (void*)DDS_EnumAttachedSurfaces;
    g_ddsVtblArray[i++] = (void*)DDS_EnumOverlayZOrders;
    g_ddsVtblArray[i++] = (void*)DDS_Flip;
    g_ddsVtblArray[i++] = (void*)DDS_GetAttachedSurface;
    g_ddsVtblArray[i++] = (void*)DDS_GetBltStatus;
    g_ddsVtblArray[i++] = (void*)DDS_GetCaps;
    g_ddsVtblArray[i++] = (void*)DDS_GetClipper;
    g_ddsVtblArray[i++] = (void*)DDS_GetColorKey;
    g_ddsVtblArray[i++] = (void*)DDS_GetDC;
    g_ddsVtblArray[i++] = (void*)DDS_GetFlipStatus;
    g_ddsVtblArray[i++] = (void*)DDS_GetOverlayPosition;
    g_ddsVtblArray[i++] = (void*)DDS_GetPalette;
    g_ddsVtblArray[i++] = (void*)DDS_GetPixelFormat;
    g_ddsVtblArray[i++] = (void*)DDS_GetSurfaceDesc;
    g_ddsVtblArray[i++] = (void*)DDS_Initialize;
    g_ddsVtblArray[i++] = (void*)DDS_IsLost;
    g_ddsVtblArray[i++] = (void*)DDS_Lock;
    g_ddsVtblArray[i++] = (void*)DDS_ReleaseDC;
    g_ddsVtblArray[i++] = (void*)DDS_Restore;
    g_ddsVtblArray[i++] = (void*)DDS_SetClipper;
    g_ddsVtblArray[i++] = (void*)DDS_SetColorKey;
    g_ddsVtblArray[i++] = (void*)DDS_SetOverlayPosition;
    g_ddsVtblArray[i++] = (void*)DDS_SetPalette;
    g_ddsVtblArray[i++] = (void*)DDS_Unlock;
    g_ddsVtblArray[i++] = (void*)DDS_UpdateOverlay;
    g_ddsVtblArray[i++] = (void*)DDS_UpdateOverlayDisplay;
    g_ddsVtblArray[i++] = (void*)DDS_UpdateOverlayZOrder;
    
    g_vtblsInitialized = true;
}

/*==============================================================================
 * Surface Creation Helper
 *============================================================================*/

static DDSurfaceWrapper* CreateSurfaceInternal(DDWrapper* dd, int w, int h, bool primary, bool withBackBuffer) {
    auto* surf = new DDSurfaceWrapper();
    memset(surf, 0, sizeof(DDSurfaceWrapper));
    surf->vtbl = g_ddsVtblArray;
    surf->refCount = 1;
    surf->parent = dd;
    surf->isPrimary = primary;
    surf->hasBackBuffer = withBackBuffer;
    surf->width = w;
    surf->height = h;
    surf->bpp = dd->displayBpp;
    
    // Create DIB section for software backbuffer
    // GDI will draw into this, then we upload it to OpenGL on Flip
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;  // Top-down (negative = origin at top-left)
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;  // Always 32bpp internally for easy OpenGL upload
    bmi.bmiHeader.biCompression = BI_RGB;
    
    HDC screenDC = GetDC(nullptr);
    surf->hMemDC = CreateCompatibleDC(screenDC);
    surf->hBitmap = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &surf->pixels, nullptr, 0);
    ReleaseDC(nullptr, screenDC);
    
    if (surf->hBitmap) {
        SelectObject(surf->hMemDC, surf->hBitmap);
        memset(surf->pixels, 0, w * h * 4);
    } else {
        DDRAW_LOG("ERROR: CreateDIBSection failed!");
    }
    
    // Create back buffer if requested (for double buffering)
    if (withBackBuffer) {
        surf->backBuffer = CreateSurfaceInternal(dd, w, h, false, false);
    }
    
    DDRAW_LOG("Surface created: %dx%d, primary=%d, backbuf=%d, pixels=%p", 
              w, h, primary, withBackBuffer, surf->pixels);
    return surf;
}

/*==============================================================================
 * Exported Functions
 *============================================================================*/

extern "C" {

__declspec(dllexport) HRESULT WINAPI DirectDrawCreate(GUID* lpGUID, void** lplpDD, void* pUnkOuter) {
    DDRAW_LOG("DirectDrawCreate called");
    (void)lpGUID;
    (void)pUnkOuter;
    
    if (!lplpDD) return E_POINTER;
    
    initVtables();
    
    auto* dd = new DDWrapper();
    memset(dd, 0, sizeof(DDWrapper));
    dd->vtbl = g_ddVtblArray;
    dd->refCount = 1;
    dd->displayWidth = 640;
    dd->displayHeight = 480;
    dd->displayBpp = 16;
    
    *lplpDD = dd;
    DDRAW_LOG("DirectDraw object created at %p", dd);
    return S_OK;
}

__declspec(dllexport) HRESULT WINAPI DirectDrawCreateEx(GUID* lpGUID, void** lplpDD, REFIID iid, void* pUnkOuter) {
    DDRAW_LOG("DirectDrawCreateEx called");
    (void)iid;
    return DirectDrawCreate(lpGUID, lplpDD, pUnkOuter);
}

__declspec(dllexport) HRESULT WINAPI DirectDrawEnumerateA(void* lpCallback, void* lpContext) {
    DDRAW_LOG("DirectDrawEnumerateA called");
    (void)lpCallback;
    (void)lpContext;
    return S_OK;
}

__declspec(dllexport) HRESULT WINAPI DirectDrawEnumerateW(void* lpCallback, void* lpContext) {
    DDRAW_LOG("DirectDrawEnumerateW called");
    (void)lpCallback;
    (void)lpContext;
    return S_OK;
}

__declspec(dllexport) HRESULT WINAPI DirectDrawEnumerateExA(void* lpCallback, void* lpContext, DWORD dwFlags) {
    DDRAW_LOG("DirectDrawEnumerateExA called");
    (void)lpCallback;
    (void)lpContext;
    (void)dwFlags;
    return S_OK;
}

__declspec(dllexport) HRESULT WINAPI DirectDrawEnumerateExW(void* lpCallback, void* lpContext, DWORD dwFlags) {
    DDRAW_LOG("DirectDrawEnumerateExW called");
    (void)lpCallback;
    (void)lpContext;
    (void)dwFlags;
    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    (void)hinstDLL;
    (void)lpvReserved;
    
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DDRAW_LOG_INIT();
            DDRAW_LOG("=== ddraw.dll wrapper loaded ===");
            break;
        case DLL_PROCESS_DETACH:
            DDRAW_LOG("=== ddraw.dll wrapper unloaded ===");
            DDRAW_LOG_SHUTDOWN();
            break;
    }
    return TRUE;
}

} // extern "C"

DirectDraw Usage Analysis for ShadowFlare (RKC_DBFCONTROL.dll)
================================================================

IDirectDraw interface methods used:
- DirectDrawCreate() - Creates DirectDraw object
- SetCooperativeLevel(hwnd, DDSCL_FULLSCREEN) [vtable 0x50]
- SetDisplayMode(width, height, bpp) [vtable 0x54]
- CreateSurface(desc, surface, null) [vtable 0x18]

IDirectDrawSurface interface methods used:
- GetDC(hdc) [vtable 0x30] - Get GDI device context for surface
- ReleaseDC(hdc) [vtable 0x68] - Release GDI device context
- Flip(null, flags) [vtable 0x2c] - Flip back buffer to front
- Restore() [vtable 0x6c] - Restore lost surface
- GetFlipStatus/GetBltStatus [vtable 0x28] - Check status

NOT USED (important!):
- Blt() - Not used! All rendering goes through GDI (GetDC/BitBlt/ReleaseDC)
- BltFast() - Not used
- Lock()/Unlock() - Not used for direct pixel access

The game uses DirectDraw purely for:
1. Fullscreen mode + display mode switching
2. Page flipping (double buffering)
3. Getting a GDI DC from the surface for BitBlt operations

Since all actual rendering goes through RKC_DIB (which uses GDI only), 
we can replace DirectDraw with:
- OpenGL for display/flipping
- Software backbuffer that RKC_DIB draws to
- glTexSubImage2D to upload the software buffer to GPU
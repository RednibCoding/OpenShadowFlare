/*
 * HWL.hpp - Window and Input for HWL (Happy Windowing Library)
 * by Rednib Coding (Michael Binder)
 * 
 * Minimal cross-platform windowing
 * Supports: Windows, Linux (X11)
 * Modern C++17, memory-safe (no raw pointers in API)
 * 
 * Usage:
 *   #define HWL_IMPLEMENTATION in ONE .cpp file before including
 *   Link: -lX11 -lGL (Linux), -lopengl32 -lgdi32 (Windows)
 */

#ifndef HWL_HPP
#define HWL_HPP

#include <memory>
#include <string>
#include <optional>
#include <functional>
#include <array>
#include <cstdint>

/*==============================================================================
 * Platform Detection
 *============================================================================*/
#if defined(_WIN32) || defined(_WIN64)
    #define HWLS
#elif defined(__linux__)
    #define HWL_LINUX
#else
    #error "Unsupported platform (only Windows and Linux supported)"
#endif

namespace hwl {

/*==============================================================================
 * Enums
 *============================================================================*/
enum class EventType {
    None = 0,
    Close,
    Resize,
    KeyDown,
    KeyUp,
    MouseDown,
    MouseUp,
    MouseMove,
    MouseScroll,
    Focus,
    Blur,
};

enum class MouseButton {
    Left = 0,
    Right,
    Middle,
};

enum class Key {
    Unknown = 0,
    
    // Printable
    Space = 32,
    Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    // Function keys
    Escape = 256,
    Enter,
    Tab,
    Backspace,
    Insert,
    Delete,
    Right,
    Left,
    Down,
    Up,
    PageUp,
    PageDown,
    Home,
    End,
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LeftShift,
    LeftControl,
    LeftAlt,
    RightShift,
    RightControl,
    RightAlt,
    
    Count
};

/*==============================================================================
 * Event
 *============================================================================*/
struct Event {
    EventType type = EventType::None;
    Key key = Key::Unknown;
    MouseButton mouse_button = MouseButton::Left;
    int mouse_x = 0;
    int mouse_y = 0;
    float scroll_x = 0.0f;
    float scroll_y = 0.0f;
    int width = 0;
    int height = 0;
};

/*==============================================================================
 * HwlWindow (forward declaration - implementation is platform-specific)
 *============================================================================*/
class HwlWindow {
public:
    // Factory method - returns nullptr on failure
    static std::unique_ptr<HwlWindow> create(const std::string& title, int width, int height);
    
    virtual ~HwlWindow() = default;
    
    // Non-copyable, movable
    HwlWindow(const HwlWindow&) = delete;
    HwlWindow& operator=(const HwlWindow&) = delete;
    HwlWindow(HwlWindow&&) = default;
    HwlWindow& operator=(HwlWindow&&) = default;
    
    // Window state
    virtual bool shouldClose() const = 0;
    virtual void setShouldClose(bool close) = 0;
    
    // Events
    virtual std::optional<Event> pollEvent() = 0;
    
    // Rendering
    virtual void swapBuffers() = 0;
    virtual void makeGLCurrent() = 0;
    
    // Properties
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void setTitle(const std::string& title) = 0;
    
    // Input state
    virtual bool isKeyDown(Key key) const = 0;
    virtual bool isMouseDown(MouseButton button) const = 0;
    virtual void getMousePos(int& x, int& y) const = 0;
    virtual void setMousePos(int x, int y) = 0;
    
    // Mouse capture (for FPS-style controls)
    virtual void grabMouse() = 0;
    virtual void releaseMouse() = 0;
    virtual bool isMouseGrabbed() const = 0;
    
protected:
    HwlWindow() = default;
};

/*==============================================================================
 * GL loader helper
 *============================================================================*/
void* getGLProc(const char* name);

} // namespace hgl

#endif // HWL_HPP

/*==============================================================================
 * IMPLEMENTATION
 *============================================================================*/
#ifdef HWL_IMPLEMENTATION

#include <cstring>
#include <queue>

/*==============================================================================
 * Platform includes - MUST be outside namespace
 *============================================================================*/
#ifdef HWLS
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
    #include <windowsx.h>
#endif

#ifdef HWL_LINUX
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/keysym.h>
    #include <GL/glx.h>
    #include <dlfcn.h>
#endif

namespace hwl {

/*==============================================================================
 * Windows Implementation
 *============================================================================*/
#ifdef HWLS

// WGL function types
typedef HGLRC (WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
typedef BOOL  (WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC, const int*, const float*, UINT, int*, UINT*);
typedef BOOL  (WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);

// WGL constants
#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001
#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023

class WindowsWindow : public HwlWindow {
public:
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
    HGLRC hglrc = nullptr;
    bool should_close = false;
    int win_width = 0;
    int win_height = 0;
    bool mouse_grabbed = false;
    
    std::array<bool, static_cast<size_t>(Key::Count)> keys{};
    std::array<bool, 3> mouse_buttons{};
    int mouse_x = 0;
    int mouse_y = 0;
    std::queue<Event> event_queue;
    
    ~WindowsWindow() override {
        if (hglrc) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(hglrc);
        }
        if (hdc && hwnd) ReleaseDC(hwnd, hdc);
        if (hwnd) DestroyWindow(hwnd);
    }
    
    bool shouldClose() const override { return should_close; }
    void setShouldClose(bool close) override { should_close = close; }
    
    std::optional<Event> pollEvent() override {
        MSG msg;
        while (PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!event_queue.empty()) {
            Event e = event_queue.front();
            event_queue.pop();
            return e;
        }
        return std::nullopt;
    }
    
    void swapBuffers() override {
        SwapBuffers(hdc);
    }
    
    void makeGLCurrent() override {
        wglMakeCurrent(hdc, hglrc);
    }
    
    int width() const override { return win_width; }
    int height() const override { return win_height; }
    
    void setTitle(const std::string& title) override {
        SetWindowTextA(hwnd, title.c_str());
    }
    
    bool isKeyDown(Key key) const override {
        auto idx = static_cast<size_t>(key);
        return idx < keys.size() && keys[idx];
    }
    
    bool isMouseDown(MouseButton button) const override {
        auto idx = static_cast<size_t>(button);
        return idx < mouse_buttons.size() && mouse_buttons[idx];
    }
    
    void getMousePos(int& x, int& y) const override {
        x = mouse_x;
        y = mouse_y;
    }
    
    void setMousePos(int x, int y) override {
        POINT pt = {x, y};
        ClientToScreen(hwnd, &pt);
        SetCursorPos(pt.x, pt.y);
        mouse_x = x;
        mouse_y = y;
    }
    
    void grabMouse() override {
        if (!mouse_grabbed) {
            // Hide cursor and clip to window
            ShowCursor(FALSE);
            RECT rect;
            GetClientRect(hwnd, &rect);
            POINT tl = {rect.left, rect.top};
            POINT br = {rect.right, rect.bottom};
            ClientToScreen(hwnd, &tl);
            ClientToScreen(hwnd, &br);
            RECT clipRect = {tl.x, tl.y, br.x, br.y};
            ClipCursor(&clipRect);
            mouse_grabbed = true;
        }
    }
    
    void releaseMouse() override {
        if (mouse_grabbed) {
            ShowCursor(TRUE);
            ClipCursor(nullptr);
            mouse_grabbed = false;
        }
    }
    
    bool isMouseGrabbed() const override {
        return mouse_grabbed;
    }

    void pushEvent(const Event& e) {
        event_queue.push(e);
    }
};

// Global for WndProc to find window
static WindowsWindow* g_current_window = nullptr;

static Key translateKey(WPARAM wParam, LPARAM lParam) {
    int scancode = (HIWORD(lParam) & 0x1FF);
    
    switch (wParam) {
        case VK_ESCAPE: return Key::Escape;
        case VK_RETURN: return Key::Enter;
        case VK_TAB: return Key::Tab;
        case VK_BACK: return Key::Backspace;
        case VK_INSERT: return Key::Insert;
        case VK_DELETE: return Key::Delete;
        case VK_RIGHT: return Key::Right;
        case VK_LEFT: return Key::Left;
        case VK_DOWN: return Key::Down;
        case VK_UP: return Key::Up;
        case VK_PRIOR: return Key::PageUp;
        case VK_NEXT: return Key::PageDown;
        case VK_HOME: return Key::Home;
        case VK_END: return Key::End;
        case VK_CAPITAL: return Key::CapsLock;
        case VK_SCROLL: return Key::ScrollLock;
        case VK_NUMLOCK: return Key::NumLock;
        case VK_SNAPSHOT: return Key::PrintScreen;
        case VK_PAUSE: return Key::Pause;
        case VK_F1: return Key::F1;
        case VK_F2: return Key::F2;
        case VK_F3: return Key::F3;
        case VK_F4: return Key::F4;
        case VK_F5: return Key::F5;
        case VK_F6: return Key::F6;
        case VK_F7: return Key::F7;
        case VK_F8: return Key::F8;
        case VK_F9: return Key::F9;
        case VK_F10: return Key::F10;
        case VK_F11: return Key::F11;
        case VK_F12: return Key::F12;
        case VK_SHIFT: return (scancode == 0x36) ? Key::RightShift : Key::LeftShift;
        case VK_CONTROL: return (lParam & 0x01000000) ? Key::RightControl : Key::LeftControl;
        case VK_MENU: return (lParam & 0x01000000) ? Key::RightAlt : Key::LeftAlt;
        case VK_SPACE: return Key::Space;
        default:
            if (wParam >= '0' && wParam <= '9') return static_cast<Key>(static_cast<int>(Key::Num0) + (wParam - '0'));
            if (wParam >= 'A' && wParam <= 'Z') return static_cast<Key>(static_cast<int>(Key::A) + (wParam - 'A'));
            return Key::Unknown;
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WindowsWindow* win = reinterpret_cast<WindowsWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!win) return DefWindowProcW(hwnd, msg, wParam, lParam);
    
    Event e;
    switch (msg) {
        case WM_CLOSE:
            win->should_close = true;
            e.type = EventType::Close;
            win->pushEvent(e);
            return 0;
            
        case WM_SIZE:
            win->win_width = LOWORD(lParam);
            win->win_height = HIWORD(lParam);
            e.type = EventType::Resize;
            e.width = win->win_width;
            e.height = win->win_height;
            win->pushEvent(e);
            return 0;
            
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            e.type = EventType::KeyDown;
            e.key = translateKey(wParam, lParam);
            win->keys[static_cast<size_t>(e.key)] = true;
            win->pushEvent(e);
            return 0;
            
        case WM_KEYUP:
        case WM_SYSKEYUP:
            e.type = EventType::KeyUp;
            e.key = translateKey(wParam, lParam);
            win->keys[static_cast<size_t>(e.key)] = false;
            win->pushEvent(e);
            return 0;
            
        case WM_LBUTTONDOWN:
            e.type = EventType::MouseDown;
            e.mouse_button = MouseButton::Left;
            win->mouse_buttons[0] = true;
            win->pushEvent(e);
            return 0;
            
        case WM_LBUTTONUP:
            e.type = EventType::MouseUp;
            e.mouse_button = MouseButton::Left;
            win->mouse_buttons[0] = false;
            win->pushEvent(e);
            return 0;
            
        case WM_RBUTTONDOWN:
            e.type = EventType::MouseDown;
            e.mouse_button = MouseButton::Right;
            win->mouse_buttons[1] = true;
            win->pushEvent(e);
            return 0;
            
        case WM_RBUTTONUP:
            e.type = EventType::MouseUp;
            e.mouse_button = MouseButton::Right;
            win->mouse_buttons[1] = false;
            win->pushEvent(e);
            return 0;
            
        case WM_MBUTTONDOWN:
            e.type = EventType::MouseDown;
            e.mouse_button = MouseButton::Middle;
            win->mouse_buttons[2] = true;
            win->pushEvent(e);
            return 0;
            
        case WM_MBUTTONUP:
            e.type = EventType::MouseUp;
            e.mouse_button = MouseButton::Middle;
            win->mouse_buttons[2] = false;
            win->pushEvent(e);
            return 0;
            
        case WM_MOUSEMOVE:
            win->mouse_x = GET_X_LPARAM(lParam);
            win->mouse_y = GET_Y_LPARAM(lParam);
            e.type = EventType::MouseMove;
            e.mouse_x = win->mouse_x;
            e.mouse_y = win->mouse_y;
            win->pushEvent(e);
            return 0;
            
        case WM_MOUSEWHEEL:
            e.type = EventType::MouseScroll;
            e.scroll_y = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / 120.0f;
            win->pushEvent(e);
            return 0;
            
        case WM_SETFOCUS:
            e.type = EventType::Focus;
            win->pushEvent(e);
            return 0;
            
        case WM_KILLFOCUS:
            e.type = EventType::Blur;
            win->pushEvent(e);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

std::unique_ptr<HwlWindow> HwlWindow::create(const std::string& title, int width, int height) {
    auto win = std::make_unique<WindowsWindow>();
    win->win_width = width;
    win->win_height = height;
    
    // Register window class
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"HglWindowClass";
    RegisterClassExW(&wc);
    
    // Adjust window size for borders
    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    // Create window
    win->hwnd = CreateWindowExW(
        0, L"HglWindowClass",
        std::wstring(title.begin(), title.end()).c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, GetModuleHandleW(nullptr), nullptr
    );
    
    if (!win->hwnd) return nullptr;
    
    SetWindowLongPtrW(win->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win.get()));
    win->hdc = GetDC(win->hwnd);
    
    // Setup pixel format
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    
    int pf = ChoosePixelFormat(win->hdc, &pfd);
    SetPixelFormat(win->hdc, pf, &pfd);
    
    // Create OpenGL context
    win->hglrc = wglCreateContext(win->hdc);
    wglMakeCurrent(win->hdc, win->hglrc);
    
    ShowWindow(win->hwnd, SW_SHOW);
    
    return win;
}

void* getGLProc(const char* name) {
    void* p = reinterpret_cast<void*>(wglGetProcAddress(name));
    if (!p || p == reinterpret_cast<void*>(0x1) || p == reinterpret_cast<void*>(0x2) || p == reinterpret_cast<void*>(0x3)) {
        HMODULE module = LoadLibraryA("opengl32.dll");
        p = reinterpret_cast<void*>(GetProcAddress(module, name));
    }
    return p;
}

#endif // HWLS

/*==============================================================================
 * Linux/X11 Implementation
 *============================================================================*/
#ifdef HWL_LINUX

// Use X11's Window type explicitly
using XWindow = ::Window;

class X11Window : public HwlWindow {
public:
    Display* display = nullptr;
    XWindow xwindow = 0;
    GLXContext glx_context = nullptr;
    Atom wm_delete_window;
    bool should_close = false;
    int win_width = 0;
    int win_height = 0;
    bool mouse_grabbed = false;
    Cursor invisible_cursor = None;
    
    std::array<bool, static_cast<size_t>(Key::Count)> keys{};
    std::array<bool, 3> mouse_buttons{};
    int mouse_x = 0;
    int mouse_y = 0;
    
    std::queue<Event> event_queue;
    
    void createInvisibleCursor() {
        if (invisible_cursor == None) {
            // Create a 1x1 transparent pixmap
            Pixmap pixmap = XCreatePixmap(display, xwindow, 1, 1, 1);
            XColor color = {};
            invisible_cursor = XCreatePixmapCursor(display, pixmap, pixmap, &color, &color, 0, 0);
            XFreePixmap(display, pixmap);
        }
    }
    
    ~X11Window() override {
        if (invisible_cursor != None) {
            XFreeCursor(display, invisible_cursor);
        }
        if (glx_context) {
            glXMakeCurrent(display, None, nullptr);
            glXDestroyContext(display, glx_context);
        }
        if (xwindow) XDestroyWindow(display, xwindow);
        if (display) XCloseDisplay(display);
    }
    
    bool shouldClose() const override { return should_close; }
    void setShouldClose(bool close) override { should_close = close; }
    
    std::optional<Event> pollEvent() override {
        while (XPending(display) > 0) {
            XEvent xev;
            XNextEvent(display, &xev);
            handleEvent(xev);
        }
        if (!event_queue.empty()) {
            Event e = event_queue.front();
            event_queue.pop();
            return e;
        }
        return std::nullopt;
    }
    
    void swapBuffers() override {
        glXSwapBuffers(display, xwindow);
    }
    
    void makeGLCurrent() override {
        glXMakeCurrent(display, xwindow, glx_context);
    }
    
    int width() const override { return win_width; }
    int height() const override { return win_height; }
    
    void setTitle(const std::string& title) override {
        XStoreName(display, xwindow, title.c_str());
    }
    
    bool isKeyDown(Key key) const override {
        auto idx = static_cast<size_t>(key);
        return idx < keys.size() && keys[idx];
    }
    
    bool isMouseDown(MouseButton button) const override {
        auto idx = static_cast<size_t>(button);
        return idx < mouse_buttons.size() && mouse_buttons[idx];
    }
    
    void getMousePos(int& x, int& y) const override {
        // Query current mouse position directly from X11
        ::Window root_return, child_return;
        int root_x, root_y;
        unsigned int mask_return;
        XQueryPointer(display, xwindow, &root_return, &child_return,
                      &root_x, &root_y, &x, &y, &mask_return);
    }
    
    void setMousePos(int x, int y) override {
        XWarpPointer(display, None, xwindow, 0, 0, 0, 0, x, y);
        XFlush(display);
        mouse_x = x;
        mouse_y = y;
    }
    
    void grabMouse() override {
        if (!mouse_grabbed) {
            // Create invisible cursor if not already done
            createInvisibleCursor();
            
            // Grab pointer - confines to window, hides cursor, captures all mouse events
            int result = XGrabPointer(
                display, xwindow, True,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync,
                xwindow,              // Confine to this window
                invisible_cursor,     // Use invisible cursor
                CurrentTime
            );
            if (result == GrabSuccess) {
                mouse_grabbed = true;
            }
        }
    }
    
    void releaseMouse() override {
        if (mouse_grabbed) {
            XUngrabPointer(display, CurrentTime);
            XFlush(display);
            mouse_grabbed = false;
        }
    }
    
    bool isMouseGrabbed() const override {
        return mouse_grabbed;
    }
    
    Key translateKeySym(KeySym ks) {
        switch (ks) {
            case XK_Escape: return Key::Escape;
            case XK_Return: return Key::Enter;
            case XK_Tab: return Key::Tab;
            case XK_BackSpace: return Key::Backspace;
            case XK_Insert: return Key::Insert;
            case XK_Delete: return Key::Delete;
            case XK_Right: return Key::Right;
            case XK_Left: return Key::Left;
            case XK_Down: return Key::Down;
            case XK_Up: return Key::Up;
            case XK_Page_Up: return Key::PageUp;
            case XK_Page_Down: return Key::PageDown;
            case XK_Home: return Key::Home;
            case XK_End: return Key::End;
            case XK_Caps_Lock: return Key::CapsLock;
            case XK_Scroll_Lock: return Key::ScrollLock;
            case XK_Num_Lock: return Key::NumLock;
            case XK_Print: return Key::PrintScreen;
            case XK_Pause: return Key::Pause;
            case XK_F1: return Key::F1;
            case XK_F2: return Key::F2;
            case XK_F3: return Key::F3;
            case XK_F4: return Key::F4;
            case XK_F5: return Key::F5;
            case XK_F6: return Key::F6;
            case XK_F7: return Key::F7;
            case XK_F8: return Key::F8;
            case XK_F9: return Key::F9;
            case XK_F10: return Key::F10;
            case XK_F11: return Key::F11;
            case XK_F12: return Key::F12;
            case XK_Shift_L: return Key::LeftShift;
            case XK_Shift_R: return Key::RightShift;
            case XK_Control_L: return Key::LeftControl;
            case XK_Control_R: return Key::RightControl;
            case XK_Alt_L: return Key::LeftAlt;
            case XK_Alt_R: return Key::RightAlt;
            case XK_space: return Key::Space;
            default:
                if (ks >= XK_0 && ks <= XK_9) return static_cast<Key>(static_cast<int>(Key::Num0) + (ks - XK_0));
                if (ks >= XK_a && ks <= XK_z) return static_cast<Key>(static_cast<int>(Key::A) + (ks - XK_a));
                if (ks >= XK_A && ks <= XK_Z) return static_cast<Key>(static_cast<int>(Key::A) + (ks - XK_A));
                return Key::Unknown;
        }
    }
    
    void handleEvent(XEvent& xev) {
        Event e;
        switch (xev.type) {
            case ClientMessage:
                if (static_cast<Atom>(xev.xclient.data.l[0]) == wm_delete_window) {
                    should_close = true;
                    e.type = EventType::Close;
                    event_queue.push(e);
                }
                break;
                
            case ConfigureNotify:
                if (xev.xconfigure.width != win_width || xev.xconfigure.height != win_height) {
                    win_width = xev.xconfigure.width;
                    win_height = xev.xconfigure.height;
                    e.type = EventType::Resize;
                    e.width = win_width;
                    e.height = win_height;
                    event_queue.push(e);
                }
                break;
                
            case KeyPress: {
                KeySym ks = XLookupKeysym(&xev.xkey, 0);
                e.type = EventType::KeyDown;
                e.key = translateKeySym(ks);
                keys[static_cast<size_t>(e.key)] = true;
                event_queue.push(e);
                break;
            }
            
            case KeyRelease: {
                // Check for auto-repeat
                if (XEventsQueued(display, QueuedAfterReading)) {
                    XEvent next;
                    XPeekEvent(display, &next);
                    if (next.type == KeyPress && next.xkey.time == xev.xkey.time && next.xkey.keycode == xev.xkey.keycode) {
                        return; // Skip auto-repeat release
                    }
                }
                KeySym ks = XLookupKeysym(&xev.xkey, 0);
                e.type = EventType::KeyUp;
                e.key = translateKeySym(ks);
                keys[static_cast<size_t>(e.key)] = false;
                event_queue.push(e);
                break;
            }
            
            case ButtonPress:
                e.type = EventType::MouseDown;
                if (xev.xbutton.button == Button1) { e.mouse_button = MouseButton::Left; mouse_buttons[0] = true; }
                else if (xev.xbutton.button == Button2) { e.mouse_button = MouseButton::Middle; mouse_buttons[2] = true; }
                else if (xev.xbutton.button == Button3) { e.mouse_button = MouseButton::Right; mouse_buttons[1] = true; }
                else if (xev.xbutton.button == Button4) { e.type = EventType::MouseScroll; e.scroll_y = 1.0f; }
                else if (xev.xbutton.button == Button5) { e.type = EventType::MouseScroll; e.scroll_y = -1.0f; }
                event_queue.push(e);
                break;
                
            case ButtonRelease:
                e.type = EventType::MouseUp;
                if (xev.xbutton.button == Button1) { e.mouse_button = MouseButton::Left; mouse_buttons[0] = false; }
                else if (xev.xbutton.button == Button2) { e.mouse_button = MouseButton::Middle; mouse_buttons[2] = false; }
                else if (xev.xbutton.button == Button3) { e.mouse_button = MouseButton::Right; mouse_buttons[1] = false; }
                else break; // Ignore scroll release
                event_queue.push(e);
                break;
                
            case MotionNotify:
                mouse_x = xev.xmotion.x;
                mouse_y = xev.xmotion.y;
                e.type = EventType::MouseMove;
                e.mouse_x = mouse_x;
                e.mouse_y = mouse_y;
                event_queue.push(e);
                break;
                
            case FocusIn:
                e.type = EventType::Focus;
                event_queue.push(e);
                break;
                
            case FocusOut:
                e.type = EventType::Blur;
                event_queue.push(e);
                break;
        }
    }
};

std::unique_ptr<HwlWindow> HwlWindow::create(const std::string& title, int width, int height) {
    auto win = std::make_unique<X11Window>();
    win->win_width = width;
    win->win_height = height;
    
    win->display = XOpenDisplay(nullptr);
    if (!win->display) return nullptr;
    
    // Choose visual with GLX
    static int visual_attribs[] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER,
        None
    };
    
    XVisualInfo* vi = glXChooseVisual(win->display, DefaultScreen(win->display), visual_attribs);
    if (!vi) return nullptr;
    
    // Create colormap
    Colormap cmap = XCreateColormap(win->display, RootWindow(win->display, vi->screen), vi->visual, AllocNone);
    
    // Set window attributes
    XSetWindowAttributes swa{};
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | 
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                     StructureNotifyMask | FocusChangeMask;
    
    // Create window
    win->xwindow = XCreateWindow(
        win->display, RootWindow(win->display, vi->screen),
        0, 0, width, height, 0,
        vi->depth, InputOutput, vi->visual,
        CWColormap | CWEventMask, &swa
    );
    
    // Set window title
    XStoreName(win->display, win->xwindow, title.c_str());
    
    // Handle window close
    win->wm_delete_window = XInternAtom(win->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(win->display, win->xwindow, &win->wm_delete_window, 1);
    
    // Create GLX context
    win->glx_context = glXCreateContext(win->display, vi, nullptr, GL_TRUE);
    XFree(vi);
    
    if (!win->glx_context) return nullptr;
    
    glXMakeCurrent(win->display, win->xwindow, win->glx_context);
    
    // Show window
    XMapWindow(win->display, win->xwindow);
    XFlush(win->display);
    
    return win;
}

void* getGLProc(const char* name) {
    static void* libgl = nullptr;
    if (!libgl) libgl = dlopen("libGL.so.1", RTLD_LAZY | RTLD_GLOBAL);
    return dlsym(libgl, name);
}

#endif // HWL_LINUX

} // namespace hwl

#endif // HWL_IMPLEMENTATION

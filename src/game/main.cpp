/**
 * ShadowFlare - Main Entry Point
 * 
 * Reverse-engineered from ShadowFlare.exe
 * 
 * Entry: 0x0046863c (CRT entry)
 * WinMain: 0x004022e0
 */

#include "game.h"
#include <cstdio>
#include <cstring>

// ============== Global Variables ==============
// These match the original exe layout starting at 0x00482770

HINSTANCE g_hInstance = NULL;    // 0x00482770
LPARAM g_lastLParam = 0;         // 0x00482774
SFWindow g_Window;               // 0x00482778
HWND g_hWnd = NULL;              // Alias for g_Window.m_hwnd

// State flags
int g_cursorVisible = 0;    // 0x0048D8D4
int g_imeEnabled = 0;       // 0x0048D8CC

// Game mode (0=SP, 1=Client, 2=Server)
int g_gameMode = 0;         // 0x00482DB0

// Screenshot requested flag
int g_screenshotRequested = 0;  // 0x0048D71C

// Forward declarations
LRESULT CALLBACK SFWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**
 * Key down handler
 * Address: 0x00402840
 * 
 * From decompilation:
 * - VK_RETURN (0x0D / 13): Toggle cursor visibility state
 * - VK_SNAPSHOT (0x2C / 44): Set screenshot flag at 0x48d71c = 1
 * 
 * @param vkey Virtual key code
 */
void SFOnKeyDown(WPARAM vkey)
{
    switch (vkey)
    {
        case VK_RETURN:  // 0x0D / 13
            // Toggle cursor state at g_Window + some offset
            // Original: mov ecx, 0x482778; call 0x4026e0
            break;
            
        case VK_SNAPSHOT:  // 0x2C / 44 - PrintScreen
            // Set screenshot flag
            g_screenshotRequested = 1;
            break;
    }
}

/**
 * Key up handler
 * Address: 0x004026d0
 * 
 * Currently a stub (just ret 4)
 */
void SFOnKeyUp(WPARAM vkey)
{
    (void)vkey;
    // Empty stub in original
}

/**
 * Check if another instance is already running
 * Address: 0x00401550
 * 
 * Creates a named mutex. If ERROR_ALREADY_EXISTS (183/0xB7), 
 * another instance is running.
 * 
 * @param mutexName Name of the mutex (e.g., "SHADOW FLARE for WIN  Denyusha")
 * @return true if another instance exists, false otherwise
 */
bool CheckSingleInstance(const char* mutexName)
{
    CreateMutexA(NULL, TRUE, mutexName);
    return (GetLastError() == ERROR_ALREADY_EXISTS);
}

/**
 * Create game window
 * Address: 0x00402520
 * 
 * Sets up WNDCLASS and creates the main 640x480 game window.
 * Called with 'this' = 0x482778 (g_Window)
 * 
 * @return true if window created successfully
 */
bool SFWindow::CreateGameWindow()
{
    // Copy class name "SHADOW_FLARE" to +0x08
    strcpy(m_className, "SHADOW_FLARE");
    
    // Copy window title to +0x108
    strcpy(m_windowTitle, "ShadowFlare for Window98/Me/2000");
    
    // Set up WNDCLASS at +0x508
    m_wndClass.style = CS_DBLCLKS;                  // 0x08
    m_wndClass.lpfnWndProc = SFWndProc;             // 0x402b30 in original
    m_wndClass.cbClsExtra = 0;
    m_wndClass.cbWndExtra = 0;
    m_wndClass.hInstance = g_hInstance;             // From global
    m_wndClass.hIcon = LoadIconA(g_hInstance, MAKEINTRESOURCEA(101));
    m_wndClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
    m_wndClass.hbrBackground = NULL;
    m_wndClass.lpszMenuName = m_className;
    m_wndClass.lpszClassName = m_className;
    
    RegisterClassA(&m_wndClass);
    
    // Get position and style from RKC_DBFCONTROL
    // In original: GetPosition and GetStyle called
    // For now, use defaults: centered, WS_OVERLAPPEDWINDOW
    
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exStyle = 0;
    
    // CreateWindowExA - 640x480 window
    m_hwnd = CreateWindowExA(
        exStyle,                    // dwExStyle
        m_className,                // lpClassName
        m_windowTitle,              // lpWindowName
        style,                      // dwStyle
        x, y,                       // X, Y
        640, 480,                   // nWidth, nHeight
        NULL,                       // hWndParent
        NULL,                       // hMenu
        g_hInstance,                // hInstance
        NULL                        // lpParam
    );
    
    // Get menu handle
    m_hMenu = GetMenu(m_hwnd);
    
    // Clear window flags
    m_windowFlags = 0;
    
    // Store in global alias
    g_hWnd = m_hwnd;
    
    return (m_hwnd != NULL);
}

/**
 * Window Procedure
 * Address: 0x00402b30
 * 
 * Message handling for the main game window.
 * Handles: WM_DESTROY, WM_CLOSE, WM_SETCURSOR, WM_KEYDOWN, WM_KEYUP,
 *          WM_TIMER, mouse messages, etc.
 */
LRESULT CALLBACK SFWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        // WM_CREATE (0x01) - case 4 in switch
        case WM_CREATE:
            // Store lParam at 0x482774
            g_lastLParam = lParam;
            return 0;
            
        // WM_DESTROY (0x02) - case 3 in switch
        case WM_DESTROY:
            // Call fcn.00402b00 - cleanup
            PostQuitMessage(0);
            return 0;
            
        // WM_CLOSE (0x10) - case 2 in switch
        case WM_CLOSE:
            // Call fcn.004026c0 with arg 1
            DestroyWindow(hwnd);
            return 0;
            
        // WM_SETCURSOR (0x20)
        case WM_SETCURSOR:
        {
            // Get cursor position
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(g_hWnd, &pt);
            
            // Check if cursor is within game area (0-639, 0-479)
            if (pt.x >= 0 && pt.y >= 0 && pt.x <= 639 && pt.y <= 479)
            {
                // Check g_cursorVisible flag at 0x48d8d4
                if (g_cursorVisible < 0)
                {
                    // Hide cursor
                    SetCursor(NULL);
                    return 0;
                }
            }
            // Fall through to default
            break;
        }
        
        // WM_KEYDOWN (0x100 / 256)
        case WM_KEYDOWN:
            // Check g_imeEnabled at 0x48d8cc
            if (g_imeEnabled == 0)
            {
                SFOnKeyDown(wParam);
            }
            return 0;
            
        // WM_KEYUP (0x101 / 257)
        case WM_KEYUP:
            SFOnKeyUp(wParam);
            return 0;
            
        // WM_TIMER (0x113 / 275)
        case WM_TIMER:
            // Timer handling
            return 0;
            
        // WM_LBUTTONDOWN (0x201 / 513)
        case WM_LBUTTONDOWN:
        {
            // Extract mouse coordinates from lParam
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            // Call fcn.004028a0 - left click handler
            (void)x; (void)y;
            return 0;
        }
        
        // WM_LBUTTONUP (0x202 / 514)
        case WM_LBUTTONUP:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            // Call fcn.004028a0
            (void)x; (void)y;
            return 0;
        }
        
        // WM_RBUTTONDOWN (0x204 / 516)
        case WM_RBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            // Call fcn.00402900 - right click handler
            (void)x; (void)y;
            return 0;
        }
        
        // WM_RBUTTONUP (0x205 / 517)
        case WM_RBUTTONUP:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            // Call fcn.00402900
            (void)x; (void)y;
            return 0;
        }
        
        // WM_MBUTTONDOWN (0x207 / 519)
        case WM_MBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            // Call fcn.00402900 - middle click uses same handler as right
            (void)x; (void)y;
            return 0;
        }
    }
    
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/**
 * WinMain - Main application entry point
 * Address: 0x004022e0
 * 
 * Flow:
 * 1. Check mutex for single instance
 * 2. Store hInstance at 0x482770
 * 3. Call LoadConfig (0x401eb0) with ecx=0x482770
 * 4. Process command line (0x4014a0)
 * 5. Call CreateGameWindow (0x402520) with ecx=0x482778
 * 6. Show/Update/Focus window
 * 7. Call InitGame (0x4016b0) with ecx=0x482770
 * 8. Message loop
 */
int WINAPI SFWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    (void)hPrevInstance;  // Unused
    
    // Check for existing instance (0x4022e3-0x4022fa)
    // Mutex name: "SHADOW FLARE for WIN  Denyusha" (at 0x47d4dc)
    if (CheckSingleInstance("SHADOW FLARE for WIN  Denyusha"))
    {
        return 0;
    }
    
    // Store hInstance in global (0x402306: mov [0x482770], eax)
    g_hInstance = hInstance;
    
    // TODO: Call LoadConfig (0x401eb0)
    // In original: mov ecx, 0x482770; call 0x401eb0
    
    // Process command line if provided (0x402310-0x40231e)
    if (lpCmdLine && lpCmdLine[0])
    {
        // TODO: Call ProcessCommandLine
    }
    
    // Create game window (0x402327-0x40232c)
    // In original: mov ecx, 0x482778; call 0x402520
    if (!g_Window.CreateGameWindow())
    {
        return 0;
    }
    
    // Show and update window (0x40233d-0x40234f)
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    SetFocus(g_hWnd);
    
    // TODO: Call InitGame (0x4016b0)
    // In original: mov ecx, 0x482770; call 0x4016b0
    
    // Main message loop (0x402366-0x4023b8)
    MSG msg;
    while (true)
    {
        // Check for messages without blocking (PeekMessage)
        if (PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
            // Get and process message
            if (!GetMessageA(&msg, NULL, 0, 0))
            {
                break;  // WM_QUIT received
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        else
        {
            // No messages - wait for next message
            WaitMessage();
        }
    }
    
    return (int)msg.wParam;
}

// SFWindow constructor
SFWindow::SFWindow()
{
    memset(this, 0, sizeof(SFWindow));
}

// SFWindow destructor
SFWindow::~SFWindow()
{
    // Cleanup
}

// SFGame constructor
SFGame::SFGame()
{
    // Nothing to initialize - all state is in globals
}

// SFGame destructor
SFGame::~SFGame()
{
    // Cleanup
}

/**
 * Standard WinMain entry point
 */
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    return SFWinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

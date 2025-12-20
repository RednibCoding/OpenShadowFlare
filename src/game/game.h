/**
 * ShadowFlare - Game Main Header
 * 
 * Reverse-engineered from ShadowFlare.exe (2003)
 * 
 * The game uses multiple global objects, not one monolithic structure:
 * 
 * 0x00482770: HINSTANCE g_hInstance     - Application instance
 * 0x00482774: LPARAM g_lastLParam       - Last WM_CREATE lParam
 * 0x00482778: SFWindow g_Window         - Window object (~0xB100 bytes)
 * 0x00482D10: RKC_UPDIB* g_pUpdib       - Sprite system
 * 0x00482D18: RKC_DBFCONTROL* g_pDbfCtl - Double buffer
 * 0x00482D28: RKC_DSOUND* g_pDsound     - Sound system
 * 0x0048D8B8: int g_windowStyleIndex
 * 0x0048D8CC: int g_imeEnabled
 * 0x0048D8D4: int g_cursorVisible
 */

#ifndef SHADOWFLARE_GAME_H
#define SHADOWFLARE_GAME_H

#include <windows.h>

// Forward declarations for RKC libraries
class RKC_DIB;
class RKC_UPDIB;
class RKC_DBFCONTROL;
class RKC_DSOUND;
class RKC_RPGSCRN;
class RKC_RPG_TABLE;
class RKC_RPG_SCRIPT;
class RKC_RPG_AICONTROL;
class RKC_NETWORK;
class RKC_FONTMAKER;

/**
 * Window Object Layout (at 0x00482778)
 * 
 * This object contains window-related data and is used as 'this'
 * in CreateGameWindow and WndProc related functions.
 * 
 * +0x000: HWND hwnd                    - Main window handle
 * +0x004: HMENU hMenu                  - Main menu (if any)
 * +0x008: char className[256]          - "SHADOW_FLARE" (at 0x482780)
 * +0x108: char windowTitle[256]        - "ShadowFlare for Window98/Me/2000"
 * +0x508: WNDCLASS wndClass            - Window class structure (40 bytes)
 *         +0x508: UINT style           = 0x08 (CS_DBLCLKS)
 *         +0x50C: WNDPROC lpfnWndProc  = 0x402b30
 *         +0x510: int cbClsExtra       = 0
 *         +0x514: int cbWndExtra       = 0
 *         +0x518: HINSTANCE hInstance
 *         +0x51C: HICON hIcon          = LoadIcon(hInstance, 101)
 *         +0x520: HCURSOR hCursor      = LoadCursor(NULL, IDC_ARROW)
 *         +0x524: HBRUSH hbrBackground = NULL
 *         +0x528: LPCSTR lpszMenuName  = className (0x482780)
 *         +0x52C: LPCSTR lpszClassName = className (0x482780)
 * +0x530: (padding)
 * +0x534: int windowFlags              = 0
 * 
 * Game subsystems (accessed from InitGame at 0x004016b0):
 * +0x57C: RKC_DIB cursorBitmap         - 24x24 cursor bitmap (at +0x584 from window base)
 * +0x598: RKC_UPDIB* pUpdib            - Sprite system (at +0x5A0 from window base)
 * +0x5A0: RKC_DBFCONTROL* pDbfControl  - Double buffer (at +0x5A8 from window base)
 * +0x5AC: void* pUnknownObj            - Unknown object (destroyed before DBFCONTROL)
 * +0x5B0: RKC_NETWORK* pNetwork        - Network system (at +0x5B4 from window base)
 * +0x5B8: RKC_DSOUND* pDsound          - Sound system
 * +0x5BC: void* state0Handler          - State 0 handler object
 * +0x620: void* state1Handler          - State 1 handler object  
 * +0x684: void* state2Handler          - State 2 handler object
 * +0x59C: int gameState                - Current game state (0, 1, or 2)
 * +0xB144: void* pUnknownObj2          - Another unknown object
 * +0xB154: HIMC immContext             - IMM input context
 * 
 * Game States (switch at 0x004023d0):
 *   State 0: Title screen / menu (handler at 0x420df0)
 *   State 1: Loading / transition (handler at 0x421bf0)
 *   State 2: Main gameplay (handler at 0x41d880)
 * 
 * Shutdown sequence (0x00401b90):
 *   1. Stop DBFCONTROL
 *   2. Wait for drawing to complete
 *   3. Cleanup state handlers (0x5bc, 0x620, 0x684)
 *   4. Release DSOUND
 *   5. Release IMM context
 *   6. Destroy subsystems: DSOUND, NETWORK, DBFCONTROL, RPGSCRN, UPDIB
 *   7. Destroy object at +0xB144
 * 
 * Configuration from SFlare.Cfg (LoadConfig at 0x00401eb0):
 * Uses 'this' = 0x482770 which is g_hInstance address, so offsets are:
 * +0xADB8: int cfgOption1              - Config value 1 (at 0x482770 + 0xADB8)
 * +0xADBC: int cfgOption2              - etc.
 * ...
 * +0xB140: int configLoaded
 * +0xB14C: HIMC immContext             - IMM input context
 * +0xB16C: int audioSetting1           - Audio volume (clamped -10000..0)
 * +0xB170: int audioSetting2
 */
/**
 * SFWindow - Window management object (at 0x00482778)
 * 
 * This is the window object used for CreateGameWindow and message handling.
 */
class SFWindow {
public:
    SFWindow();
    ~SFWindow();
    
    // Window creation (0x00402520)
    bool CreateGameWindow();
    
    // Accessors
    HWND GetHWND() const { return m_hwnd; }
    
public:
    // === Layout must match original binary exactly ===
    
    // +0x000: Window handles
    HWND        m_hwnd;                     // +0x000
    HMENU       m_hMenu;                    // +0x004
    
    // +0x008: Window strings
    char        m_className[256];           // +0x008
    char        m_windowTitle[256];         // +0x108
    
    // +0x508: WNDCLASS structure (40 bytes)
    WNDCLASSA   m_wndClass;                 // +0x508
    
    // +0x530: Window state
    char        m_pad1[4];                  // +0x530
    int         m_windowFlags;              // +0x534
    
    // ... more fields for subsystems and config follow ...
};

/**
 * SFGame - Main game controller
 * 
 * Functions that use 'this' = 0x482770 (g_hInstance address)
 * This includes LoadConfig, InitGame, etc.
 */
class SFGame {
public:
    SFGame();
    ~SFGame();
    
    // Load configuration (0x00401eb0)
    bool LoadConfig(HINSTANCE hInstance);
    
    // Command line processing (0x004014a0)
    void ProcessCommandLine(const char* cmdLine);
    
    // Game initialization (0x004016b0)
    bool InitGame();
    
    // Main game loop
    int Run(int nCmdShow);
    
public:
    // Note: The actual layout starts at g_hInstance (0x482770)
    // and the SFWindow object starts 8 bytes later at 0x482778
    
    // These are actually separate globals, not a single struct:
    // HINSTANCE at 0x482770
    // LPARAM at 0x482774  
    // SFWindow at 0x482778
};

// ============== Global Variables ==============

// Global HINSTANCE (at 0x00482770)
extern HINSTANCE g_hInstance;

// Last lParam from WM_CREATE (at 0x00482774)
extern LPARAM g_lastLParam;

// Global window object (at 0x00482778)
extern SFWindow g_Window;
extern HWND g_hWnd;  // Alias for g_Window.m_hwnd

// RKC subsystem pointers
extern RKC_UPDIB* g_pUpdib;          // at 0x00482D10
extern RKC_DBFCONTROL* g_pDbfControl; // at 0x00482D18
extern RKC_DSOUND* g_pDsound;         // at 0x00482D28

// State flags
extern int g_windowStyleIndex;  // at 0x0048D8B8
extern int g_imeEnabled;        // at 0x0048D8CC
extern int g_cursorVisible;     // at 0x0048D8D4

// Game mode (at 0x00482DB0)
// 0 = Single player
// 1 = Network client
// 2 = Network server
extern int g_gameMode;

// Screenshot flag (at 0x0048D71C)
extern int g_screenshotRequested;

// ============== Functions ==============

/**
 * WinMain equivalent (at 0x004022e0)
 */
int WINAPI SFWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
);

/**
 * Check if another instance is running (at 0x00401550)
 */
bool CheckSingleInstance(const char* mutexName);

/**
 * Window procedure (at 0x00402b30)
 */
LRESULT CALLBACK SFWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**
 * Game state machine (at 0x004023d0)
 * Dispatches to state-specific handlers based on g_Window.m_gameState
 */
void SFUpdateGameState(int param1, int param2);

/**
 * Key down handler (at 0x00402840)
 * - VK_RETURN (Enter): Toggle cursor state
 * - VK_SNAPSHOT (PrintScreen): Set screenshot flag
 */
void SFOnKeyDown(WPARAM vkey);

/**
 * Key up handler (at 0x004026d0)
 * Currently a stub (just returns)
 */
void SFOnKeyUp(WPARAM vkey);

/**
 * Shutdown/cleanup (at 0x00401b90)
 * Called when game is closing - releases all subsystems
 */
void SFShutdown();

#endif // SHADOWFLARE_GAME_H

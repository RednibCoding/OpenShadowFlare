# ShadowFlare.exe Reverse Engineering

## Overview

The ShadowFlare.exe is a 532KB Win32 GUI application with ~1043 functions.
It was compiled January 14, 2003 and uses custom DLLs (RK_FUNCTION, RKC_*) for most heavy lifting.

Developer: **Missinglink**  
Publisher: **Denyusha**  
Window Class: `SHADOW_FLARE`  
Window Title: `ShadowFlare for Window98/Me/2000`  
Registry Key: `HKEY_LOCAL_MACHINE\SOFTWARE\Missinglink\ShadowFlare`

## DLL Import Summary

| DLL | Import Count | Purpose |
|-----|--------------|---------|
| RKC_RPGSCRN | 88 | RPG screen rendering, objects, maps |
| RKC_NETWORK | 38 | Multiplayer networking |
| RKC_RPG_SCRIPT | 25 | Scenario scripting |
| RKC_RPG_AICONTROL | 20 | AI/NPC behavior |
| RKC_DBFCONTROL | 20 | Double-buffer control |
| RKC_UPDIB | 17 | Sprite rendering |
| RKC_DIB | 14 | Bitmap/graphics |
| RKC_RPG_TABLE | 9 | Data tables |
| RKC_DSOUND | 9 | DirectSound audio |
| RKC_MEMORY | 6 | Memory management |
| RKC_FILE | 6 | File I/O |
| RKC_FONTMAKER | 4 | Font rendering |
| RKC_WINDOW | 2 | Window management |

Total: 258 RKC DLL imports

## Key Addresses

| Address    | Description |
|------------|-------------|
| 0x00401550 | CheckSingleInstance - Mutex check for single instance |
| 0x004014a0 | ProcessCommandLine - Parse command line args |
| 0x00401eb0 | LoadConfig - Load SFlare.Cfg configuration |
| 0x004016b0 | InitGame - Initialize all game subsystems |
| 0x004022e0 | WinMain - Main entry point |
| 0x00402520 | CreateGameWindow - Window class and creation |
| 0x00402b30 | WndProc - Window procedure (1142 bytes) |
| 0x00402840 | KeyDownHandler - Keyboard input |
| 0x004026d0 | KeyUpHandler - Key release (stub) |
| 0x004028a0 | LeftClickHandler - Left mouse button |
| 0x00402900 | RightClickHandler - Right/middle mouse |
| 0x004023d0 | UpdateGameState - Game state machine (switches on state 0/1/2) |
| 0x00401b90 | Shutdown - Cleanup all subsystems |
| 0x0041d970 | State2Handler_Main - Main gameplay update (2936 bytes) |
| 0x00420df0 | State0Handler - Title/menu state |
| 0x00421bf0 | State1Handler - Loading/transition state |
| 0x0041d880 | State2Handler - Main gameplay dispatch |
| 0x0046863c | CRT Entry - Runtime startup |

## Global Variables

The game uses separate globals, not one monolithic struct:

| Address    | Type | Description |
|------------|------|-------------|
| 0x00482770 | HINSTANCE | Application instance handle |
| 0x00482774 | LPARAM | Last lParam from WM_CREATE |
| 0x00482778 | SFWindow | Window object (~0xB100 bytes) |
| 0x00482D10 | RKC_UPDIB* | Sprite system |
| 0x00482D18 | RKC_DBFCONTROL* | Double buffer control |
| 0x00482D20 | RKC_RPGSCRN_CHARANIMBLOCK* | Character animation |
| 0x00482D24 | RKC_NETWORK* | Network system |
| 0x00482D28 | RKC_DSOUND* | Sound system |
| 0x00482DB0 | int | Game mode (0=SP, 1=Client, 2=Server) |
| 0x0048D71C | int | Screenshot requested flag |
| 0x0048D8B8 | int | Window style index |
| 0x0048D8CC | int | IME enabled flag |
| 0x0048D8D4 | int | Cursor visibility flag |

## Game State Machine (0x004023d0)

The game uses a state machine with 3 states stored at SFWindow+0x59C:

| State | Handler   | Description |
|-------|-----------|-------------|
| 0     | 0x420df0  | Title screen / Main menu |
| 1     | 0x421bf0  | Loading / Transition |
| 2     | 0x41d880  | Main gameplay |

State transitions:
- State 0→1: Start new game or continue
- State 1→2: Finished loading scenario
- State 2→0: Return to menu

## SFWindow Object Layout (at 0x00482778)

```
+0x000: HWND hwnd
+0x004: HMENU hMenu
+0x008: char className[256]        "SHADOW_FLARE"
+0x108: char windowTitle[256]      "ShadowFlare for Window98/Me/2000"
+0x508: WNDCLASS wndClass (40 bytes)
+0x534: int windowFlags
+0x584: RKC_DIB cursorBitmap
+0x59C: int gameState              (0, 1, or 2)
+0x5A0: RKC_UPDIB* pUpdib
+0x5A4: RKC_RPGSCRN* pRpgScrn
+0x5A8: RKC_DBFCONTROL* pDbfCtl
+0x5AC: void* pUnknownObj
+0x5B4: RKC_NETWORK* pNetwork
+0x5B8: RKC_DSOUND* pDsound
+0x5BC: State0Handler object
+0x620: State1Handler object
+0x684: State2Handler object
+0xB144: void* pUnknownObj2
+0xB154: HIMC immContext
```

## Shutdown Sequence (0x00401b90)

1. RKC_DBFCONTROL::StopAll()
2. Wait for drawing complete (spin on drawing flag)
3. State handler cleanup (at +0x5BC, +0x620, +0x684)
4. RKC_DSOUND::Release()
5. ImmReleaseContext()
6. Destroy DSOUND, NETWORK, DBFCONTROL, RPGSCRN, UPDIB
7. Destroy object at +0xB144

## Key Function Tables

### Initialization
| Address    | Function Name | Description |
|------------|---------------|-------------|
| 0x004022e0 | WinMain       | Entry point, message loop |
| 0x00401550 | CheckSingleInstance | Mutex-based single instance check |
| 0x00401eb0 | LoadConfig    | Load SFlare.Cfg |
| 0x004014a0 | ProcessCommandLine | Parse command line args |
| 0x00402520 | CreateGameWindow | Register class, create window |
| 0x004016b0 | InitGame      | Initialize all subsystems |

### Game Loop
| Address    | Function Name | Description |
|------------|---------------|-------------|
| 0x004023d0 | UpdateGameState | State machine dispatcher |
| 0x00420c40 | State0_Init   | Initialize title/menu state |
| 0x00420df0 | State0_Update | Update title/menu |
| 0x00421a00 | State1_Init   | Initialize loading state |
| 0x00421bf0 | State1_Update | Update loading (cleanup) |
| 0x0041d3f0 | State2_Init   | Initialize gameplay state |
| 0x0041d880 | State2_Update | Dispatch gameplay logic |
| 0x0041d970 | State2_Main   | Main gameplay (2936 bytes!) |

### Input Handling
| Address    | Function Name | Description |
|------------|---------------|-------------|
| 0x00402b30 | WndProc       | Window message handler |
| 0x00402840 | KeyDownHandler | VK_RETURN, VK_SNAPSHOT |
| 0x004026d0 | KeyUpHandler  | (stub) |
| 0x004028a0 | LeftClickHandler | Mouse left button |
| 0x00402900 | RightClickHandler | Mouse right/middle |

### Scenario/Loading
| Address    | Function Name | Description |
|------------|---------------|-------------|
| 0x00426200 | LoadScenario  | Main scenario loader (4846 bytes) |
| 0x00426160 | LoadScenarioData | Helper for scenario loading |
| 0x004021b0 | FindSaveSlot  | Find next free save slot |

### Major Functions (by size)
| Address    | Size (bytes) | Description |
|------------|--------------|-------------|
| 0x00429ec0 | 20119 | CommandDispatcher - Command/Event dispatcher (huge switch) |
| 0x00430f80 | 13677 | ScriptInterpreter - 75 opcodes |
| 0x00462f80 | 9247  | LoadItemData - magic SFItemDataV0000 |
| 0x004103c0 | 7773  | OptionsMenu - Settings menu |
| 0x0044cac0 | 7527  | LoadGame - load save file, XOR decrypt |
| 0x00414990 | 7221  | ObjectNpcDisplay - via RKC_RPGSCRN_OBJECTDISP |
| 0x00427b50 | 6780  | LoadScenarioMct - magic MCED DATA v0000 |
| 0x0040aed0 | 6758  | ItemStatsDisplay - format item stat bonuses |
| 0x0044b580 | 5429  | SaveGame - write save file, XOR encrypt |
| 0x00409a60 | 5212  | StatusScreenDisplay |
| 0x00405750 | 4936  | CharacterStatusDisplay - element bonuses |
| 0x00426200 | 4846  | LoadScenario - main scenario loader |
| 0x00423ca0 | 4306  | ScenarioLoader_Phase2 - called from State1 |
| 0x00446320 | 3898  | (UI/input related - needs analysis) |
| 0x0041afc0 | 3680  | NetworkServerHandler - RKC_NETWORK packets |
| 0x00441c00 | 3462  | (needs analysis) |
| 0x004039f0 | 3437  | SpritePacketSetup - RKC_UPDIB::SetPacket |
| 0x0041d970 | 2936  | State2Handler_Main - main gameplay update |
| 0x00421e10 | 2710  | CharacterCreation - class/gender select |
| 0x004239b0 | ~500  | LoadSaveSlotInfo - read save headers |
| 0x004021b0 | 129   | FindSaveSlot - find free save slot |

### Cleanup
| Address    | Function Name | Description |
|------------|---------------|-------------|
| 0x00401b90 | Shutdown      | Release all subsystems |

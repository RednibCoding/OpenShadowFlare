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
| 0x00402920 | PaintInitialLoadingScreen - cached loading page and overlay |
| 0x004030F0 | RenderWorld - ground, shadow, and depth-sorted object passes |
| 0x004023d0 | UpdateGameState - Game state machine (switches on state 0/1/2) |
| 0x00401b90 | Shutdown - Cleanup all subsystems |
| 0x0041d970 | BootstrapGameplay - first-frame scenario setup (2936 bytes) |
| 0x00420df0 | LeaveTitleState |
| 0x00421bf0 | LeaveCharacterSelectState |
| 0x0041d880 | LeaveGameplayState |
| 0x00417bd0 | RenderLoadingScreen - gameplay loading presentation |
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
| 0x0048D8D4 | int | Initial loading state (-1 hidden, 0/1 loading, 2 confirm, 3 accepted) |

## Game State Machine (0x004023d0)

The game uses a state machine with 3 states stored at SFWindow+0x59C:

| State | Handler   | Description |
|-------|-----------|-------------|
| 0     | 0x420df0  | Title screen / Main menu |
| 1     | 0x421bf0  | Character selection / save selection |
| 2     | 0x41d880  | Main gameplay |

State transitions:
- State 0→1: Open new-character or saved-game selection
- State 1→2: Confirm a single-player or network mode
- State 2→0: Return to menu

The visible loading screen is a sub-state of gameplay rather than top-level
state 1. At application startup, the game decodes `Waiting.njp` patterns 0, 1,
3, and 2 into cached DIBs. Initial single-player entry paints the Episode 1
background and loading label through `0x00402920`. Once scenario setup marks
the world ready, the label is replaced by a 16-pixel horizontally moving
arrow; Return or a click in its bottom-right rectangle continues into the
world.

`0x00417bd0` is a different loading presenter used later in gameplay. It draws
`Waiting.njp` pattern 4 or an alternate `VisualNN.njp`, fades it over 120
frames, and uses `WaitIcon.njp`.

The initial scenario map is loaded by the large transition routine at
`0x00426200`. The `f00_01.Lst` indices are preserved across ground and object
records: NJP entries hold visible patterns and their following SDW entries hold
one-bit shadows. `0x004030f0` builds separate shadow and visible-object lists,
sorts them using status classes and judgement rectangles, and inserts dynamic
actors into the visible depth order. The portable first-world slice now
reconstructs that pipeline for static OBL scenery and the player.

The MCT loader at `0x00427b50` first reads a 16-byte
`MCED DATA v0000\x1a` signature, two 260-byte paths, two unknown 32-bit
values, the music index, and a 256-byte title. Its variable entity section is
followed near EOF by a count and 16-byte entry records in key, world X, world
Y, direction order. `0x00427930` searches those records by key. The portable
loader now uses scenario `00000000`'s map path and entry key zero rather than
embedding `f00_01`, (`89898`, `2811`), direction 3, and music index 0 in
`WorldScene`.

Gameplay pointer selection is handled by `0x0040ee70`. For an ordinary person
it projects the actor's feet, subtracts the MCT label height, draws a
half-transparent black plate around the centered 6-by-12 name, then draws a
black one-pixel shadow and the actor's configured name color. The selected
actor's visible RGB strengths each receive `+300`. Values above 1000 do not
multiply the palette color: RKC_UPDIB moves each channel toward white, which
produces the pale hover tint seen in the retail game.

Message layout at `0x00456550` counts ASCII and Shift-JIS glyph widths, adds
an eight-pixel text-box inset, and positions actor messages above the same MCT
label anchor. `0x00456bb0` surrounds that box with the nine-pixel
`Hukidasi.njp` frame and draws its tail from
`System\Game\Pattern\Hukidasi.njp`, then places the 6-by-12 `Font01.njp` text
at a four-pixel inset. The tail overlaps the bottom four frame pixels before
extending into the world. The portable renderer follows this path for actor
messages instead of using a fixed screen-bottom dialogue box. Message events
retain their current script character, which also anchors Syria's initial
branch even though it does not run an explicit facing command.

The same layout function gives `~` a special meaning when message flag
`0x40000000` is set. The tildes are removed from the displayed text and each
enclosed run is stored as a clickable character range with its line and
columns. The companion messages use opcode-2 mode one, pass a writable script
operand for the selected zero-based range, and supply an initial selection.
Dune's first menu starts on range three, `QUIT`. These bytes are UI control
markup, not part of the English message.

Mode one also carries companion follow-up text with initial range `-1`.
Those messages have no `~` spans and are acknowledged like ordinary speech;
they do not write a choice result. Harley's `Explanation` branch uses this
form for messages `1000057` and `1000058`, then reaches the same status-one
release chain. A non-negative initial range is therefore part of the choice
contract, not just a visual default.

Pointer handling at `0x00457fa0` replaces the current range only when
`0x00457bb0` finds the pointer inside one of those spans. Moving away leaves
the last range selected. `0x00456bb0` draws that range in red
`(255, 0, 0)` and the other ranges in gray `(96, 96, 96)`. The portable
conversation state now owns the same selection, so hover rendering and the
option returned to the interpreter use one set of range indices.

World interaction goes through `0x00449240`. It measures the shortest gap
between the player and target judgement rectangles with `0x004143c0`, rather
than comparing their center points. The player's initial interaction distance
at offset `0x3f4` is `0x9f` (159 world units). A target outside that distance
starts movement-controller mode one and is followed as it moves. Once the
rectangle gap reaches 159, the player faces the actor and starts that actor's
status-zero script. This is why clicking a distant person in retail walks
toward them instead of opening a remote conversation or simply rejecting the
click.

`0x00454210` initializes the executable's shared movement controller and
`0x00454930` advances it. It is not an A* route search. A direct collision
sweep is followed by stateful cardinal obstacle-edge steering. Controller
modes cover fixed points, scenario actors, other players, bounded wandering,
and related approach behavior. Calls from the player, PEOPLE actors, and enemy
actors all reach this controller. `RKC_RPG_AICONTROL` chooses enemy intent and
parameters; it does not contain a second enemy pathfinder.

The portable `MovementController` now keeps the same pair of cardinal
movement and wall directions between updates. A blocked direct sweep selects
an edge and blocked edge movement rotates the pair. Once the actor has made
net progress past the contact point and the next direct step is clear, it
retries the direct sweep. This prevents a later tree from keeping the actor
attached to an earlier obstacle. Fixed ground targets stop at the collision
edge when the requested center itself is blocked; actor-follow targets remain
active until their judgement rectangles enter interaction range. Player and
PEOPLE movement share this owner, including their facing direction while
detouring. Remote Town fixtures cover the route from the initial entry to
Kerberos, the irregular sacks footprint beside Ostare, and several routes
through separate blocker groups. Dynamic actor collision masks and later
controller modes remain follow-up work.

Primary-button input has two retail behaviors. A press and release is a
latched destination click. Keeping the button down continuously replaces the
destination with the live pointer position, but releasing after that held
state cancels movement immediately. The portable gameplay state tracks that
distinction explicitly rather than treating every held frame as another
independent click. A press consumed by a speech bubble stays UI-owned until
button release, even when selecting the option closes that bubble. It cannot
be reinterpreted as a held ground command on the following update.

Portable gameplay still updates at the retail 30 Hz cadence, while the window
is presented at 60 Hz. Rendering the current simulation snapshot twice made
camera scrolling visibly step at a constant rate. The runtime now keeps the
previous and current actor positions and interpolates only their render
positions and the camera between updates. Collision, scripts, animation
counters, and all other game state remain on the fixed 30 Hz clock.

The variable section at `0x324` begins with three counted ID lists, followed
by counted runtime entity groups. The object and `PEOPLE` groups share IDs,
optional names and colors, label height, position, judgement, direction,
initial CAF part overrides, and optional fixed-capacity part/color arrays
before their type-specific tails. The portable decoder now reads all seven
Remote Town people records and the bounded-wander fields at the start of their
tails.
Later entity groups and the final two unnamed people fields are still open.

All seven people records are instantiated from that table. Resource lookup at
`0x00455ee0` resolves each ID to its zero-padded `Character\PEOPLE` directory;
the four animals share resources `01000000` and `01000001` exactly as named by
the MCT. The first record creates Ostare through the type-one path constructed
at `0x0045d020`. `0x0045d620` draws idle chart zero using MCT direction 7 and
advances its frame counter once per game update. After the tail's 30-update
pause, `0x0045d150` starts movement-controller mode three. That mode chooses an
inclusive random point inside the spawn-relative rectangle, while
`0x0045d9f0` draws chart one and moves at 10 world units per update until
arrival or the tail's 30-update limit. The MCT's custom mask disables parts 4
and 5, leaving the shadow and two visible frame-zero cells rather than drawing
every CAF layer.

Player CAF parts are not independent actors that should all be drawn.
`0x00444ca0` rebuilds an enable table on every appearance refresh: entries 0
and 1 are the base body and shadow, while equipped items select additional
armor and weapon entries. The new-player MCT record starts at direction 3, and
`Animation00.Sdw` supplies the corresponding one-bit player shadow.

The same MCT stores music index 0 for Remote Town. `0x004275e0` maps scenario
music indices to `System\Game\Music\BGM%02d.Voc`, loads the selected container
into voice slot 500, and resets its start counter. `0x004275a0` starts sample
zero looping on the following gameplay update with the configured BGM volume.

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
| 0x00430f80 | 13677 | ScriptInterpreter - opcode values 0x00 through 0x4b |
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

# Radare2 Analysis Files for ShadowFlare.exe

This folder contains radare2 project files and scripts for reverse engineering ShadowFlare.exe.

## Files

| File | Description |
|------|-------------|
| `shadowflare.r2` | Full radare2 project (analysis + function names) |
| `shadowflare-init.r2` | Script with function names and comments (human-readable) |

## Usage

### Loading the Full Project

The `shadowflare.r2` file contains the complete analysis. To use it:

```bash
cd /path/to/ShadowFlare/game/folder
r2 -e bin.relocs.apply=true ShadowFlare.exe

# Inside r2:
. /path/to/OpenShadowFlare/radare2/shadowflare.r2
```

Or copy to the default project location:
```bash
mkdir -p ~/.local/share/radare2/projects/shadowflare
cp shadowflare.r2 ~/.local/share/radare2/projects/shadowflare/rc.r2

# Then just load by name:
cd /path/to/ShadowFlare
r2 -p shadowflare ShadowFlare.exe
```

### Using the Init Script (Recommended for Comments)

The init script has human-readable comments. Use if the project file is corrupted or outdated:

```bash
cd /path/to/ShadowFlare
r2 -e bin.relocs.apply=true -i /path/to/OpenShadowFlare/radare2/shadowflare-init.r2 ShadowFlare.exe
```

### Saving Updates

After renaming more functions or adding comments:

```bash
# Inside r2 session:
Ps shadowflare

# Then copy updated project to repo:
cp ~/.local/share/radare2/projects/shadowflare/rc.r2 /path/to/OpenShadowFlare/radare2/shadowflare.r2

# Also update the init script with new function names!
```

## Key Commands

| Command | Description |
|---------|-------------|
| `afn NewName @ addr` | Rename function at address |
| `CC comment @ addr` | Add comment at address |
| `f flagname @ addr` | Create flag (named location) |
| `afl` | List all functions |
| `afl~Name` | Search for functions by name |
| `pdf @ addr` | Print disassembly of function |
| `pdc @ addr` | Pseudo-decompile function |
| `Ps name` | Save project |
| `Po name` | Open project |

## Named Functions

Current named functions (39 total):

### Initialization
- `CheckSingleInstance` @ 0x00401550
- `ProcessCommandLine` @ 0x004014a0
- `LoadConfig` @ 0x00401eb0
- `InitGame` @ 0x004016b0
- `WinMain` @ 0x004022e0
- `CreateGameWindow` @ 0x00402520
- `Shutdown` @ 0x00401b90

### Window/Input
- `WndProc` @ 0x00402b30
- `KeyDownHandler` @ 0x00402840
- `KeyUpHandler` @ 0x004026d0
- `LeftClickHandler` @ 0x004028a0
- `RightClickHandler` @ 0x00402900

### Game State Machine
- `UpdateGameState` @ 0x004023d0
- `State0_Init` @ 0x00420c40
- `State0Handler` @ 0x00420df0 (Title/Menu)
- `State1_Init` @ 0x00421a00
- `State1Handler` @ 0x00421bf0 (Loading)
- `State2_Init` @ 0x0041d3f0
- `State2Handler` @ 0x0041d880 (Gameplay)
- `State2Handler_Main` @ 0x0041d970

### Scenario/Loading
- `LoadScenario` @ 0x00426200
- `LoadScenarioData` @ 0x00426160
- `LoadScenarioMct` @ 0x00427b50

### Save/Load
- `FindSaveSlot` @ 0x004021b0
- `LoadSaveSlotInfo` @ 0x004239b0
- `SaveGame` @ 0x0044b580

### Major Systems
- `CommandDispatcher` @ 0x00429ec0 (20119 bytes!)
- `ScriptInterpreter` @ 0x00430f80 (75 opcodes)
- `LoadItemData` @ 0x00462f80
- `OptionsMenu` @ 0x004103c0
- `ObjectNpcDisplay` @ 0x00414990
- `StatusScreenDisplay` @ 0x00409a60
- `CharacterStatusDisplay` @ 0x00405750
- `CharacterCreation` @ 0x00421e10

## Named Globals

- `g_hInstance` @ 0x00482770 - HINSTANCE
- `g_SFWindow` @ 0x00482778 - Main window object (~0xB100 bytes)
- `g_pUpdib` @ 0x00482D10 - RKC_UPDIB*
- `g_pDbfControl` @ 0x00482D18 - RKC_DBFCONTROL*
- `g_pNetwork` @ 0x00482D24 - RKC_NETWORK*
- `g_pDsound` @ 0x00482D28 - RKC_DSOUND*
- `g_GameMode` @ 0x00482DB0 - 0=SP, 1=Client, 2=Server
- `g_ScreenshotFlag` @ 0x0048D71C - PrintScreen flag

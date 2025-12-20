# OpenShadowFlare Implementation Roadmap

This document tracks the implementation status of all DLLs and their functions.

## Overview

| DLL | Total | Real Impl | Internal Fwd | Stubs | External Fwd | Status |
|-----|-------|-----------|--------------|-------|--------------|--------|
| RK_FUNCTION | 43 | 24 | 0 | 17 | 2 | 95% local |
| RKC_FILE | 10 | 10 | 0 | 0 | 0 | done |
| RKC_MEMORY | 9 | 9 | 0 | 0 | 0 | done |
| RKC_WINDOW | 7 | 3 | 4 | 0 | 0 | partial |
| RKC_FONTMAKER | 13 | 6 | 7 | 0 | 0 | partial |
| RKC_RPG_TABLE | 25 | ~19 | 5 | 0 | 1 | partial |
| RKC_DIB | 42 | 0 | 0 | 0 | 42 | not started |
| RKC_DSOUND | 43 | 0 | 0 | 0 | 43 | not started |
| RKC_DBFCONTROL | 41 | 0 | 0 | 0 | 41 | not started |
| RKC_UPDIB | 81 | 0 | 0 | 0 | 81 | not started |
| RKC_NETWORK | 131 | 0 | 0 | 0 | 131 | not started |
| RKC_RPG_AICONTROL | 51 | 0 | 0 | 0 | 51 | not started |
| RKC_RPG_SCRIPT | 82 | 0 | 0 | 0 | 82 | not started |
| RKC_RPGSCRN | 185 | 0 | 0 | 0 | 185 | not started |

**Legend:**
- **Real Impl** = actually implemented, no forwarding
- **Internal Fwd** = local function that calls original DLL via CallFunctionInDLL()
- **Stubs** = function exists but just returns failure (not used by game)
- **External Fwd** = forwards via dll.def to original DLL

---

## Priority Order

Based on dependencies and game functionality:

1. **RK_FUNCTION** (2 remaining) - Core utilities, LZ compression
2. **RKC_DIB** (42) - Graphics primitives (Device Independent Bitmap)
3. **RKC_UPDIB** (81) - Sprite rendering (uses RKC_DIB)
4. **RKC_DSOUND** (43) - Audio
5. **RKC_RPGSCRN** (185) - Game screen rendering
6. **RKC_DBFCONTROL** (41) - Database/data files
7. **RKC_RPG_TABLE** (1 remaining) - Game tables
8. **RKC_RPG_SCRIPT** (82) - Scripting engine
9. **RKC_RPG_AICONTROL** (51) - AI/enemy behavior
10. **RKC_NETWORK** (131) - Multiplayer (low priority - single player first)

---

## Detailed Status Per DLL

### RK_FUNCTION (Core Utilities)
**Status: 41/43 local (2 forwarding)**

| Function | Status | Used By |
|----------|--------|---------|
| RK_AnalyzeFilename | done | o_RKC_RPGSCRN.dll |
| RK_CheckDriveEffective | done | - |
| RK_CheckFileExist | done | ShadowFlare.exe |
| RK_CheckFilesExist | done | ShadowFlare.exe, o_RKC_RPG_TABLE.dll |
| RK_CheckLastRoot | done | - |
| RK_CheckSJIS | done | ShadowFlare.exe, o_RKC_RPG_SCRIPT.dll, o_RKC_RPGSCRN.dll, o_RKC_RPG_TABLE.dll |
| RK_CheckStringSJIS | done | ShadowFlare.exe |
| RK_CutDirectoryFromFullPath | done | o_RKC_RPGSCRN.dll |
| RK_CutFilenameFromFullPath | done | o_RKC_RPGSCRN.dll |
| RK_CutLastRoot | done | - |
| RK_DeleteTabSpaceString | done | o_RKC_RPG_SCRIPT.dll |
| RK_FilenameCompareWildCard | done | - |
| RK_GetFileLastWrite | done | - |
| RK_HuffmanDecodeFileToFile | stub | - |
| RK_HuffmanDecodeFileToMemory | stub | - |
| RK_HuffmanDecodeMemoryToFile | stub | - |
| RK_HuffmanDecodeMemoryToMemory | stub | - |
| RK_HuffmanEncodeFileToFile | stub | - |
| RK_HuffmanEncodeFileToMemory | stub | - |
| RK_HuffmanEncodeMemoryToFile | stub | - |
| RK_HuffmanEncodeMemoryToMemory | stub | - |
| RK_LzDecodeFileToFile | stub | - |
| RK_LzDecodeFileToMemory | stub | - |
| RK_LzDecodeMemoryToFile | stub | - |
| **RK_LzDecodeMemoryToMemory** | **forwarding** | ShadowFlare.exe, o_RKC_RPGSCRN.dll, o_RKC_RPG_TABLE.dll, o_RKC_UPDIB.dll |
| RK_LzEncodeFileToFile | stub | - |
| RK_LzEncodeFileToMemory | stub | - |
| RK_LzEncodeMemoryToFile | stub | - |
| **RK_LzEncodeMemoryToMemory** | **forwarding** | o_RKC_FONTMAKER.dll, o_RKC_RPGSCRN.dll, o_RKC_RPG_TABLE.dll |
| RK_MesDefineCheck | done | - |
| RK_MesDefineCut | done | - |
| RK_MesDefineSet | done | - |
| RK_ReleaseFilesExist | done | ShadowFlare.exe, o_RKC_RPG_TABLE.dll |
| RK_SelectDirectory | stub | - |
| RK_SelectFilename | stub | - |
| RK_SetAbsoluteDirNameLayer | stub | - |
| RK_SetFileLastWrite | done | - |
| RK_SetLastRoot | done | ShadowFlare.exe, o_RKC_RPGSCRN.dll, o_RKC_RPG_TABLE.dll |
| RK_StringCopyNumber | done | - |
| RK_StringsCompare | done | - |
| RK_StringsCopyAuto | done | - |
| RK_SystemTimeCompare | done | - |
| RK_WriteBitFile | stub | - |

**Next Steps:**
- Implement `RK_LzDecodeMemoryToMemory` (LZSS decompression - critical for loading game assets)
- Implement `RK_LzEncodeMemoryToMemory` (LZSS compression - used for saving)

---

### RKC_FILE (File I/O)
**Status: done (10/10 real implementations)**

All file I/O functions fully implemented with no forwarding.

---

### RKC_MEMORY (Memory Management)
**Status: done (9/9 real implementations)**

All memory management functions fully implemented with no forwarding.

---

### RKC_WINDOW (Window Management)
**Status: partial (3 real, 4 internal forwarding)**

| Function | Status |
|----------|--------|
| constructor | done |
| destructor | done |
| EqualsOperator | done |
| HScroll | internal fwd |
| VScroll | internal fwd |
| Resize | internal fwd |
| Show | internal fwd |

---

### RKC_FONTMAKER (Font Rendering)
**Status: partial (6 real, 7 internal forwarding)**

| Function | Status |
|----------|--------|
| constructor | done |
| destructor | internal fwd |
| Release | done |
| GetDoubleDDBitmap | done |
| GetDoubleDIBitmap | done |
| GetNormalDDBitmap | done |
| GetNormalDIBitmap | done |
| CreateDIB | internal fwd |
| DrawDoubleFont | internal fwd |
| DrawNormalFont | internal fwd |
| Initialize | internal fwd |
| EqualsOperator | internal fwd |
| SaveNJPFile | internal fwd |

---

### RKC_RPG_TABLE (Game Data Tables)
**Status: partial (~19 real, 5 internal forwarding, 1 external forwarding)**

Most table functions implemented. Some still forward internally.

---

### RKC_DIB (Device Independent Bitmap - Graphics)
**Status: not started (0/42)**

Core graphics primitives. High priority - needed for rendering.

---

### RKC_DSOUND (DirectSound - Audio)
**Status: not started (0/43)**

Audio playback. Medium priority.

---

### RKC_DBFCONTROL (Database Control)
**Status: not started (0/41)**

Database/data file management.

---

### RKC_UPDIB (Sprite Rendering)
**Status: not started (0/81)**

Sprite and pattern rendering. Depends on RKC_DIB.

---

### RKC_NETWORK (Networking)
**Status: not started (0/131)**

Multiplayer networking. Low priority - focus on single player first.

---

### RKC_RPG_AICONTROL (AI Control)
**Status: not started (0/51)**

Enemy AI and pathfinding.

---

### RKC_RPG_SCRIPT (Scripting Engine)
**Status: not started (0/82)**

Game scripting and event handling.

---

### RKC_RPGSCRN (RPG Screen Rendering)
**Status: not started (0/185)**

Main game screen rendering. Largest DLL - likely depends on many others.

---

## Legend

- **done** = fully implemented, no forwarding to original DLL
- **internal fwd** = local function exists but calls original via CallFunctionInDLL()
- **stub** = function exists but not used by game (returns failure)
- **external fwd** = forwards via dll.def directly to original DLL
- **-** in "Used By" = not referenced by any module

---

## Notes

### Compression Functions
The game uses RCLIB-L (LZSS variant) compression for all game assets:
- Header: `"RCLIB-L\0"` (8 bytes) + decompressed_size (4 bytes LE)
- Algorithm: 4KB sliding window, initial position 0xFEE, fill with 0x20 (space)
- Critical for loading: sprites (NJP), maps, scenarios, etc.

### SJIS Text Handling
Many string functions need to be SJIS-aware (Japanese text encoding):
- Lead bytes: 0x80-0x9F, 0xE0-0xFF indicate 2-byte characters
- Must not split 2-byte sequences when truncating/copying

### Testing Strategy
Each function is tested incrementally:
1. Enable function in dll.def (change from forwarding to local)
2. Build DLL
3. Run game with Wine for 30+ seconds
4. If crash, check osf_trace.log for last function calls
5. If working, commit and move to next function

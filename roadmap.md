# OpenShadowFlare Implementation Roadmap

This document tracks the implementation status of all DLLs and their functions.

## Overview
```
DLL                 Functions  Completed  Status
------------------  ---------  ---------  ------
RK_FUNCTION                43         43   COMPLETED
RKC_FILE                   10         10   COMPLETED
RKC_MEMORY                  9          9   COMPLETED
RKC_DSOUND                 43         43   COMPLETED
RKC_FONTMAKER              13         13   COMPLETED
RKC_WINDOW                  7          7   COMPLETED
RKC_DIB                    42         42   COMPLETED
RKC_RPG_TABLE              25         24   WIP
RKC_UPDIB                  81         70   WIP
RKC_RPG_SCRIPT             82         57   WIP
RKC_NETWORK               131        104   WIP
RKC_RPG_AICONTROL          51         31   WIP
RKC_RPGSCRN               185         95   WIP
RKC_DBFCONTROL             41         16   WIP
------------------  ---------  ---------  ------
TOTAL                     763        564   WIP
```

**Legend:**
- **Real Impl** = actually implemented, no forwarding
- **Internal Fwd** = local function that calls original DLL via CallFunctionInDLL()
- **Stubs** = function exists but just returns failure (not used by game)
- **External Fwd** = forwards via dll.def to original DLL

---

## Priority Order

Based on dependencies and game functionality:

1. ~~**RK_FUNCTION**~~ - COMPLETED
2. ~~**RKC_DIB**~~ - COMPLETED
3. **RKC_UPDIB** (11 forwards) - Sprite rendering (uses RKC_DIB)
4. ~~**RKC_DSOUND**~~ - COMPLETED
5. **RKC_RPGSCRN** (90 forwards) - Game screen rendering
6. **RKC_DBFCONTROL** (25 forwards) - Database/data files
7. **RKC_RPG_TABLE** (1 forward) - Game tables (blocked: allocator mismatch)
8. **RKC_RPG_SCRIPT** (25 forwards) - Scripting engine
9. **RKC_RPG_AICONTROL** (20 forwards) - AI/enemy behavior
10. **RKC_NETWORK** (27 forwards) - Multiplayer (low priority - single player first)

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
- RK_LzDecodeMemoryToMemory and RK_LzEncodeMemoryToMemory are done (RCLIB-L LZSS).
  All 43 functions are local -- no external forwarding remains.

---

### RKC_FILE (File I/O)
**Status: COMPLETED (10/10 local)**

All file I/O functions fully implemented with no forwarding.

---

### RKC_MEMORY (Memory Management)
**Status: COMPLETED (9/9 local)**

All memory management functions fully implemented with no forwarding.

---

### RKC_WINDOW (Window Management)
**Status: COMPLETED (7/7 local, 4 are stubs for unused functions)**

| Function | Status |
|----------|--------|
| constructor | done |
| destructor | done |
| EqualsOperator | done |
| HScroll | stub (unused) |
| VScroll | stub (unused) |
| Resize | stub (unused) |
| Show | stub (unused) |

---

### RKC_FONTMAKER (Font Rendering)
**Status: COMPLETED (13/13 local, 4 internal fwd via CallFunctionInDLL)**

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
| Initialize | done |
| EqualsOperator | internal fwd |
| SaveNJPFile | stub (not referenced) |

---

### RKC_DSOUND (DirectSound - Audio)
**Status: COMPLETED (43/43 local)**

All audio functions reimplemented using the haudio library. No forwarding.

---

### RKC_DIB (Device Independent Bitmap - Graphics)
**Status: COMPLETED (42/42 local, 0 external forwarding)**

All graphics primitives are now local. Key implemented functions:

| Function | Status | Notes |
|----------|--------|-------|
| constructor / destructor | done | Returns `this` per MSVC convention |
| Create / Release | done | Allocates/frees BITMAPINFOHEADER + palette + pixel buffer |
| GetAlignWidth | done | DWORD-aligned stride calculation |
| Fill / FillByte | done | Whole-bitmap fill |
| SetBitmap / SetPalette / CopyPalette | done | Pointer/palette management |
| GetBitmap / GetBitmapInfo / GetPalette | done | Getters |
| GetPaletteCount / GetRect | done | Getters |
| TransferToDIB (2 overloads) | done | Simple blit with transparency |
| TransferToDIBFast (2 overloads) | done | Fast memcpy blit |
| **TransferToDIBEx (2 overloads)** | **done** | Full sprite blit: flip modes, alpha blending (1000-based), palette lookup, 8/24bpp paths, additive blend |
| TransferToDDB (2 overloads) | done | DIB -> device context (stubs for now, GDI path) |
| **Convert** | **done** | BPP conversion: 1/4/8/24 bpp source -> 1/4/8/16/24 bpp dest |
| **DrawFill** | **done** | Filled rectangle with blending (8/16/24bpp, opaque/alpha/additive/darken/brighten) |
| **DrawBox** | **done** | Rectangle outline (4x DrawLine) |
| **DrawLine** | **done** | DDA line with blending |
| **DrawPoint** | **done** | Single pixel with blending |
| ZoomToDIB / ZoomToDIBEx | done | Scaled blit with nearest-neighbor sampling |
| ReadFile | done | Loads BMP from file |
| WriteFile | stub | Not referenced by game |
| Copy / operatorAssign | done | Deep copy |
| AddOffset / ClearUnusedArea | stub | Not referenced |
| CompareBitmapColor / PaintArea | stub | Not referenced |
| ScreenPaintLineScan | stub | Not referenced |
| DIBHISPEEDMODE (3 functions) | stub | Lookup table mode, not yet needed |

---

### RKC_RPG_TABLE (Game Data Tables)
**Status: 24/25 local, 1 external forward**

| Function | Status | Notes |
|----------|--------|-------|
| Release | **external fwd** | Allocator mismatch: nodes created by original DLL's operator_new can't be freed with our delete. Needs Insert + ReadBinaryFile implemented locally first. |
| 19 functions | done | Real implementations |
| 5 functions | internal fwd | Forward via CallFunctionInDLL |

---

### RKC_UPDIB (Sprite Rendering)
**Status: 70/81 local (11 external forwards)**

Sprite and pattern rendering. Depends on RKC_DIB. Most getters and simple functions
are now implemented locally, including the UPD pattern/parts accessors, VS block and
VS packet counting helpers, font parameter lookup, temp DIB rebuild, and UPD deletion.
Core rendering functions (Render, SetPacket, etc.) still forward to the original DLL.

---

### RKC_RPG_SCRIPT (Scripting Engine)
**Status: 57/82 local (25 external forwards)**

Game scripting and event handling. Unused functions stubbed, core script
execution still forwards to original DLL.

---

### RKC_NETWORK (Networking)
**Status: 104/131 local (27 external forwards)**

Multiplayer networking. Most unused functions stubbed locally.
Low priority -- focus on single player first.

---

### RKC_RPG_AICONTROL (AI Control)
**Status: 31/51 local (20 external forwards)**

Enemy AI and pathfinding. Unused functions stubbed, core AI processing
still forwards to original DLL.

---

### RKC_RPGSCRN (RPG Screen Rendering)
**Status: 95/185 local (90 external forwards)**

Main game screen rendering. Largest DLL with the most remaining forwards.
Depends on RKC_DIB, RKC_UPDIB, and most other DLLs.

---

### RKC_DBFCONTROL (Database Control)
**Status: 16/41 local (25 external forwards)**

Database/data file management. Core database operations still forward
to original DLL.

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
- Header: `"RCLIB-L"` (7 bytes) + terminator (1 byte) + decompressed_size (4 bytes LE) + reserved (4 bytes)
- Algorithm: 4KB sliding window, initial position 0xFEE, window filled with 0x00
- Flags: MSB-first bit order, 1 = match reference, 0 = literal byte
- Critical for loading: sprites (NJP), maps, scenarios, save files, etc.

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

# Remina Project for ShadowFlare

This folder contains the Remina reverse engineering project for ShadowFlare.

## Project File

- `shadowflare.rmn` - Main project with ShadowFlare.exe + all 14 DLLs (analyzed)

## Quick Start

```bash
cd tmp/ShadowFlare
remina ../../remina/shadowflare.rmn
aa    # Run full analysis on all modules
```

## Analysis Results

After running `aa`:
- **15 modules** analyzed (EXE + 14 DLLs)
- **3,360 functions** found
- **1,474 imports** resolved
- **2,901 strings** found
- **6,735 cross-references**

## Key Findings

### EXE Entry Points (Annotated)
| Address | Name | Description |
|---------|------|-------------|
| 0x46863c | _start | CRT startup code |
| 0x4022E0 | WinMain | Main game entry point |
| 0x402520 | CreateWindow | Window creation |
| 0x401550 | CheckMutex | Single instance check |
| 0x401EB0 | InitializeApp | Application init |
| 0x4016B0 | InitializeGame | Game initialization |

### Compression (RK_FUNCTION)
| Address | Function | Notes |
|---------|----------|-------|
| 0x10004960 | RK_LzDecodeMemoryToMemory | LZSS decode (RCLIB-L) |
| 0x100045a0 | RK_LzDecodeFileToFile | File-based LZ decode |
| 0x100047b0 | RK_LzDecodeFileToMemory | File to memory LZ |
| 0x10004b00 | RK_LzDecodeMemoryToFile | Memory to file LZ |

### LZSS Algorithm Details (from disassembly)
- Header: `"RCLIB-L"` (8 bytes) + decompressed size (4 bytes LE)
- 4KB sliding window (0xFFF mask)
- Initial position: 0xFEE
- Flags: LSB-first bit order
- Match: low nibble + 3 = length, high nibble << 4 | next byte = offset

### Huffman Compression
- Header: `"RCLIB-H"` (8 bytes)
- 5 cross-references found

### NJP Sprite Format
- Header: `"NJudgeUniPat002"` or `"NJudgeUniPat"`
- Used for all sprite/pattern files (.njp)

### EXE Imports from RK_FUNCTION (7 functions)
1. RK_CheckFileExist
2. RK_SetLastRoot  
3. RK_CheckStringSJIS
4. RK_CheckFilesExist
5. RK_LzDecodeMemoryToMemory ← Main decompression
6. RK_CheckSJIS
7. RK_ReleaseFilesExist

## Loaded Modules

The project includes:
- ShadowFlare.exe (main executable)
- o_RK_FUNCTION.dll
- o_RKC_FILE.dll
- o_RKC_MEMORY.dll
- o_RKC_DIB.dll
- o_RKC_DSOUND.dll
- o_RKC_WINDOW.dll
- o_RKC_FONTMAKER.dll
- o_RKC_DBFCONTROL.dll
- o_RKC_UPDIB.dll
- o_RKC_NETWORK.dll
- o_RKC_RPG_AICONTROL.dll
- o_RKC_RPG_SCRIPT.dll
- o_RKC_RPG_TABLE.dll
- o_RKC_RPGSCRN.dll

## Common Commands

### Navigation & Viewing
```
s <addr>      - Seek to address or symbol
d [n]         - Disassemble n instructions
df            - Disassemble current function
dfc           - C-style pseudo-decompile function
x [n]         - Hexdump n bytes
g             - Show control flow graph
```

### Listing Information
```
ls            - List PE sections
li [filter]   - List imports (optionally filter by DLL name)
le [filter]   - List exports
lf [filter]   - List functions
lz [filter]   - List strings
lm            - List loaded modules
lh            - Show PE headers
```

### Analysis
```
a             - Analyze current binary
aa            - Analyze all (EXE + loaded DLLs)
xr <addr>     - Find cross-references TO address
xrf <addr>    - Find cross-references FROM address
```

### Annotations
```
n <addr> <name>   - Rename symbol/function
c <addr> <text>   - Add comment
bm add            - Bookmark current location
bm list           - List bookmarks
```

### Search
```
/ <text>      - Search for string
/x <hex>      - Search for hex pattern
```

### Workspace
```
dll <path>    - Load a DLL into workspace
ws            - Show workspace status
w [file]      - Save project
e <file>      - Load project
```

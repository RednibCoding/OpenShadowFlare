# ShadowFlare Data Files

## File Formats

| Extension | Description |
|-----------|-------------|
| `.njp`    | NJudgeUniPat - Sprite/pattern data (RCLIB-L compressed) |
| `.sdw`    | Shadow data for sprites |
| `.caf`    | Character animation frames |
| `.Ssv`    | Save game data |
| `.Bmp`    | Save slot thumbnail (Windows Bitmap) |
| `.Voc`    | Voice/sound data |
| `.Scs`    | Scenario script |
| `.Mct`    | Map/scenario data |
| `.Cfg`    | Configuration file |

## System Files

### Fonts (InitGame at 0x004016b0)
- `System\Common\Pattern\Font00.njp` - Main font
- `System\Common\Pattern\Font01.njp` - Alt font
- `System\Common\Pattern\System.njp` - System graphics
- `System\Common\Pattern\Waiting.njp` - Loading animation

### Audio
- `System\Game\Voice\Voice00.Voc` - Voice/sound effects
- `System\Title\Music\BGM00.Voc` - Title screen music

### Item Sprites
- `System\Game\Pattern\Item0000.njp` through `Item0013.njp` (14 item patterns)

## Player Character Files

### Male Character
- `Player\Male\Animation00.caf` - Animation data
- `Player\Male\Animation00.sdw` - Shadow data
- `Player\Male\Animation00.njp` - Sprite patterns

### Female Character
- `Player\Female\Animation00.caf` - Animation data
- `Player\Female\Animation00.sdw` - Shadow data
- `Player\Female\Animation00.njp` - Sprite patterns

### Common Player Data
- `Player\Common\UnlockSW.caf/sdw/njp` - Unlock animations
- `Player\Common\Compasses.caf/sdw/njp` - Compass graphics
- `Player\Common\Powerup.caf/sdw/njp` - Powerup effects

## Save Files

### Save Data (Save\%04d.Ssv)
- 6 save slots (0000-0005)
- Header: 16 bytes
- Data: 352 bytes (0x160)
- Format: Binary

### Save Thumbnails (Save\%04d.Bmp)
- Screenshot at time of save
- Used in load game screen

## Scenario Files

### Directory Structure
```
Scenario\
├── %08d\                    # Scenario ID (8-digit, 209 total scenarios)
│   ├── Scenario.Njp         # Scenario graphics (NJP sprite format)
│   ├── Scenario.Scs         # Scenario script (compiled script)
│   └── Scenario.Mct         # Map character/entity table
```

### Scenario ID Format
IDs appear to follow a structured format:
- `00XXXXXX` - Main game scenarios (00000000-00000006)
- `0001XXXX` - Episode/chapter 1 (00010000-00010005)
- `01XXXXXX` - Different region/area
- `99XXXXXX` - Special/bonus scenarios (99000000-99000037)

### Scenario.Mct
Map Character/Entity Table format:
```
Header:
  0x00: char[16] magic = "MCED DATA v0000"
```

### Scenario.Scs
Scenario Script - loaded via `RKC_RPG_SCRIPT::ReadBinary()`

Script components:
- StatusBlock - Character statuses
- SentenceBlock - Script sentences
- Commands with operands (75 opcodes)
- TempFlag / NetFlag - Script flags

```
Header (16 bytes):
  0x00: char[16] magic = "NJudgeUniPat003"
  0x10: uint32_t patternCount

Per Pattern (20 bytes + data):
  0x00: uint8_t bpp           # Bits per pixel
  0x04: uint32_t width
  0x08: uint32_t height
  0x0C: uint32_t flags
  0x10: ... RCLIB-L compressed pixel data

Stride alignment: (width + 3) & ~3
```

## RCLIB-L Compression (LZSS)

RCLIB-L is an LZSS-based compression format used for all game assets (sprites, maps, etc.).

### Header (16 bytes)
```
Offset  Size  Description
------  ----  -----------
0x00    7     Magic: "RCLIB-L" (ASCII)
0x07    1     Terminator: usually 0x00 or 0x1A (varies, not checked)
0x08    4     Decompressed size (uint32_t, little-endian)
0x0C    4     Reserved/unused (skip these bytes)
```

### Decompression Algorithm

**Initialization:**
- 4KB (4096 bytes) sliding window, filled with **zeros** (0x00)
- Initial window write position: **0xFEE** (4078)
- Data starts at offset **16** (after header)

**Flag Byte Processing:**
- Read one flag byte from input
- Process 8 bits, **MSB first** (bit 7 → bit 0)
- Mask starts at 0x80, shifts right each iteration

**Bit Interpretation:**
| Bit Value | Meaning | Action |
|-----------|---------|--------|
| 0 (clear) | Literal | Read 1 byte, write to output and window |
| 1 (set)   | Match   | Read 2-byte reference, copy from window |

**Match Reference Encoding (2 bytes):**
```
Byte 1 (b1): Lower 8 bits of offset
Byte 2 (b2): Upper 4 bits = offset high nibble, Lower 4 bits = length - 3

Offset = b1 | ((b2 & 0xF0) << 4)    // 12-bit offset (0-4095)
Length = (b2 & 0x0F) + 3            // Length 3-18 bytes
```

**Pseudocode:**
```c
void decompress_rclib_l(uint8_t* src, int srcSize, uint8_t* dest) {
    uint8_t window[4096] = {0};    // Zero-filled
    int srcPos = 16;                // Skip 16-byte header
    int destPos = 0;
    int winPos = 0xFEE;             // Initial window position
    int decompSize = *(uint32_t*)(src + 8);
    
    while (srcPos < srcSize && destPos < decompSize) {
        uint8_t flags = src[srcPos++];
        
        for (uint8_t mask = 0x80; mask != 0 && destPos < decompSize; mask >>= 1) {
            if (flags & mask) {
                // Match reference (bit = 1)
                uint8_t b1 = src[srcPos++];
                uint8_t b2 = src[srcPos++];
                int offset = b1 | ((b2 & 0xF0) << 4);
                int length = (b2 & 0x0F) + 3;
                
                for (int i = 0; i < length && destPos < decompSize; i++) {
                    uint8_t c = window[(offset + i) & 0xFFF];
                    dest[destPos++] = c;
                    window[winPos] = c;
                    winPos = (winPos + 1) & 0xFFF;
                }
            } else {
                // Literal byte (bit = 0)
                uint8_t c = src[srcPos++];
                dest[destPos++] = c;
                window[winPos] = c;
                winPos = (winPos + 1) & 0xFFF;
            }
        }
    }
}
```

### Common Mistakes to Avoid
1. **Header size**: 16 bytes, not 12 (there's 4 bytes of padding/reserved after decompSize)
2. **Bit order**: MSB first (0x80 → 0x01), NOT LSB first
3. **Bit meaning**: 1 = match, 0 = literal (opposite of some LZSS variants)
4. **Window init**: Zero-filled (0x00), NOT space-filled (0x20)
5. **Magic check**: Only compare 7 bytes ("RCLIB-L"), byte 8 varies

## Save File Format (.Ssv)

### Header
```
0x00: char[16] magic = "ShadowFlare0005"
0x10: uint32_t scenarioId (at [edi + 0x4e8])
0x14: uint32_t playTime (at [edi + 0x4ec])
... more fields from offset 0x4e8-0x510 in game state
```

### Encryption
Save files use XOR encryption with a 64-byte key:
```
BE 66 B3 2F 01 6E 6D C8 1F 98 A5 46 76 5C 3D 0E
AA 5E 9D FF EA A0 0D 4B 75 F6 61 85 5D BB DC FB
8B C3 4F 45 04 90 81 1E 6B C9 D3 73 C6 E7 24 BA
32 F3 C0 EC 57 CC C4 B6 C1 AE AF 88 ... (64 bytes)
```

### Related Files
- `Save\%04d.Ssv` - Save slot data (0000-0005)
- `Save\%04d.Bmp` - Save thumbnail
- `Save\M%08d%02d.msk` - Map state/mask data

## Item Database (Item.Ibn)

### Location
`System\Game\Parameter\Item.Ibn`

### Format
```
Header:
  0x00: char[16] magic = "SFItemDataV0000"
  
Encryption:
  Same 64-byte XOR key as save files
```

## Table Data (Table.Tbd)

### Location  
`System\Game\Parameter\Table.Tbd`

### Access
Via RKC_RPG_TABLE DLL:
- `ReadBinaryFile()` - Load table
- `GetFromTableNo()` - Get table by number
- `GetRowCount()` / `GetColCount()` - Get dimensions
- `GetValue()` / `GetStrings()` - Get cell data

## Scenario Data

### Scenario.Mct
Map Character/Entity Table format:
```
Header:
  0x00: char[16] magic = "MCED DATA v0000"
```

### Scenario.Scs
Scenario Script - loaded via `RKC_RPG_SCRIPT::ReadBinary()`

Script components:
- StatusBlock - Character statuses
- SentenceBlock - Script sentences
- Commands with operands (75 opcodes)
- TempFlag / NetFlag - Script flags

## Audio Files

### Background Music (.Voc)
Game music tracks (11 total):
- `System\Game\Music\BGM00.Voc` - BGM 1
- `System\Game\Music\BGM01.Voc` - BGM 2
- `System\Game\Music\BGM02.Voc` - BGM 3
- `System\Game\Music\BGM03.Voc` - BGM 4
- `System\Game\Music\BGM04.Voc` - BGM 5
- `System\Game\Music\BGM05.Voc` - BGM 6
- `System\Game\Music\BGM06.Voc` - BGM 7
- `System\Game\Music\BGM07.Voc` - BGM 8
- `System\Game\Music\BGM08.Voc` - BGM 9
- `System\Game\Music\BGM09.Voc` - BGM 10
- `System\Game\Music\BGM10.Voc` - BGM 11
- `System\Title\Music\BGM00.Voc` - Title screen music

### Voice/Sound Effects
- `System\Game\Voice\Voice00.Voc` - Voice/SFX

### Animation Files (.Caf)
Character animation frames - loaded via `RKC_RPGSCRN_CHARANIM::ReadCafFile()`

Player animations use format: `%s\Animation.Caf`

## Configuration

### SFlare.Cfg
Configuration file loaded by `LoadConfig` (0x00401eb0).

### Registry
`HKEY_LOCAL_MACHINE\SOFTWARE\Missinglink\ShadowFlare`
- `User` - Registered user name

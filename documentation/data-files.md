# ShadowFlare Data Files

## File Formats

| Extension | Description |
|-----------|-------------|
| `.njp`    | NJudgeUniPat - Sprite/pattern data (RCLIB-L compressed) |
| `.sdw`    | Shadow data for sprites |
| `.caf`    | Character animation frames |
| `.Obl`    | Map object placement and collision records |
| `.Ssv`    | Save game data |
| `.Bmp`    | Save slot thumbnail (Windows Bitmap) |
| `.Voc`    | Voice/sound data |
| `.Scs`    | Scenario script |
| `.Mct`    | Map/scenario data |
| `.Aid`    | Global actor/enemy AI action database |
| `.Tbd`    | General gameplay parameter tables |
| `.Ibn`    | Item definition database |
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

### AI behavior data

`System\Game\Parameter\Control.aid` is the global actor and enemy behavior
database loaded through `RKC_RPG_AICONTROL`. Its header is
`RKC_AIDATA v001`, followed by byte `0x1a`. The preserved file declares 64
behavior lists and 18 event buckets per list.

A version-1 behavior list stores a variable-length name and walk-point speed,
then its event buckets. Every action candidate within an event stores:

| Field | Size | Purpose |
|---|---:|---|
| Action number | 4 bytes | Selects native executable behavior |
| Parameter block | 36 bytes | Priority, weight, timing, range, movement, and other action inputs |
| Condition block | 24 bytes | Eligibility tests against actor/world state |

The reconstructed DLL handles the binary container, list/event lookup, and
copying. The executable evaluates conditions, chooses an eligible action, and
executes the selected native behavior. This makes AID script-like data, but it
is not another `Scenario.Scs` format and does not use the scenario opcode
interpreter.

The known division of responsibility and the requirements for its future
portable implementation are covered in
[ShadowFlare's script engine](script-engine.md#the-other-behavior-systems).

### Item Sprites

- `System\Game\Pattern\Item0000.njp` through `Item0013.njp` contain the
  inventory and equipment-panel artwork.
- `Character\ITEM\%08d\Animation.Caf/Njp/Sdw` contain animated dropped-item
  graphics and their shadows. Some later resources use `Pattern.njp/sdw`
  instead.

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

The CAF directions contain several parallel visual parts. Part 0 is the base
body and part 1 selects the matching SDW shadow. The remaining parts are
equipment variants, not layers that are all meant to be visible at once. The
executable rebuilds a per-part enable table from the currently equipped items.

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
The executable loader at `0x00427b50` starts with this fixed section:

| Offset | Size | Meaning |
|--------|------|---------|
| `0x000` | 16 | `MCED DATA v0000`, followed by byte `0x1a` |
| `0x010` | 260 | Controller/AI path, normally `System\Game\Parameter\Control.aid` |
| `0x114` | 260 | Map path, such as `Map\f00_01.map` |
| `0x218` | 4 | Unknown 32-bit field |
| `0x21c` | 4 | Unknown 32-bit field |
| `0x220` | 4 | Zero-based music index |
| `0x224` | 256 | Area title |
| `0x324` | variable | Entity and scenario records, not fully mapped yet |

The variable section begins with three `count + int32[count]` preload lists
for `Character\OBJECT`, `Character\PEOPLE`, and `Character\ENEMY` resources,
in that order. Every nonnegative resource used by the matching records appears
in its list across all 209 shipped MCT files.

Four counted runtime-entity groups follow: object, `PEOPLE`, enemy, and item.
They all use this shared variable-length prefix:

- local ID and character resource ID;
- byte length, name bytes, and a name color when the length is nonzero;
- an unknown 32-bit field;
- world X/Y, four judgement bounds, and eight-way direction;
- a counted list of initial CAF part overrides;
- an optional custom-part table containing a count, 32-bit visibility values,
  and signed 16-bit red, green, and blue strengths;
- one more unknown 32-bit field.

An object's `0x34`-byte tail contains 13 signed values:

| Index | Confirmed use |
|---:|---|
| 0 | Visual mode: `0` selects a CAF animation and `1` selects a static pattern |
| 1 | Static `Pattern.Njp` index when mode is `1` |
| 2 | `Animation.Caf` chart when mode is `0` |
| 3 | Adds status bit `0x80` to the draw request |
| 4 | Object height passed to the screen object |
| 5 | Unknown zero/one value |
| 6 | Unknown value; retail data uses `-1` or `30000` through `30500` |
| 7 | Draw flags; retail data uses `0` or `16` |
| 8 | Draw strength; retail data uses `0` or `1000` |
| 9 | Unknown zero/one value |
| 10–12 | Red, green, and blue draw strengths |

These meanings come from the type-zero initializer at `0x0045dd00` and draw
path at `0x0045ddd0`. The portable decoder preserves the still-unknown values
without assigning behavior to them.

A person's `0x2c`-byte tail starts with walk speed, maximum walk updates, idle
updates, and a flag choosing relative or absolute bounds, followed by the
left, top, right, and bottom of its movement rectangle. The next value allows
native action 21 to turn the person toward its evaluated target. The following
value disables autonomous wandering when nonzero. The final signed value is
still unnamed; it is `-65` for every Remote Town person.

Enemy and item records carry `0x13c`- and `0x10`-byte tails respectively.
Their shared prefixes and exact boundaries are validated now, but their tails
remain to be named and exposed.

Remote Town has seven people. The first is Ostare: local ID 0, resource ID 13,
position (`91467`, `1532`), judgement `[-80, -80, 79, 79]`, and direction 7.
His custom table enables CAF parts 0, 1, 2, 3, and 6. Resource 13 resolves to
`Character\PEOPLE\00000013\Animation.{Caf,Njp,Sdw}`. His tail stores speed 10,
30 walk updates, 30 idle updates, and a spawn-relative rectangle from
(`-437`, `-223`) to (`269`, `231`). Its scripted-turning and wandering values
are both enabled. Malse disables both; Syria and all four town animals allow
scripted turning but do not wander.

Remote Town also has seven dynamic object records, separate from the 279
static entries in `f00_01.Obl`. Their local IDs are `0`, `200` through `204`,
and `300`; the three nonnegative resources are `8`, `15`, and `14`. Object
`300` is named `Warehouse`. Resources 8 and 14 use
`Character\OBJECT\00000008` and `00000014` pattern/shadow pairs, while
resource 15 contains both static pattern data and
`Animation.Caf`/`Animation.Njp`.
Scenario `00000000` has zero enemy and zero item records.

After the item group is a 32-bit entry count followed by 16-byte entry records.
Each record stores a signed 32-bit key, world X, world Y, and eight-way
direction, in that order. Three more 32-bit scenario fields close the file;
their meanings are not known yet. The portable loader now reaches this table
sequentially and rejects missing groups, invalid entry directions, truncated
footers, and trailing bytes instead of guessing the table location from the
end of the file.

The initial `00000000` file names `Map\f00_01.map`, title `Remote Town`, and
music index 0. Entry key 0 starts a new character at world position
(`89898`, `2811`) facing direction 3.

### Scenario.Scs
Compiled scenario script, loaded by `RKC_RPG_SCRIPT::ReadBinary()`. It begins
with `ScenaScriptV000\0` and contains counted temporary flags, network flags,
bitwise-inverted message strings, status triggers, sentences, commands, and
typed operands. Remote Town has 66 temporary flags, 61 messages, 23 triggers,
220 sentences, and 608 commands.

The full known layout and the gradually reconstructed interpreter are
documented in [ShadowFlare's script engine](script-engine.md).

### Map Object Lists (`Map\Object\*.Obl`)

Object lists begin with `RPGSCRN_OBJv000` or `RPGSCRN_OBJv001`, followed by a
32-bit record count. Each record stores world X/Y, a 16-bit pattern-list index
and pattern number, palette, opacity, status, height, and four 32-bit judgement
bounds. Version 1 adds three 16-bit color-strength fields before the judgement
rectangle. Records are 36 bytes in version 0 and 42 bytes in version 1.

The pattern-list index refers to the matching map `.Lst`. Visible NJP entries
are commonly followed by a `ShadowLowPat` SDW entry used by objects whose
status includes the shadow bit.

The judgement rectangle is also the object's depth footprint. Retail rendering
first orders objects by the projected position of its left/top corner, then
runs a second pass over all four absolute rectangle edges. It is therefore not
safe to reduce an OBL record to a single anchor or Y value: large house and
wall rectangles deliberately decide when actors pass behind their artwork.

## NJP Sprite Format (NJudgeUniPat)

NJP files contain paletted sprite/pattern data with embedded color palettes. 524 of 545 NJP files
in the game contain embedded palettes.

### File Structure Overview

```
+---------------------------+
| Magic Header (16 bytes)   |
+---------------------------+
| Pattern Count (4 bytes)   |
+---------------------------+
| Pattern 0 Header (20 b)   |
| RCLIB-L Block 0           |
+---------------------------+
| Pattern 1 Header (20 b)   |
| RCLIB-L Block 1           |
+---------------------------+
| ... more patterns ...     |
+---------------------------+
| Extended Header (12 b)    |
| Extended Metadata (32*N)  |
+---------------------------+
| Palette Section           |
+---------------------------+
```

### Magic Header (16 bytes)
```
Offset  Size  Description
------  ----  -----------
0x00    16    Magic: "NJudgeUniPat003\0"
```

### Main Header (4 bytes)
```
Offset  Size  Description
------  ----  -----------
0x10    4     Pattern count (uint32_t, little-endian)
```

### Pattern Header (20 bytes each)
Before each RCLIB-L compressed block:
```
Offset  Size  Description
------  ----  -----------
0x00    4     Field0 (unknown, often 0)
0x04    4     BPP - Bits per pixel (4 or 8)
0x08    4     Width in pixels
0x0C    4     Height in pixels
0x10    4     Flags (bitfield, see below)
```

**Flags bitfield:**
- Bit 0: Unknown
- Other bits: Unknown (needs more analysis)

**BPP values:**
- 4 = 16-color palette (64 bytes)
- 8 = 256-color palette (1024 bytes)

### Pixel Data (RCLIB-L compressed)
After each pattern header is an RCLIB-L compressed block containing the raw pixel indices.

**Stride alignment:** `(width + 3) & ~3` (4-byte aligned rows)

### Extended Header (12 bytes)
Located after all pattern RCLIB-L blocks:
```
Offset  Size  Description
------  ----  -----------
0x00    4     Pattern count (repeated)
0x04    4     Pattern count (repeated again)
0x08    4     Palette count
```

### Extended Metadata (32 bytes per pattern)
After extended header, 32 bytes of additional data per pattern:
```
Offset  Size  Description
------  ----  -----------
0x00    4     Width (repeated)
0x04    4     Height (repeated)
0x08    24    Other metadata (unknown)
```

### Palette Section
Located after extended metadata. Palette data is stored in **BGRA format** (Blue, Green, Red, Alpha).

**Color entry (4 bytes):**
```
Offset  Size  Description
------  ----  -----------
0x00    1     Blue (0-255)
0x01    1     Green (0-255)
0x02    1     Red (0-255)
0x03    1     Alpha (usually 0x00)
```

**Palette sizes:**
- 4-bit (16 colors): 64 bytes
- 8-bit (256 colors): 1024 bytes

### Parsing Algorithm

To find the palette:
1. Skip 16-byte magic + 4-byte pattern count
2. For each pattern:
   - Read 20-byte pattern header
   - Read RCLIB-L block (check "RCLIB-L" magic, read decompSize at offset +8)
   - Skip compressed data to next pattern
3. After last pattern, read 12-byte extended header
4. Skip 32 * pattern_count bytes of extended metadata
5. Read palette_count palettes (each 64 or 1024 bytes based on BPP)

### Example: Small 4-bit NJP (Pattern.Njp)
```
0x00: "NJudgeUniPat003\0"   Magic (16 bytes)
0x10: 01 00 00 00           Pattern count = 1
0x14: [20 bytes]            Pattern 0 header (BPP=4, W=21, H=7)
0x28: [78 bytes]            RCLIB-L block 0
0x76: 01 00 00 00           Extended header: pattern_count = 1
0x7A: 01 00 00 00           Extended header: pattern_count = 1
0x7E: 01 00 00 00           Extended header: palette_count = 1
0x82: [48 bytes]            Extended metadata (32 bytes + 16 pad)
0xB2: 01 00 00 00           Palette count confirmation
0xB6: [64 bytes]            16-color BGRA palette
```

### Example: Large 8-bit NJP (Card.Njp)
```
0x00: "NJudgeUniPat003\0"   Magic (16 bytes)
0x10: 45 00 00 00           Pattern count = 69
0x14: [69 patterns...]      Pattern headers + RCLIB-L blocks
0x7AA84: [ext header]       Extended header
0x7AA90: [69*32 bytes]      Extended metadata
0x8B994: [palette data]     Multiple 256-color BGRA palettes
```

### Notes
- Some NJP files contain multiple palettes even when palette_count=1
- The palette_count field may not always reflect the actual number of palettes
- For reliable extraction, calculate palette region as: EOF - (palette_count * palette_size)
- Color 0 is typically transparent in sprites

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

The writer at `0x0044b580` starts every save with:

| File offset | Size | Meaning |
|-------------|------|---------|
| `0x000` | 16 | `ShadowFlare0005` plus a terminating zero |
| `0x010` | `0x160` | Plain player record used by the load-game menu |
| `0x170` | 4 | Encrypted payload size |
| `0x174` | 1 | Random XOR byte |
| `0x175` | 4 | Checksum of the plaintext payload |
| `0x179` | variable | Substituted, XOR-obscured payload |

The plain 0x160-byte record is copied from player offset `+0x10`. Confirmed
fields within that record are:

| Record offset | Meaning |
|---------------|---------|
| `0x00` | Character name in a 24-byte area |
| `0x18` | Gender (`0` male, `1` female) |
| `0x1c` | Job value displayed by the save menu |
| `0x24` | Level |
| `0x28`..`0x60` | Thirteen initial/base parameters in retail storage order |
| `0x30` / `0x34` | Base maximum/current life |
| `0x38` / `0x3c` | Base maximum/current mana |

Unknown bytes remain part of the record and must be preserved. The record is
also serialized again at the beginning of the encrypted payload. Retail first
uses the plain copy to construct the player and then restores the complete
gameplay state through `0x0044cac0`.

The payload is not encrypted with a repeating 64-byte key. The writer sums the
plaintext bytes for its checksum, XORs every byte with the stored random byte,
then replaces it with its index in a fixed 256-byte permutation. Loading
reverses the permutation and XOR before parsing the dynamic records. That
payload includes inventory and equipment objects, scenario state, global and
script flags, companion progress, position, and other runtime-owned data. The
portable executable decodes the envelope and the item prefix: eleven optional
equipment records followed by backpack, belt, and special-item containers.
Each concrete item stores category, definition ID, an instance value, optional
grid coordinates, its category-sized state-block length, and that state block.
The nine known player equipment slots, backpack, belt, and Special Item
container are restored and rewritten. The extra two equipment records and
unmapped trailing payload remain byte-for-byte preserved. After those item
containers, loading skips the first counted flag array and restores the next
counted array as the 51 Table 40 transport flags. Scenario position, quests,
mines, companions, and the rest of the dynamic payload are still pending.

## Transport destination table

Table 40 in `Table.Tbd` has 51 rows and three columns:

| Column | Meaning |
|---:|---|
| 0 | Display name |
| 1 | Scenario ID |
| 2 | Scenario entry value |

Every new character starts with row zero enabled. That row is `Remote Town`,
scenario zero, entry 50. The transport panel only displays enabled rows and
packs them into ten visible slots per page; disabled rows do not leave gaps.
For a same-scenario single-player move, the executable looks up the MCT entry
key as `local player number + entry * 4`, so row zero resolves entry key 200.

Scenario folders use zero-padded decimal IDs, not hexadecimal IDs. For
example, Table 40 row one names scenario `6`, which lives at
`Scenario/00000006/`; row 12 names scenario `99000001`, whose directory keeps
all eight decimal digits. Each folder supplies its own `Scenario.Mct`,
`Scenario.Scs`, and `Scenario.Njp`. Row one's entry value 4 selects MCT key 16
for local player zero and starts Wasteland of Pillars at `(35105,-6156)`.

### Related Files
- `Save\%04d.Ssv` - Save slot data (0000-0005)
- `Save\%04d.Bmp` - Save thumbnail
- `Save\M%08d%02d.msk` - Map state/mask data

## Item Database (Item.Ibn)

### Location
`System\Game\Parameter\Item.Ibn`

### Format

The executable loader at `0x00462f80` reads this outer container:

| Offset | Size | Meaning |
|--------|------|---------|
| `0x00` | 16 | `SFItemDataV0000` followed by `0x1a` |
| `0x10` | 4 | Signed checksum of the decoded payload |
| `0x14` | 4 | Compression flag |
| `0x18` | 16 | RCLIB-L header when the flag is one |
| `0x28` | variable | Encrypted compressed bytes |

Item data does **not** use the save-file XOR stream. It uses a 256-byte
substitution table: every encrypted byte selects one decoded byte from that
table. Substitution starts after the RCLIB-L header, then the normal RCLIB-L
decoder expands the result. The checksum is the sum of every decoded payload
byte treated as a signed 8-bit value. The Episode 1 file expands from 332,566
bytes to 2,271,347 payload bytes and has checksum `-6010708`.

The decoded payload contains five item categories. Each category starts with
a signed 32-bit record count. A record then contains:

1. a 32-bit name length and bitwise-inverted Shift-JIS name bytes;
2. a 32-bit description length and bitwise-inverted Shift-JIS description;
3. a fixed binary field block.

The field-block sizes are 804, 764, 672, 140, and 100 bytes for categories
zero through four. Their retail counts are 1264, 1281, 239, 31, and 45. These
known offsets are shared by all five field blocks:

| Field offset | Meaning |
|--------------|---------|
| `0x04` | Definition ID |
| `0x08` | Item subtype |
| `0x1c` | Inventory width in 32-pixel cells |
| `0x20` | Inventory height in 32-pixel cells |
| `0x24` | Item weight |
| `0x28` | Inventory `ItemNNNN.njp` group |
| `0x2c` | Inventory pattern number |
| `0x30` | Ground `Character/ITEM` resource ID |
| `0x34` | Ground CAF chart or pattern selection |
| `0x38` | Inventory shadow/overlay pattern, or `-1` |
| `0x3c` | Ground sprite red strength (`1000` is unchanged) |
| `0x40` | Ground sprite green strength (`1000` is unchanged) |
| `0x44` | Ground sprite blue strength (`1000` is unchanged) |

Weapon records also expose the fields used by the first equipment slice:

| Field offset | Meaning |
|--------------|---------|
| `0x68`..`0x8c` | Ten contributions consumed by the derived-stat refresh |
| `0x94` | Required player level |
| `0xa8` | Player CAF appearance part |
| `0xac` | Appearance-part red strength |
| `0xb0` | Appearance-part green strength |
| `0xb4` | Appearance-part blue strength |

For the Short Sword these values are requirement level 1, CAF part 12, RGB
strengths `1000, 1000, 1000`, and first derived contributions 20 and 100.
Those are decoded from the table rather than attached to the item by name.

The portable loader retains the complete unnamed field block instead of
throwing it away. Ostare's four opening drops have inventory patterns 0, 45,
279, and 270, but those are not used on the map. All four select ground
resource `Character/ITEM/00000000`; Short Sword, Round Shield, Dagger, and
Gold use CAF charts 0, 5, 36, and 30 respectively. Those charts resolve to
visible NJP patterns 77, 82, 113, and 107 plus matching SDW shadows.

The item CAF has palette mode 1 and a chart-priority stride of 2. The
executable therefore selects palette `chart * 2 + cell priority`, rather than
using each NJP pattern's default palette. This gives the visible sword,
shield, dagger, and gold palettes 0, 10, 72, and 60. The separate SDW file
contains one shared shadow palette, so shadows keep their pattern default.
The three strength fields are applied after palette lookup. The opening Round
Shield uses `900, 800, 500`, while the other three drops use
`1000, 1000, 1000`.

Each script-created ground entity now also receives a stable runtime ID.
Pointer hit testing follows the opaque pixels selected by its current CAF
cell, while the label comes from the definition name above. Once the player is
inside the retail interaction range, pickup transfers the category,
definition ID, and quantity to the inventory owner before removing that
ground ID. The same field block remains the source of truth for backpack
footprints, equipment requirements, derived contributions, and player
appearance.

## Table Data (Table.Tbd)

### Location  
`System\Game\Parameter\Table.Tbd`

### Access
Via RKC_RPG_TABLE DLL:
- `ReadBinaryFile()` - Load table
- `GetFromTableNo()` - Get table by number
- `GetRowCount()` / `GetColCount()` - Get dimensions
- `GetValue()` / `GetStrings()` - Get cell data

### Binary layout

The file begins with the 16-byte `TABLE DATA V000\x1a` signature and a
32-bit compression flag. An uncompressed file follows with a 32-bit payload
size and the payload. A compressed file places an ordinary RCLIB-L stream
directly after the flag.

The decoded payload is:

1. a signed 32-bit table count;
2. for each table, its number, row count, and column count;
3. all signed 32-bit numeric cells in row-major order;
4. one string for every cell, stored as a 32-bit byte count followed by
   bitwise-inverted bytes.

The portable `RKC_RPG_TABLE` static library keeps the numeric and string cells
together and looks tables up by their stored number. The Episode 1 database
contains 138 tables.

`0x00440f70` uses table 901 for a new male player and table 900 for a new
female player. Both tables have 13 rows and five columns. Column zero supplies
the initial values; the other four columns hold per-growth-step values used by
later progression:

| Row | Female table 900 | Male table 901 |
|-----|-----------------:|---------------:|
| 0 | 100 | 100 |
| 1 | 128 | 128 |
| 2, life | 150 | 140 |
| 3, mana | 150 | 160 |
| 4 | 140 | 140 |
| 5 | 10 | 8 |
| 6 | 10 | 12 |
| 7 | 10 | 10 |
| 8 | 10 | 10 |
| 9–12 | 40 | 40 |

Only the life, mana, and second-row movement relationship are named so far.
The remaining rows stay numbered until their consumers establish the retail
names.

## Scenario Data

### Scenario.Mct
The fixed header, preload lists, object/PEOPLE records, remaining entity-group
boundaries, entry table, and footer are documented in
[Scenario Files](#scenario-files). Enemy and item tails are still being mapped
from `0x00427b50`.

### Scenario.Scs
Compiled scenario script containing flags, messages, status triggers,
sentences, commands, and typed operands. See
[ShadowFlare's script engine](script-engine.md) for the known binary layout,
Remote Town inventory, and portable interpreter architecture.

## Audio Files

### Background Music (.Voc)
The executable contains an 11-entry path table. The preserved Episode 1 data
set ships seven of those tracks:
- `System\Game\Music\BGM00.Voc` - BGM 1
- `System\Game\Music\BGM01.Voc` - BGM 2
- `System\Game\Music\BGM02.Voc` - BGM 3
- `System\Game\Music\BGM03.Voc` - BGM 4
- `System\Game\Music\BGM04.Voc` - BGM 5
- `System\Game\Music\BGM05.Voc` - BGM 6
- `System\Game\Music\BGM06.Voc` - BGM 7
- `System\Title\Music\BGM00.Voc` - Title screen music

Scenario MCT records select these tracks by zero-based index. Remote Town uses
index 0, so its looping track is `System\Game\Music\BGM00.Voc`.

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

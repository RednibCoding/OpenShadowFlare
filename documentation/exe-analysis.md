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
| 0x0040CEA0 | RenderMissionList - two pages, script state, and detail panel |
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
inserts both static scenery and the current dynamic actor block, then calls
`RKC_RPGSCRN_OBJECTDISP::SortDisplayObject` on each combined list.

Insertion first groups status classes and compares the projected
`position + (judgement.left, judgement.top)` point. The final sort cannot be
replaced by one Y key: it compares the absolute left, top, right, and bottom
edges of every remaining judgement rectangle. A candidate is held back when
another rectangle begins before its right and bottom edges and ends before at
least one of them; that other entry must be drawn first. All comparisons are
strict, so merely touching edges do not create a dependency. This is what puts
a character behind Remote Town's long walls and houses even when the visible
sprite covers a large screen area.
Status bits `0x100`, `0x80`, and `0x20` select classes one, two, and three;
class zero is the default. The packet sequence draws non-default shadows and
objects first, followed by default shadows and objects.

The portable `RKC_RPGSCRN` library now owns that exact ordering rule. The
renderer supplies decoded OBL bounds and the player/NPC judgement rectangles,
keeping DLL-derived behavior out of the executable-owned render code.

The MCT loader at `0x00427b50` first reads a 16-byte
`MCED DATA v0000\x1a` signature, two 260-byte paths, two unknown 32-bit
values, the music index, and a 256-byte title. Its variable entity section is
followed near EOF by a count and 16-byte entry records in key, world X, world
Y, direction order. `0x00427930` searches those records by key. The portable
loader now uses scenario `00000000`'s map path and entry key zero rather than
embedding `f00_01`, (`89898`, `2811`), direction 3, and music index 0 in
`WorldScene`.

Gameplay pointer selection starts at `0x0040ede0`, which asks `0x004165d0` to
collect the current display objects inside the configured pointer square. A
candidate must have an opaque pixel from a visible NJP part in that inclusive
square; this is not a loose actor-bounds test. Turning the range option off
reduces the test to the exact cursor tip. Within one priority group, an exact
tip hit wins first and the nearest candidate wins otherwise. The candidates
also keep their normal world depth order and are grouped by the five
priorities stored in `SFlare.Cfg`. With the retail defaults, a ground item
wins over a person when both are in range.

`0x0040ee70` scans that result from front to back. For a person it projects the
actor's feet, subtracts the MCT label height, draws a half-transparent black
plate around the centered 6-by-12 name, then draws a black one-pixel shadow
and the actor's configured name color. Ground items use the name from
`Item.Ibn`; ordinary money is formatted as its quantity followed by `Gold`.
The selected object's visible RGB strengths each receive `+300`. Values above
1000 do not multiply the palette color: RKC_UPDIB moves each channel toward
white, which produces the pale hover tint seen in the retail game.

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
click. Type-three ground items use the same approach path. Once close enough,
`0x004526a0` calls the local player's inventory insertion routine and removes
the scenario entity only when ownership has transferred successfully.

The portable first pickup checkpoint keeps that separation. `WorldScene`
owns the stable ground entity and pending approach, while `PlayerInventory`
owns the accepted category, definition ID, and quantity. Gold fills existing
stacks to 10,000 as `0x00449ef0` does. The retail 9-by-4 placement grid,
multi-cell item sizes, full-inventory failure, and inventory panel are still
part of the next inventory slice.

Inventory artwork goes through `0x00465cb0`, separately from the world item
CAF path. After drawing an ordinary weapon or armor icon, it divides current
durability times 100 by maximum durability. Results from zero through nine
draw `Status.njp` pattern 16 at the lower-right of the complete multi-cell
footprint. Nonzero durability uses the low four bits of the gameplay update
counter for an eight-update-visible, eight-update-hidden cycle. Zero
durability skips that blink and leaves the warning visible. Backpack,
equipment, and pointer-held items all reach the same drawing rule. The nearby
pattern-17 branch depends on a different runtime item state and remains
unclassified rather than being folded into the condition model.

`0x00454210` initializes the executable's shared movement controller and
`0x00454930` advances it. It is not an A* route search. A direct collision
sweep is followed by stateful cardinal obstacle-edge steering. Controller
modes cover fixed points, scenario actors, other players, bounded wandering,
and related approach behavior. Calls from the player, PEOPLE actors, and enemy
actors all reach this controller. `RKC_RPG_AICONTROL` chooses enemy intent and
parameters; it does not contain a second enemy pathfinder.

The portable `MovementController` now keeps the same pair of cardinal
movement and wall directions between updates. `0x00414990` sweeps along the
dominant axis, interpolates the other coordinate with integer arithmetic, and
returns the last free point before contact. During cardinal edge movement it
also checks a one-pixel strip on the wall side; that small side step is what
lets an actor stay against an edge and turn its corner without an axis-slide
shortcut.

On the first blocked direct step, `0x00454930` compares the attempted quadrant
with the returned contact. If no movement was possible, it probes east, south,
west, or north in the quadrant-specific retail order. Those results select
the cardinal movement direction and the side occupied by the wall. A blocked
edge step swaps them by taking the opposite wall direction. Leaving an edge
is controlled by the signs of the remaining X/Y distance and the exact
movement/wall pair. It is not based on whether the Euclidean distance happened
to improve.

The portable implementation follows those tables directly and has no A*
fallback. Live town actors take part in the same judgement query. The selected
interaction actor remains solid; mode-one interaction ends at the 159-unit
rectangle gap before collision. Player and PEOPLE movement share this owner,
including facing during detours. The PEOPLE walk at `0x0045da25` passes
dynamic collision mask `0xffffffff`, just like the player. Its own character
number is the only live entry excluded by `0x004145b0`. The portable world
therefore keeps one live blocker set for the hero and all PEOPLE actors,
excludes only the actor currently moving, and refreshes positions as actors
are updated.

Remote Town fixtures cover the sacks beside Ostare, the Ostare-to-Malse
approach with the town actors present, longer companion trips made from
successive ordinary movement commands, and a wandering PEOPLE actor steering
around another live actor. Per-actor retail type masks, enemy integration,
and the remaining controller modes are still follow-up work.

The controller's cardinal values are `1 = north`, `2 = south`, `3 = west`,
and `4 = east`. The initial one-pixel probes are ordered by the attempted
direction:

| Attempt | Probe order if contact equals the start | Selected movement / wall |
|---|---|---|
| southeast | east, south, west, north | east/south, south/east, west/south, north/east |
| northeast | east, north, west, south | east/north, north/east, west/north, south/east |
| southwest | west, south, east, north | west/south, south/west, east/south, north/west |
| northwest | west, north, east, south | west/north, north/west, east/north, south/west |
| south or north | east, then west | east/vertical, then west/vertical |
| east or west | south, then north | south/horizontal, then north/horizontal |

The edge-stop tests are equally specific. For a destination northwest of the
actor, the stop pairs are south/east and east/south. Northeast uses
south/west and west/south. Southwest uses north/east and east/north.
Southeast uses north/west and west/north. On a shared axis, movement directly
away from the destination stops the edge walk. These pairs explain why a
generic “distance got smaller” test produces loops that the retail code does
not.

Player movement calls `0x00454930` with dynamic collision enabled and mask
`0xffffffff`; the resolver uses its low byte. `0x004145b0` excludes only the
moving character number and interprets the mask like this:

| Mask bit | Dynamic character class |
|---|---|
| `0x01` | local character numbers 0 through 3 |
| `0x02` | type 0 |
| `0x04` | type 1 |
| `0x08` | type 2 |
| `0x10` | type 4 |
| `0x20` | type 3 |
| `0x40` | type 5 |

That means the player query treats every relevant live actor as solid. It does
not remove the NPC selected for interaction. The controller's mode-one range
test is what prevents the player from trying to occupy that NPC's rectangle.
The portable player currently supplies all loaded town people to the same
query; preserving the individual class masks matters once enemies and other
players join the portable world.

## Player record and new-game tables

The character-selection object owns a 0x160-byte record beginning eight bytes
into its small wrapper. `0x00440f70` copies all `0x58` dwords into the gameplay
player at offset `+0x10`, then initializes the new-game defaults. The confirmed
record fields are name at `0x00`, gender at `0x18`, the save-menu job value at
`0x1c`, and level at `0x24`.

New characters obtain thirteen values from `Table.Tbd`. The selected table is
`0x385 - gender`, which is table 901 for male and 900 for female. The values
are stored in a slightly shuffled part of the record because current life and
current mana sit beside their base maxima. Runtime `+0x40/+0x44` are base
maximum and current life, while `+0x48/+0x4c` are base maximum and current
mana. Both current values start at their maximum. `0x0044ea60` later builds
the separate derived values which include equipment and status modifiers.

The item panel keeps equipment separate from the backpack. Pointer classifier
`0x00447290` returns main hand for x=480 through 543 and y=16 through 143.
`0x00446320` accepts category-zero items there, compares the definition's
required-level field with player offset `+0x34`, and stores the equipped
instance at player offset `+0x4f4`. Clicking an occupied main hand with an
empty pointer unequips it; carrying another valid weapon swaps the two.

The decoded weapon record maps requirement level from field-block offset
`0x94`, player CAF part from `0xa8`, and that part's RGB strengths from
`0xac` through `0xb4`. Short Sword requires level one and selects part 12.
Its first two entries in the ten-value derived contribution block are 20 and
100. Retail runs both the derived-value refresh at `0x00450080` and appearance
refresh at `0x00444ca0` after changing equipment. The portable equipment owner
keeps those contributions available for combat, updates the visible 30 weight
immediately, and rebuilds the player part mask without making the item panel
own world rendering state.

The second table row is the value consumed by `0x00450d40`. It is 128 for both
new characters, producing movement tier five. This is now read through the
portable `RKC_RPG_TABLE` boundary and owned by `PlayerData`; `PlayerActor`
receives only the resulting movement tier and no longer owns level or other
character data.

Retail saves begin with `ShadowFlare0005` and the same plain 0x160-byte record.
The load menu reads this copy directly. The complete record is repeated inside
the obfuscated payload and restored by `0x0044cac0` along with dynamic objects
and world state.

`0x0044b580` writes the payload size first, followed by one byte from the
Visual C++ `rand()` sequence and a four-byte signed-byte checksum. It XORs
each plain payload byte with that random byte, then replaces the result with
its index in the same 256-byte permutation used by the retail data path.
OpenShadowFlare now writes that envelope and performs the inverse operation
when preserving an existing save. It also follows the retail variable-sized
item stream to restore and rewrite the nine known equipment slots, backpack,
and belt. Grid positions, Gold quantities, durability, quality, and all
still-unnamed instance bytes survive the round trip. The extra equipment
records, special-item container, and unknown trailing payload bytes remain
untouched. The rest of the dynamic payload is still pending.

Primary-button input has two retail behaviors. A press and release is a
latched destination click. Keeping the button down continuously replaces the
destination with the live pointer position, but releasing after that held
state cancels movement immediately.

`0x004562f0` makes the distinction explicit in each 28-byte input record:
offsets `+0x04` and `+0x08` are the previous and current down states, `+0x10`
is the press edge, `+0x14` is the release edge, and `+0x18` counts updates
while the button remains down. `0x00441c00` does not arm release cancellation
until that counter is greater than nine. A normal click can therefore span
several game updates and still latch its destination. The pointer target is
refreshed on every down update either way.

The portable gameplay state now preserves that ten-update threshold rather
than classifying any click longer than one 30 Hz update as a hold. A press
consumed by a speech bubble stays UI-owned until button release, even when
selecting the option closes that bubble. It cannot be reinterpreted as a held
ground command on the following update.

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

## Gameplay HUD and pointer

The fixed gameplay interface is drawn by `0x004039f0`, after
`0x004030f0` has submitted the camera-driven world. Gameplay resource loading
at `0x004127d0` assigns `System\Game\Pattern\Bar.njp` to UPD slot 5.
Patterns 7 and 8 form the fixed y=400 through y=479 bar, pattern 10 marks run
mode, pattern 11 marks walk mode, and pattern 15 frames the experience bar.
Patterns 19 through 28 are the level digits.

Life uses patterns 0 through 2 at x=81, y=426. Mana uses patterns 3 through 5
at x=106, y=453. Both live fills are 206 pixels wide and use
`current * 206 / maximum`, preserving one pixel for a positive value which
would otherwise truncate to zero. Packet clip rectangles reveal the live
portion without scaling the artwork. The other patterns provide delayed
damage/healing colors, particles, conditions, companion state, and later
values which still need their gameplay owners.

The executable registers `IDC_ARROW` once in the window class and contains no
later `SetCursor` call. Hover, selection, and click state are therefore drawn
as world feedback; the pointer itself remains the normal platform arrow.

`0x00416bb0` draws the ordinary gameplay feedback as four one-pixel lines.
The configured range selects half-sizes `0`, `12`, `16`, `24`, or `48`; the
shipped default is the 16-pixel choice. Empty ground uses white at strength
100, any target raises it to 300, and a ground item changes it to
`(224, 224, 0)`. The square is not submitted below y=407 and disappears while
a modal gameplay message owns input.

## In-game settings

`0x004103c0` draws the Escape menu with patterns 59 and 58 from `Status.njp`;
the first is the half-transparent fill and the second is the opaque frame.
Its six original boolean rows begin at y=86 and advance by 16 pixels. The
portable game deliberately leaves the screen-mode row at y=86 blank and always
uses an LWL window, so the first visible setting remains at the retail y=102.
ON occupies x=376 through 425 and OFF occupies x=426 through 463.

The five click-range values occupy 24-pixel cells beginning at x=317 and y=182.
The priority row below them displays classes from priority four down to zero.
Clicking a class shifts every lower class up by one and moves the selected
class to priority zero, which is why it moves to the right-hand end rather
than simply swapping with its neighbor.

The effect and BGM tracks use Status patterns 120 and 68 at y=223 and y=243.
The first 200 slider positions cover DirectSound values `-3000` through zero;
the far-left position is the separate mute value `-10000`. BGM changes update
the playing world voice immediately. Escape closes the panel and ordinary
world input remains suspended for every frame owned by it.

The Help row at y=286 switches to `0x0040e710`. Status pattern 10 is the
640-by-415 authored frame; pattern 66 supplies the 230-by-128 ground preview at
`(64, 70)`, surrounded by the original half-opacity one-pixel outline. The
player is drawn at `(152, 148)` with CAF chart 7. The mouse and keyboard tables
are not rewritten descriptions: their original text, colors, 6-by-12 font
cells, two-pixel keyboard line spacing, and coordinates are preserved.

Opening Help from the settings row also starts the common modal-tab animator
at `0x004088b0`. Status patterns 27 through 30 slide the `CLOSE` tab from
y=413 to y=393 and then pulse in the retail eight-phase order. Escape or a
click above y=412 closes Help. The `H` shortcut opens the same reference page
directly. The companion preview is conditional in retail; the portable
renderer will add it when the player-owned companion system exists.

The Map row at y=270 and the `N` shortcut switch to `0x0040d4d0`. The screen
keeps the right half of the world visible and clips its own content to
`(32, 40)` through `(318, 374)`. Pattern 0 from the current scenario's
`Scenario.Njp` is positioned in one-tenth real-screen coordinates, with the
local player held at `(160, 210)`. A separate map-sized mask begins black.
Gameplay clears a 68-by-46 rectangle in that mask around every visited
position, which reveals the overview without exposing unexplored territory.

`MapIcon.njp` patterns 0 and 1 form the local `PLAYER.` marker. It is visible
for 15 updates and hidden for five. Arrow keys change the map origin by 16
pixels horizontally or 10 vertically; Enter clears both offsets. Status
pattern 71 supplies the authored half-screen frame and pattern 118 pulses over
60 updates. The scenario title is drawn at `(72, 50)` over its half-opacity
label backing.

Unlike Help and the Mission List, the Map is not a pausing modal. Its active
UI flags are `0x11`: the left 320 pixels belong to the Map while the world
keeps updating in the right 320-pixel viewport. The camera anchor moves from
`(320, 240)` to `(480, 240)`, so the local player stays centered in the visible
half, and mouse picking uses the shifted projection. Clicks in the Map panel
do not become world commands. Escape or `N` closes it and restores the normal
camera anchor; the secondary-click handler at `0x0044ad80` also closes it for
clicks above the HUD. Retail loads and saves the exploration masks as
`Save/M%08d%02d.msk`; portable
per-save mask persistence and the three other online-player markers remain
to be connected.

The Save and Return row occupies y=302 through 313, and Save and Exit occupies
y=318 through 329. They enter confirmation states two and three. Both dialogs
draw their prompt at y=170, with YES at `(336, 202)` and NO at `(384, 202)`.
Opening a dialog and accepting it use sample 56; NO uses sample 55 and returns
to the ordinary settings state. After YES, states one and four draw
`Now saving the data ` for one update. A successful state-one save then
transitions to the title, while state four ends the game.

When `Save Image at Game End` is enabled, the render loop first draws world
layers 20, 19, and 18 into a separate preview surface. `0x0044e830` writes the
centered player view beside the save as `Save\%04d.Bmp`. The bitmap is 391 by
114 pixels, uncompressed 24-bit BGR, and does not include the HUD or Escape
menu. The load screen places it at `(224, 60)` without scaling.

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
| 0x004039f0 | 3437  | Gameplay HUD, bars, level, and status packets |
| 0x0041d970 | 2936  | State2Handler_Main - main gameplay update |
| 0x00421e10 | 2710  | CharacterCreation - class/gender select |
| 0x004239b0 | ~500  | LoadSaveSlotInfo - read save headers |
| 0x004021b0 | 129   | FindSaveSlot - find free save slot |

### Cleanup
| Address    | Function Name | Description |
|------------|---------------|-------------|
| 0x00401b90 | Shutdown      | Release all subsystems |

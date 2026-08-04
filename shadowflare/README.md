<!--
Copyright (C) 2026 Michael Binder and contributors

This file is part of OpenShadowFlare.

OpenShadowFlare is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

OpenShadowFlare is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.

You should have received a copy of the GNU General Public License along with
OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
-->

# ShadowFlare

This is the new, small C99 game runtime. `src/SF_EXE/` is still extremely
useful as a working reference, but this code is being written fresh instead of
being moved over wholesale. The aim is to keep the game easy to follow while
still preserving sensible boundaries between memory, game rules, rendering,
and the outer application loop.

Before changing this runtime, read the [standing runtime rules](RULES.md).
They make the 33 MHz CPU, 8 MiB main-RAM, and 4 MiB video-RAM constraints
explicit and describe the boundaries that new code must preserve.

The current executable opens a 640×480 RGB555 surface and reconstructs the
retail title screen from the original NJP, CAF, and VOC files. Game rules run
at the retail 30 Hz cadence, while integer-only interpolation presents moving
gameplay at 60 Hz on capable targets. It supports mouse, keyboard, and
controller menu input and keeps the fade, menu brightness, smoke timing,
music, and effects in their recovered order. New Game now continues through
the retail character-creation screen: gender selection, the portrait slide and
fade, 15-byte name entry, visible caret, menu sounds, and the
Online/Single/Back choices are present.
Load Game now opens the retail six-entry save screen, reads the original save
summaries, swaps the selected thumbnail, supports mouse and two-column
keyboard/controller navigation, and includes Continue, Delete, Back, Exit,
the delete confirmation, and game-mode dialogs. Choosing Single Mode now
continues through the loading hand-off into the first gameplay slice: a
scrolling Remote Town viewport loaded entirely from scenario, GND, OBL, LST,
NJP, and SDW retail data. The selected male or female hero now appears at the
authored MCT entry, wears the new-character Leather Cloth described by
`Item.Ibn`, and plays the retail idle, walk, and run charts with matching
shadows and scenery depth. Mouse movement follows the retail distinction
between a latched click
and a held pointer that stops on release, while `R` switches between the
recovered walk and run speeds. Static map collision and the retail cardinal
edge-following route controller are now active as well, including the full
route around the sacks beside Ostare. This is deliberately not an A* search.
Remote Town's seven PEOPLE records now come from `Scenario.Mct`. Their idle
and walk animations, shadows, part masks, and depth ordering use the retail
character archives, while the periodic actor rows in `Scenario.Scs` decide
which of the four town companions is visible for the current companion type.
The authored PEOPLE timing and wander bounds drive Ostare's movement. PEOPLE
actors and the player share the same dynamic collision query and retail edge
controller, so they route around one another without a second pathfinder.
The seven type-zero records in the same MCT are live objects rather than map
scenery. Their separate `Character/OBJECT` static and animated resources,
shadows, script state, color strengths, opacity, judgement boxes, and display
order now feed the ordinary world passes. That makes the Warehouse and both
transport points solid when their authored state says they are solid. Opaque
object pixels also participate in hover and click selection, including the
MCT-owned nameplate. Clicking the Warehouse now routes into range, starts its
authored status-zero sentence, and lets opcode 41 toggle the existing Special
Item panel. The script emits a one-shot game request; the UI owns the panel and
keeps an independent right Inventory open. The matching transport object now
uses the same boundary: its real status-zero sentence reaches opcode 37 and
opens the retail left-hand destination panel while an independent Inventory
can remain open on the right. All 51 names and scenario/entry pairs are
streamed from Table 40, enabled rows are compacted into ten slots per page,
and the arrows, hover rows, frame, and sample 58 come from the original game.
Selecting Remote Town resolves entry key 200 through the active MCT instead of
hardcoding world coordinates, consumes the panel click before world movement,
and closes only the transport panel. Opcode 38 supplies the matching scripted
close request. The real periodic activation sentence now discovers row zero,
plays sample 80 once, fades both authored transport objects, and draws its
opcode-27 `Remote Town` label above the script-selected object. Cross-scenario
rows now use the same fixed travel request as opcode 17. The screen runtime
releases the old scenario arena, loads the selected scenario and MCT entry,
and preserves the live player, inventory, companion, quest, script, and
transport owners.
The default retail click-range square now selects opaque PEOPLE pixels, hover
adds the pale tint and authored nameplate, and clicking a distant actor routes
the player to the recovered `0x9f` interaction distance without issuing a
ground command behind the actor. The SCS interpreter now enters Ostare's
status-zero conversation, keeps its fixed call stack across message waits,
runs the first two status-one callbacks, and reads the original Shift-JIS
message table. Ostare stops wandering, turns toward the player, and speaks
through the actor-anchored five-piece `Hukidasi.njp` bubble. Authored `~`
choice markers are hidden, their exact text ranges are clickable, hover changes
the selected red option, and clicks outside a choice cannot leak through as
movement. Harley's complete `Explanation` branch proves choice selection, two
ordinary follow-up messages, and the final actor release. Layout and pointer
resolution stay in `ui/`; script state and actor behavior stay in
`interpreter/` and `game/`. Ostare's next callback now executes the shipped
opcode 10 commands through a small world-service boundary. It creates the
Short Sword, Round Shield, Dagger, and 200 Gold with their retail positions,
colors, two-bounce motion, and landing sounds. Their CAF cells, sparse
NJP/SDW patterns, and explicit palettes are discovered from the active script
and `Item.Ibn`; Remote Town IDs are not hardcoded into the game or renderer.
Those drops can now be selected through their opaque CAF artwork, approached
through the same retail edge-routing path as actors, and picked up into a
fixed 9x4 inventory owner. Item names, dimensions, weight, durability, and
identification state come from the streamed `Item.Ibn` records. Gold keeps
the original 10,000-piece stack limit, failed pickups leave the inventory
untouched and replay the drop bounce, and pickup sounds follow the retail
item category and weight rules. Hovering an item applies the original pale
tint and draws its quantity or decoded name above the world sprite.
The first always-visible gameplay HUD is live too. It draws the authored
`Bar.njp` pieces over retail's black lower surface, with the level digit,
life, mana, experience, and walk/run indicator coming from the player owner.
A streaming `Table.Tbd` reader extracts only the active gender's 13 base
parameters and the current level's experience threshold; the 460,387-byte
decoded table payload is never retained. HUD input is resolved in `ui/`, so
clicking its surface cannot leak through as a movement command. `I` and the
authored ITEM button now open the right-hand inventory panel while the live
world shifts to the retail x=160 camera anchor. The panel reads Gold from the
fixed owner and draws picked-up items in their real 9x4 cells from the separate
inventory patterns in `Item0000.njp` through `Item0013.njp`. Its frame, gender
silhouette, values, and Close tab come from the required pieces of
`Status.njp`; unrelated parts of those large archives are never retained. The
lower HUD also owns the retail 4x2 belt: `1` through `8` and right-click
consume Tablets and Capsules, while mines use their separate 5/10 counter
instead of occupying bag cells.
Leaving the pointer over a backpack or equipped item for three game updates
now opens the retail information overlay. The active definitions gain their
description, price, combat bonuses, and elemental values during the same
streaming `Item.Ibn` pass; the full database is still discarded. The tooltip
uses the original translucent backing, faint frame, tier color, sale-price
rule, and wide one-cell-item layout, and follows the pointer without becoming
game state or renderer behavior. Weapons and armor below ten percent
durability now add the authored `Status.njp` condition marker at the lower
right of their backpack, equipment, or pointer-held footprint. It blinks for
eight 30 Hz updates on and eight off while durability remains above zero; a
broken item keeps it visible. The belt remains unchanged because retail only
allows consumables there.
`X` now opens the retail Special Item panel on the left. Its independent fixed
9x10 owner uses the same pointer-held item, transactional placement, swaps,
Gold merging, condition marker, and information overlay as the backpack. The
left and right panels can remain open together; opening only one shifts the
live world and its input anchor to the exposed half of the screen. Items move
directly between either owner without a second transfer path, and the fourth
retail save container restores exact Special Item cells on load.
`S` and the authored STATUS button open the Status tab of the shared
Status/Magic window. The live Status page uses retail pattern 5 for its frame
and labels, then overlays the character identity, current pools, base and
equipment-adjusted physical and magical values, eight elemental affinities,
and the saved elemental marker. Its game-side profile ignores broken gear and
suppressed off-hand items; the UI only formats and draws the result. Status can
remain open beside Inventory, replaces Special Item on the left, shifts the
world and input by the same integer offset, and is closed together with other
open panels by Escape.

`M` and the authored top tab open the other half of that same owner. Magic uses
retail pattern 6, four six-spell pages, authored icon wells, saved availability,
level and experience arrays, and streamed Tables 16, 17, 27, and 600 through
621. Learned icons can be dragged into eight saved slots and moved without
duplicates. The always-visible bar above the HUD uses its original compact and
selected icons, expands around the selected spell, and follows the retail
full/left/right-panel positions. Clicking its final icon selects normal attack;
every new or loaded gameplay entry starts there. UI code owns the page, hit
rectangles, tooltip, and held icon, while `game/player_magic.c` owns spell data
and applies the emitted selection or assignment. Samples 57 and 58 use a small
general world-event queue and dedicated gameplay-interface assets before TAL
plays them at the runtime edge.

The hero's owned companion now comes from the retail companion tables and
PARTNER archives instead of borrowing a town dog. A fixed six-row player owner
keeps each type's level and experience, while the active profile supplies its
name, movement speeds, life, resource, and draw strengths. The live actor uses
the same integer collision query and cardinal edge controller as the player
and PEOPLE actors, including dynamic actor blockers and retail's close, walk,
run, and far-relocation distances. It starts inactive but still follows;
Space, a controller shoulder button, or the exact bottom-left HUD strip toggles
the authored active/inactive marker. Save loading restores the active type,
all six progression rows, and the defeated countdown. Attacks, damage, death,
revival, Moon, and swapping are intentionally left for their own gameplay
slices.

Choosing a retail save now restores more than its load-screen summary. The
`ShadowFlare0005` envelope is decoded and checksummed as a stream, so even a
large save never needs a second payload-sized buffer. The complete plain
player record restores name, sex, job, level, life, mana, experience, and base
parameters. Its following owned-item stream restores exact backpack, belt, and
Special Item cells plus all eleven equipment slots, including the two hidden
alternate weapon slots. Item definitions and artwork are still loaded through
the active map's ordinary resource request, not from save-specific shortcuts.
The three counted progress owners restore quest state, unlocked transports,
and all 1,000 persistent script/conversation values before the scenario's first
periodic pass. Mine count, walk/run pace, scenario, and authored entry restore
through the world boundary too. Maps which the C99 runtime does not implement
yet still fail honestly instead of silently moving that character back to
Remote Town. A failed entry returns to the same highlighted Load Game row;
it no longer closes the application and looks like a crash when `osf` was
started from the desktop. Non-town maps may contain `?` pattern-list slots and
no PEOPLE records; both are valid retail data. Their periodic scripts can also
retain placed-effect descriptors and use the shared integer random source
without preventing a saved character from entering the world.

## Hard limits

The limits are part of the code, not just goals written in a document:

- 8 MiB total main RAM;
- 1 MiB of that is held back for code, stacks, and platform state;
- 7 MiB is available to the caller-owned game arena;
- 4 MiB video RAM;
- one 640×480 RGB555 framebuffer uses 600 KiB;
- the remaining 3,496 KiB is the initial video-asset budget.

Desktop builds use fixed arrays to emulate the two memory pools. A future PS1
backend can point the video pool at real video memory and size the main arena
from the memory left after the executable and stacks. Nothing in the game core
allocates from the heap. The original PlayStation's 2 MiB/1 MiB split remains
a later porting target that can use dedicated asset packages without forcing
streaming and cache eviction into the general game implementation.

## Folder layout

- `core/` contains small utilities and the memory budgets;
- `assets/` owns screen assets and the centralized retail archive path table;
- `data/` reads the retail scenario, map, artwork, audio, and save formats;
- `game/` owns game state and rules, including the current world and camera;
- `interpreter/` applies the implemented parts of retail scripts to game state;
- `render/` contains the small backend-neutral drawing API and damage helper;
- `screens/` owns screen lifetimes, transitions, and high-level composition;
- `ui/` owns HUD, panels, conversations, tooltips, and other game interface;
- `runtime/` connects the game to the platform-neutral TWL and TAL APIs;
- `main.c` only provides the fixed memory pools and starts the runtime.

Platform SDK headers and operating-system calls do not belong here. New target
support goes into TWL or TAL backends, while the game continues to use the same
interfaces.

The executable is built at `build/<platform>/<config>/shadowflare/osf` (or
`osf.exe` on Windows). The older reference executable remains
`ShadowFlare_rebuilt`, so it is always clear which implementation is being
tested. Presentation defaults to 60 Hz with 30 Hz integer-interpolated game
states. Constrained builds can select 30 Hz without adding target code:

```sh
cmake -S . -B build/linux/release \
  -DOPENSHADOWFLARE_C99_PRESENTATION_HZ=30
```

Debug-tool builds also produce `osf-measure` beside the game. It loads every
implemented C99 screen through the real screen runtime and prints the total,
screen-scoped, and remaining arena bytes. This keeps memory-budget checks out
of platform backends and gives later runtime profiling a small dedicated home:

```sh
./build/linux/debug/shadowflare/osf-measure
```

The same build option enables a small live profiler in `osf`. The outer
runtime records FPS, current and peak arena usage, and average and peak
framebuffer-fill and presentation-preparation times. The presentation metric
ends before TWL displays or swaps the prepared frame, so vertical-blank waits
cannot inflate it. It uses timestamps supplied through TWL and contains no
target-specific code. The snapshot is ready for a future
top-right debug overlay; no profiling UI is drawn yet. Configuring with
`-DOPENSHADOWFLARE_ENABLE_DEBUG_TOOLS=OFF` omits the profiler source, calls,
test, and `osf-measure` target from the C99 game build.

The executable first looks for the retail `System` folder beside itself. This
means a release can be copied directly into an original ShadowFlare install.
Development builds also find `tmp/ShadowFlare` from their standard
`build/<platform>/<config>/shadowflare/` folder, so they can be started by
double-clicking without changing the working directory:

```sh
./build/linux/debug/shadowflare/osf
```

An unpacked retail game directory can still be passed explicitly as the first
argument. An explicit path is never silently replaced by an automatic one if
it is invalid.

## Current screen budgets

The complete title currently uses 1,489,205 bytes of the 7 MiB main arena,
leaving 5,850,827 bytes free. Its screen-scoped artwork accounts for 1,101,182
bytes. The rest includes TWL/TAL state, game and screen metadata, persistent
8-bit menu music and effects, and one reusable 60,000-byte decode buffer. The
video pool contains only the 614,400-byte RGB555 framebuffer, leaving 3,579,904
bytes.

Static title frames do not redraw the whole screen. A fixed 16-entry damage
list restores only the areas touched by changing menu highlights and smoke.
Blank animation frames are detected while loading and skipped. In the worst
nonblank case, all ten smoke streams together decode at most 57,864 bytes into
the same reusable buffer during one rendered frame.

Character creation releases all title-only artwork before loading its own
assets. It uses 802,128 bytes of the main arena, leaving 6,537,904 bytes free;
414,105 bytes of that total are character-screen artwork and font data. Shared
NJP parts are decoded only once even when several patterns reference them, and
a static character screen is not filled again until a visible state changes.

The load-game screen uses 762,344 bytes of the main arena, leaving 6,577,688
bytes free. Its screen-scoped artwork, font, and selected save preview account
for 374,321 bytes. Save headers stay in a fixed six-entry catalog, while only
the selected 391x114 thumbnail occupies memory. Changing selection decodes the
new preview into the same 89,148-byte RGB555 buffer; idle frames perform no
file access and do not refill the framebuffer.

The complete Remote Town gameplay screen uses 7,222,236 bytes of the main
arena, leaving 117,796 bytes free. Its screen-owned scenario, script, map,
player, owned companion, PEOPLE, type-zero objects, ground-item,
inventory-panel, transport, equipment, and UI data and artwork account for
6,834,213 bytes. GND rendering data is decoded directly from its compressed three-plane
stream into two bytes
per tile, so the 300x300 town grid occupies 180,000 bytes instead of retaining
the 540,000-byte source layout.

The same runtime reload measured in Near Remote Town uses 6,280,168 bytes,
leaving 1,059,864 bytes free; 5,892,145 bytes belong to that screen. The
transition never retains two maps at once. Its fixed request lives with the
game owner, while the screen arena is rewound before the next scenario is
decoded.

The GND movement plane is active without retaining its 1,451,808-byte raw
16-bit expansion. Retail movement only reads its low two flags, so the 852x852
Remote Town plane is kept as a packed two-bit map using 181,476 bytes. Static
OBL judgement and the player rectangle feed a direct collision sweep followed
by the executable's stateful cardinal edge steering. There is no route list,
heap allocation, or file access during movement.

Map artwork is owned by the active map rather than the camera or a
scenario-specific asset list. When a map is entered, the loader scans its own
ground cells and OBL objects once and retains every referenced NJP/SDW pattern
and palette. Camera movement therefore performs no file access and cannot
reveal unloaded tiles or scenery. The static world is then drawn in the retail
ground, non-default shadow/object, and default shadow/object passes. Authored
NJP bounds cull the current viewport before sorting, and the NJP header must
identify a resource as a real shadow before it can enter a shadow pass. The
hero joins those same sorted passes using the retail player judgement box, so
gates, walls, roofs, and ordinary scenery can appear on the correct side of the
actor. Visible objects drawn in front of the hero use the retail pixel-level
51×61 obstruction check and are capped at half opacity unless their OBL status
explicitly disables fading.

The player archives are deliberately not loaded whole. The male NJP alone
would expand to more than 20 MiB. A two-pass sparse loader scans the large file
without retaining its full metadata and decodes only the body, equipped armor,
weapon, and shadow patterns used by idle, walk, and run. Its pattern metadata
is arena-sized to the selected bank rather than embedded in every screen.
`Item.Ibn` is streamed in the same way to recover the Leather Cloth appearance
fields without keeping its 2.27 MiB decoded payload. The active map also
preloads only the distinct player CAF parts referenced by its retained weapon
and armor definitions, so equipping an item never performs file access during
play. The equipment owner selects and tints those retained parts; inactive
candidate parts do not draw. Stationary frames restore and redraw only the
measured player rectangle in full retail depth order; scrolling frames redraw
the changing world once.

PEOPLE artwork follows the same sparse rule. The loader first combines the
parts needed by actors that share a retail resource, keeps the idle patterns
referenced by their eight directions, and adds walk patterns only when an
authored actor using that resource can wander. Offscreen actors are removed
before depth sorting and drawing. The decoded SCS tables and message bytes live
in the gameplay screen arena as well, so returning to a menu releases them
with the rest of the map instead of making every screen pay for script memory.
The interpreter uses fixed temporary, persistent, quest, and 16-frame call
stack storage. Command semantics, operand access, and the resumable status loop
are separate small files rather than one growing interpreter source.

Type-zero object artwork is prepared from the active MCT in the same way.
Remote Town retains only its three referenced `Character/OBJECT` resources:
the exact static NJP patterns, the SDW patterns which actually exist, and the
CAF cells needed by the animated transport point. This slice adds 328,656
bytes to the measured gameplay total. Loading, decoding, and filename-case
fallback all happen at the screen boundary; object update, picking, and draw
passes perform no file access or allocation.

The owned companion is prepared just as narrowly. The active save row chooses
one PARTNER resource, and its loader keeps only charts zero through two for the
eight ordinary directions, deduplicating every referenced NJP and SDW pattern.
That companion slice accounts for most of the latest gameplay increase, so the
remaining 117,796-byte headroom is now a hard warning for upcoming combat and
effect work: later slices must retire or stream existing screen data rather
than quietly raising the arena limit.

World pointer hit testing lives in `ui/`, where the loaded sparse actor cells
are already available. It produces a small actor intent before each game
update; `game/` owns the actual approach and distance rule. The common font is
loaded once with the map for authored nameplates, along with the five small
speech-frame patterns used by conversations. The translucent click square and
nameplate background use a general RGB555 rectangle blend operation, not a
platform or gameplay-specific renderer path.

Conversation input follows the same boundary. `ui/conversation_input.c` turns
the rendered choice spans into an option number and count. The small game-side
conversation owner changes selection, resumes the interpreter, and releases
the speaking actor. No renderer or target backend knows about script choices.

The bottom HUD follows that rule as well. `data/table.c` walks compressed or
plain retail parameter tables through a numeric callback without allocating a
database. `game/player.c` owns the selected starting values, while
`ui/gameplay_hud.c` and `ui/gameplay_companion_hud.c` compose the 22 currently
required `Bar.njp` patterns.
Only the active patterns and their 19 referenced parts are decoded. The
renderer remains unaware of gauges, levels, pace, or HUD hit areas.

Ground items follow the same ownership rule. The active SCS is scanned for
its fixed or initial temporary category/definition pairs, and `Item.Ibn` is
streamed once to retain those eight Remote Town records plus the equipped
Leather Cloth needed by a new hero. The active map
owns one fixed 64-entry item set. The interpreter only evaluates opcode 10
operands and calls the world service; it does not know about item storage,
artwork, rendering, or audio. The screen helper draws the selected CAF cells
in the ordinary depth passes, while TAL playback stays at the outer runtime
boundary. Pointer hit testing and hover labels stay in `ui/`; approach,
fixed-grid placement, gold stacking, rollback on failure, and pickup sound
selection stay in `game/`. The player owns the resulting 36-entry fixed array,
so no heap allocation or retained item-database copy is needed. A second fixed
owner holds the one item currently carried by the pointer. Taking, centered
grid placement, single-item swaps, invalid-placement rollback, and partial
Gold merges all stay in `game/inventory.c`.

The inventory and character panels follow the same lifetime. `Status.njp` is
streamed to retain only 43 required patterns, without constructing
metadata for its other 115 patterns. Item definitions provide the inventory
group, pattern, and optional palette directly; only groups and cells referenced
by the active map's retained definitions are decoded.
`ui/gameplay_inventory.c` composes the panel, while
`ui/gameplay_inventory_input.c` owns its authored rectangles and camera intent.
World drawing, opaque-pixel picking, and movement targeting all receive the same integer x offset, so an open panel cannot make what the player
sees disagree with what a click selects. The UI emits take, place, and world
drop intent without owning the item. `game/world_inventory.c` applies that
intent, keeps durability and identification state across a backpack/ground
round trip, places world drops 200 units away in the selected eight-way
direction, and guards the pointer until release so the drop cannot become a
movement command. The full held icon is drawn last over the HUD and survives
closing a panel, as it does in retail. The left Special Item panel has its own
fixed 9x10 owner but shares this pointer transfer and item presentation path.
Both panels can be open at once; their opposing integer camera offsets cancel,
while either panel alone leaves the other half as the live world.

Magic follows the same prepared-resource rule. Only the 23 required
`MagicIcon.njp` patterns and 24 required `MagicBarIcon.njp` patterns are
decoded when gameplay loads. The table scanner copies the 22 fixed MP/effect
rows, experience thresholds, and bounded help lines into gameplay-owned
storage, then discards the table stream. Panel rendering performs no file
access, allocation, decompression, or format conversion. Its only normal-frame
cost is composing the nine small gameplay-bar entries when that region is
redrawn; the four-page panel is drawn only while open or visibly changing.

The same held-item path now owns all nine visible equipment regions. Their
retail rectangles live in one small UI layout file, while category, subtype,
level requirement, swapping, equipped weight, and off-hand suppression remain
in `game/equipment.c`. A new hero's Leather Cloth begins as a concrete body
item with its original durability. Taking it uses the ordinary pointer owner;
placing armor, weapons, shields, or one-cell accessories validates the target
before replacing anything and emits the original equip sound. Complete item
footprints are centered in the authored regions, and equipment clicks are
consumed before world movement.

The framebuffer still occupies 614,400 bytes of video memory, leaving 3,579,904
bytes there; map artwork remains packed in main RAM for the desktop software
backend.

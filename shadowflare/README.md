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
Pointer interaction and conversations are later slices; none of those rules
have been hidden inside the renderer.

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

The complete title currently uses 1,431,669 bytes of the 7 MiB main arena,
leaving 5,908,363 bytes free. Its screen-scoped artwork accounts for 1,101,182
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
assets. It uses 744,592 bytes of the main arena, leaving 6,595,440 bytes free;
414,105 bytes of that total are character-screen artwork and font data. Shared
NJP parts are decoded only once even when several patterns reference them, and
a static character screen is not filled again until a visible state changes.

The load-game screen uses 704,808 bytes of the main arena, leaving 6,635,224
bytes free. Its screen-scoped artwork, font, and selected save preview account
for 374,321 bytes. Save headers stay in a fixed six-entry catalog, while only
the selected 391x114 thumbnail occupies memory. Changing selection decodes the
new preview into the same 89,148-byte RGB555 buffer; idle frames perform no
file access and do not refill the framebuffer.

The complete Remote Town gameplay screen uses 4,250,220 bytes of the main
arena, leaving 3,089,812 bytes free. Its screen-owned scenario, script, map,
player, and PEOPLE data and artwork account for 3,919,733 bytes. GND rendering
data is decoded directly from its compressed three-plane stream into two bytes
per tile, so the 300x300 town grid occupies 180,000 bytes instead of retaining
the 540,000-byte source layout.

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
fields without keeping its 2.27 MiB decoded payload. Stationary frames restore
and redraw only the measured player rectangle in full retail depth order;
scrolling frames redraw the changing world once.

PEOPLE artwork follows the same sparse rule. The loader first combines the
parts needed by actors that share a retail resource, keeps the idle patterns
referenced by their eight directions, and adds walk patterns only when an
authored actor using that resource can wander. Offscreen actors are removed
before depth sorting and drawing. The decoded SCS tables live in the gameplay
screen arena as well, so returning to a menu releases them with the rest of
the map instead of making every screen pay for script memory.

The framebuffer still occupies 614,400 bytes of video memory, leaving 3,579,904
bytes there; map artwork remains packed in main RAM for the desktop software
backend.

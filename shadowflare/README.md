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
retail title screen from the original NJP, CAF, and VOC files. It runs at the
retail 30 Hz cadence, supports mouse, keyboard, and controller menu input, and
keeps the fade, menu brightness, smoke timing, music, and effects in their
recovered order. New Game now continues through the retail character-creation
screen: gender selection, the portrait slide and fade, 15-byte name entry,
visible caret, menu sounds, and the Online/Single/Back choices are present.
Load Game now opens the retail six-entry save screen, reads the original save
summaries, swaps the selected thumbnail, supports mouse and two-column
keyboard/controller navigation, and includes Continue, Delete, Back, Exit,
the delete confirmation, and game-mode dialogs. Choosing Single Mode now
continues through the loading hand-off into the first gameplay slice: a static
Remote Town viewport loaded entirely from scenario, GND, OBL, LST, NJP, and
SDW retail data. The selected male or female hero now appears at the authored
MCT entry, wears the new-character Leather Cloth described by `Item.Ibn`, and
plays retail idle chart zero with its matching shadow and scenery depth. Player
movement, collision, and scripts are the next layers; they are deliberately
not faked in the map screen.

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
- `render/` contains the small backend-neutral drawing API and damage helper;
- `screens/` composes title, loading, load/save, and future gameplay screens;
- `runtime/` connects the game to the platform-neutral TWL and TAL APIs;
- `main.c` only provides the fixed memory pools and starts the runtime.

Platform SDK headers and operating-system calls do not belong here. New target
support goes into TWL or TAL backends, while the game continues to use the same
interfaces.

The executable is built at `build/<platform>/<config>/shadowflare/osf` (or
`osf.exe` on Windows). The older reference executable remains
`ShadowFlare_rebuilt`, so it is always clear which implementation is being
tested.

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

The complete title currently uses 1,423,725 bytes of the 7 MiB main arena,
leaving 5,916,307 bytes free. Its screen-scoped artwork accounts for 1,101,182
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
assets. It uses 736,648 bytes of the main arena, leaving 6,603,384 bytes free;
414,105 bytes of that total are character-screen artwork and font data. Shared
NJP parts are decoded only once even when several patterns reference them, and
a static character screen is not filled again until a visible state changes.

The load-game screen uses 696,864 bytes of the main arena, leaving 6,643,168
bytes free. Its screen-scoped artwork, font, and selected save preview account
for 374,321 bytes. Save headers stay in a fixed six-entry catalog, while only
the selected 391x114 thumbnail occupies memory. Changing selection decodes the
new preview into the same 89,148-byte RGB555 buffer; idle frames perform no
file access and do not refill the framebuffer.

The first Remote Town gameplay viewport uses 1,007,920 bytes of the main arena,
leaving 6,332,112 bytes free. Its map- and player-scoped data and selected
artwork account for 685,377 bytes. GND rendering data is decoded directly from
its compressed three-plane stream into two bytes per tile, so the 300x300 town
grid occupies 180,000 bytes instead of retaining the 540,000-byte source
layout. Collision
judgement is not loaded by this rendering-only slice yet.

Map artwork follows the current camera rather than a scenario-specific asset
list. The loader reads the map's own ground cells and OBL objects, checks exact
NJP/SDW pattern bounds against the 640x480 viewport, and retains only the
referenced pattern pixels and palettes. The static world is then drawn in the
retail ground, non-default shadow/object, and default shadow/object passes.
The hero joins those same sorted passes using the retail player judgement box,
so gates, walls, roofs, and ordinary scenery can appear on the correct side of
the actor.

The player archives are deliberately not loaded whole. The male NJP alone
would expand to more than 20 MiB. A two-pass sparse loader scans the large file
without retaining its metadata and decodes only the body, equipped armor, and
shadow patterns used by the active idle direction. `Item.Ibn` is streamed in
the same way to recover the Leather Cloth appearance fields without keeping
its 2.27 MiB decoded payload. The initial world draw is complete; later idle
frames restore and redraw only the measured player rectangle in full retail
depth order.

The framebuffer still occupies 614,400 bytes of video memory, leaving 3,579,904
bytes there; map artwork remains packed in main RAM for the desktop software
backend.

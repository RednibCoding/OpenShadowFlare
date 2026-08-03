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
They make the 33 MHz CPU, 2 MiB main-RAM, and 1 MiB video-RAM constraints
explicit and describe the boundaries that new code must preserve.

The current executable opens a 640×480 RGB555 surface and reconstructs the
retail title screen from the original NJP, CAF, and VOC files. It runs at the
retail 30 Hz cadence, supports mouse, keyboard, and controller menu input, and
keeps the fade, menu brightness, smoke timing, music, and effects in their
recovered order. New Game now continues through the retail character-creation
screen: gender selection, the portrait slide and fade, 15-byte name entry,
visible caret, menu sounds, and the Online/Single/Back choices are present.
Choosing a game mode currently reaches the black loading hand-off because the
new C99 gameplay screen has not been reconstructed yet.

## Hard limits

The limits are part of the code, not just goals written in a document:

- 2 MiB total main RAM;
- 512 KiB of that is held back for code, stacks, and platform state;
- 1.5 MiB is available to the caller-owned game arena;
- 1 MiB video RAM;
- one 640×480 RGB555 framebuffer uses 600 KiB;
- the remaining 424 KiB is the initial video-asset budget.

Desktop builds use fixed arrays to emulate the two memory pools. A future PS1
backend can point the video pool at real video memory and size the main arena
from the memory left after the executable and stacks. Nothing in the game core
allocates from the heap.

## Folder layout

- `core/` contains small utilities and the memory budgets;
- `assets/` owns screen assets and the centralized retail archive path table;
- `data/` reads the retail NJP, CAF, RCLIB, and VOC formats;
- `game/` owns game state and rules;
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

The complete title currently uses 1,422,733 bytes of the 1.5 MiB main arena,
leaving 150,131 bytes free. Its screen-scoped artwork accounts for 1,101,182
bytes. The rest includes TWL/TAL state, game and screen metadata, persistent
8-bit menu music and effects, and one reusable 60,000-byte decode buffer. The
video pool contains only the 614,400-byte RGB555 framebuffer, leaving 434,176
bytes.

Static title frames do not redraw the whole screen. A fixed 16-entry damage
list restores only the areas touched by changing menu highlights and smoke.
Blank animation frames are detected while loading and skipped. In the worst
nonblank case, all ten smoke streams together decode at most 57,864 bytes into
the same reusable buffer during one rendered frame.

Character creation releases all title-only artwork before loading its own
assets. It uses 732,064 bytes of the main arena, leaving 840,800 bytes free;
410,513 bytes of that total are character-screen artwork and font data. Shared
NJP parts are decoded only once even when several patterns reference them, and
a static character screen is not filled again until a visible state changes.

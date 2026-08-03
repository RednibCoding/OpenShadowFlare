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
recovered order. The character screen is still the next unfinished screen, so
choosing New Game currently reaches a black hand-off rather than a usable menu.

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

Run it from the repository root so the default `tmp/ShadowFlare` data path is
available:

```sh
./build/linux/debug/shadowflare/osf
```

An unpacked retail data directory can also be passed explicitly as the first
argument.

## Current title budget

The complete title currently uses 1,417,018 bytes of the 1.5 MiB main arena,
leaving 155,846 bytes free. That includes TWL/TAL state, game and asset
metadata, the indexed background and menu art, all ten compact smoke streams,
8-bit music and effects, and one reusable 60,000-byte decode buffer. The video
pool contains only the 614,400-byte RGB555 framebuffer, leaving 434,176 bytes.

Static title frames do not redraw the whole screen. A fixed 16-entry damage
list restores only the areas touched by changing menu highlights and smoke.
Blank animation frames are detected while loading and skipped. In the worst
nonblank case, all ten smoke streams together decode at most 57,864 bytes into
the same reusable buffer during one rendered frame.

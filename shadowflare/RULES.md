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

# Runtime rules

These are the standing rules for the small C99 game under `shadowflare/`.
They are constraints, not distant optimization goals. If a new feature cannot
fit them, we stop and rethink the feature instead of quietly raising a limit.

## The target machine

The baseline is deliberately harsh:

- roughly 33 MHz CPU;
- 2 MiB main RAM;
- 1 MiB video RAM;
- 640×480 output using packed RGB555;
- 30 Hz game updates and presentation, matching the retail game.

At 30 Hz, 33 MHz is only about 1.1 million CPU cycles per frame. A 640×480 image
contains 307,200 pixels. Touching the complete framebuffer is therefore a
serious operation, not a harmless default.

The current memory split reserves 512 KiB of main RAM for executable code,
stacks, and untracked target needs. The caller-owned game arena gets the other
1.5 MiB. One RGB555 framebuffer occupies 600 KiB of video RAM, leaving 424 KiB
for visible artwork and other video resources.

## Code and boundaries

- Game code is straightforward C99. Do not add C++.
- Do not recreate the old DLL folder structure or one static library per DLL.
  Reconstructed DLLs and `SF_EXE` are behavioral references, not the new
  architecture.
- Keep files concerned and reasonably small, but do not invent layers merely
  to avoid a direct function call.
- `core/` cannot depend on game, rendering, or runtime code.
- `game/` owns rules and state. It cannot depend on rendering or runtime code.
- `render/` may read game state, but cannot depend on runtime integration.
- `screens/` composes complete screens from game state, assets, and renderer
  operations. Title, loading, load/save, and gameplay screens belong there.
- `render/` contains only the small render API and its reusable primitives;
  it must not accumulate game screens.
- TWL and TAL belong at the outer runtime boundary only.
- Platform SDK headers and operating-system calls stay out of `shadowflare/`.
  Future target adapters belong outside the game folder.

The `shadowflare_c99_boundaries` test checks the rules that can be checked
mechanically, including language choice, dependency direction, platform
headers, legacy libraries, and heap allocation.

## Memory

- No `malloc`, `calloc`, `realloc`, or `free`.
- Every long-lived allocation comes from an explicit caller-owned arena.
- Temporary work uses a fixed scratch arena or an arena mark and rewind.
- Every collection has a fixed capacity and a defined overflow behavior.
- A loading transition must fit its peak budget, not only its final state.
- Assets are loaded for their current screen, map, or owner and released at a
  clear lifetime boundary.
- Main-RAM and video-RAM totals need tests whenever a new asset class lands.
- Do not trade large lookup tables or caches for small CPU wins without
  measuring the memory cost.

## Rendering

- Keep game renderers independent from a CPU framebuffer. They use a small
  renderer API whose implementation is selected when building the target.
- The initial renderer operations are clear, rectangle fill, opaque sprite,
  masked sprite, translucent sprite, and dirty-region restoration. Add another
  operation only when recovered game behavior needs it.
- Desktop can execute those operations in software directly into RGB555.
  A future PS1 adapter can submit native GPU sprites and primitives instead.
- Backend choice is compile-time. Do not put virtual dispatch, callbacks, or
  target checks in pixel and sprite hot paths.
- Runtime images stay packed. Do not convert formats, scale artwork, or
  allocate memory during an ordinary rendered frame. A bounded animation may
  decode its current packed frame into fixed scratch memory when retaining all
  frames would break the RAM limit; document and measure that work.
- Cull invisible objects before drawing them.
- Dirty rectangles are useful for mostly static screens such as the title and
  inventory. Do not force them onto a scrolling world when most of the view is
  changing anyway.
- A dirty-rectangle list is fixed-capacity. It merges overlapping damage and
  falls back to a full redraw when it cannot represent the damage safely.
- Avoid full framebuffer clears and copies unless the whole scene truly needs
  to change.

## Game and asset work

- Use retail data and scripts instead of hardcoding content already present in
  the game files.
- Verify behavior against the retail executable. SF_EXE, reconstructed DLLs,
  and older research can point us in the right direction, but they do not
  overrule retail behavior.
- Decode or prepare expensive data while loading, not during gameplay.
- Prefer integer and fixed-point math in recurring game and render work.
- File access, logging, decompression, and save work do not belong in a frame
  hot path.
- Optimize measured work, but reject obviously unbounded algorithms before a
  profiler is needed.

## Keeping this honest

Every completed slice should answer four questions:

1. What is its steady and peak main-RAM cost?
2. What is its steady and peak video-RAM cost?
3. What work does it add to a normal 30 Hz update and rendered frame?
4. Does it preserve the layer and platform boundaries above?

If we cannot answer those yet, the slice is not finished.

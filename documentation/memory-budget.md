# Memory budget

The long-term low-memory target is a PlayStation 2 build. Its 32 MiB of main
RAM must hold more than decoded game assets: executable code, stacks, allocator
bookkeeping, platform state, and audio all need room too. A sensible working
target is therefore roughly 24–26 MiB of tracked `TOTAL RAM`, not merely a
`GAME` reading below 32 MiB. We can adjust that target once a real PS2 host can
measure the remaining runtime overhead.

## Where the memory goes

These figures came from the portable resource accounting and focused decoding
of the retail files used around Remote Town. Female and male player graphics
are alternatives; they are not loaded together. The table is a list of the
largest known allocations, so its rows should not be added to produce the
profiler total.

| Resource | Before | Current state or next step |
|---|---:|---|
| Remote Town exploration mask | 10.55 MiB | 0.33 MiB bit mask; completed |
| Female player graphics | 18.97 MiB | 6.86 MiB with the new-character body and armour; completed |
| Male player graphics | 24.93 MiB | 8.14 MiB with the new-character body and armour; completed |
| Common fonts and loading art | 5.20 MiB | 0.06 MiB in steady English gameplay; loading art lives only on the loading page |
| All inventory sheets | 2.59 MiB | 0.37 MiB for the starter belt while panels are closed; only visible item patterns are decoded |
| Gameplay interface artwork | 2.09 MiB | 0.17 MiB with panels closed; each open panel decodes only the Status patterns it draws |
| Remote Town map patterns | 2.47 MiB | 1.29 MiB after selecting patterns from the map's own ground and object references |
| Software framebuffer | 1.17 MiB | Consider a general RGB565 surface later |

The first completed change replaced the 1920×1440 RGBA exploration bitmap with
one packed bit per map pixel. It retains the same reveal rectangles and map
clipping while reducing the allocation from 11,059,200 to 345,600 bytes, a
10.22 MiB saving.

## Order of work

Player graphics now keep the complete CAF animation and NJP pattern metadata,
but decode bitmap payloads only for the base body and currently equipped visual
layers. Equipment changes rebuild that selection at a predictable UI boundary.
The previous selection is released before replacement bitmaps are allocated, so
an equipment change does not briefly retain two complete player payloads.
With the new-character leather armour, this reduces the female resource by
12.10 MiB and the male resource by 16.79 MiB. Other equipment combinations use
different amounts, so the current figures are useful baselines rather than hard
caps. Selected NJP loading also streams individual compressed blocks from disk;
it does not temporarily copy the complete 9–12 MiB source file into memory.

The artwork lifetime pass keeps patterns zero and two of each English font.
They contain the Latin glyphs plus the Shift-JIS spacing and bracket characters
used by enemy nameplates and quest notices in the English retail interface.
This reduces either font from 2,386,357 bytes to 62,161 bytes without changing
its glyph pixels. Font00 exists only in character selection. Font01 exists only
in gameplay. The 680,281-byte loading page is released at the exact handoff to
the world.

Gameplay starts with only Bar, StatusIcon, MagicIcon, and MagicBarIcon. The
Status panel uses 0.21 MiB of its 1.27 MiB source sheet, Inventory uses 0.33
MiB, and Settings uses 0.09 MiB. Selections are combined when independent left
and right panels are open. MapIcon and the 0.65 MiB Card sheet still follow
their panels. Inventory artwork follows the always-visible belt plus the exact
items in whichever backpack, equipment, warehouse, or vendor container is
visible. Synchronization occurs at the 30 Hz UI/state boundary and performs no
file access while that state is unchanged; the render loop only reads prepared
resources.

Map pattern selection is driven by each loaded GND and OBL rather than a list
of known scenarios. Ground references select their exact patterns, and object
references select the matching normal and shadow patterns. The same loading
path therefore applies to every retail map and future data without per-map
rules.

A deterministic Remote Town starter test now measures 23,364,669 bytes
(22.28 MiB) for tracked game resources and the software framebuffer with all
panels closed. With the current roughly 0.39 MiB decoded-audio baseline, this
puts the representative tracked total around 22.67 MiB. Heavier equipment,
open panels, actors, and effects still need their own representative budgets.

Ground-item resources now retain only IDs referenced by the newly active
scenario after travel. Transient combat, script, mine, and miss-effect caches
are released at the same boundary, while resources owned by persistent spells
remain alive until those systems release them. This keeps the policy in the
shared world/resource layer and avoids decompression work during ordinary
frames.

The next memory target is the transition peak. Failure-safe map preparation
still briefly holds both the old and replacement scenarios, which can exceed a
32 MiB machine even when steady gameplay fits.

Memory work must remain portable and fidelity-safe. Resource budgets and cache
lifetimes belong in shared resource code, never in target adapters. Streaming
should happen during loading screens, equipment changes, or other predictable
boundaries rather than introducing decompression stalls in the draw loop.

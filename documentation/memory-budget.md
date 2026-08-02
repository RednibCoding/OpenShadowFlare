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
| Female player graphics | 18.92 MiB | Decode only active body and equipment layers |
| Male player graphics | 24.88 MiB | Decode only active body and equipment layers |
| Common fonts and loading art | 5.20 MiB | Load glyphs and loading art on demand |
| All inventory sheets | 2.59 MiB | Load only the groups currently needed |
| Gameplay interface artwork | 2.09 MiB | Scope optional panels such as Card artwork |
| Remote Town map patterns | 2.47 MiB | Already compact; stream only if still needed |
| Software framebuffer | 1.17 MiB | Consider a general RGB565 surface later |

The first completed change replaced the 1920×1440 RGBA exploration bitmap with
one packed bit per map pixel. It retains the same reveal rectangles and map
clipping while reducing the allocation from 11,059,200 to 345,600 bytes, a
10.22 MiB saving.

## Order of work

The next major target is player graphics. `Animation00.Njp` currently expands
every body, weapon, shield, and armour bitmap even though the CAF renderer only
enables the base body and equipped layers. Keeping CAF and pattern metadata but
loading bitmap payloads for enabled layers should recover roughly another
12–17 MiB, depending on gender and equipment.

After that, make inventory groups and optional interface artwork lazy, then
bound scenario-owned actor, item, and effect caches. Map transitions also need
to release the old scenario before allocating the complete replacement; the
current failure-safe preparation briefly holds both maps and would exceed a
32 MiB machine even if steady gameplay fits.

Memory work must remain portable and fidelity-safe. Resource budgets and cache
lifetimes belong in shared resource code, never in target adapters. Streaming
should happen during loading screens, equipment changes, or other predictable
boundaries rather than introducing decompression stalls in the draw loop.

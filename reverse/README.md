# Reverse-engineering workspace

This is the shared home for the projects, notes, and older experiments we use
while figuring out how ShadowFlare works. Keeping them together makes it
easier to find the evidence behind a reconstruction without mixing analysis
files into the portable game source.

## What's here

| Folder | Purpose |
| --- | --- |
| `shadowflare-exe/` | Curated function and global maps for the retail executable, plus the confidence labels used by those maps. |
| `ghidra/` | Raw Ghidra decompiler output for the executable and original DLLs. |
| `radare2/` | The radare2 project and its readable annotation script. |
| `remina/` | The Remina project covering the executable and all fourteen DLLs. |
| `references/` | Older or external research that can offer useful leads but is not automatically considered faithful. |

The readable analysis in [`documentation/exe-analysis.md`](../documentation/exe-analysis.md)
is still the best place for findings that have been understood well enough to
explain. The CSV maps here connect those findings and the portable code back to
retail addresses.

## How to use this material

The retail game is the source of truth. The tested DLL reconstructions under
`src/reconstructed/` are also a strong reference for behavior that originally
lived in those DLLs. Decompiler output, tool annotations, and historical
projects are clues: useful ones, but still clues.

When a reference appears to have solved something:

1. use it to find the related data, script command, or retail function;
2. check the behavior against the retail executable and original game data;
3. record the evidence in the executable maps or documentation;
4. implement the understood behavior cleanly in the portable architecture.

This matters most for the older projects under `references/`. They can save a
lot of investigation time, but they also contain guesses, shortcuts, and bugs.
Code should not be copied into `src/SF_EXE/` merely because it produces a
similar result.

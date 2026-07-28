# OpenShadowFlare reconstruction roadmap

This project is an ABI-compatible reconstruction of ShadowFlare's fourteen
support DLLs. “Local” means that an export is implemented in this repository;
it does not by itself mean that every behavior has been proven faithful.

The machine-readable source of truth is
[`fidelity/inventory.json`](fidelity/inventory.json). Run
`python3 tools/verify_fidelity.py` after building to verify every export name,
ordinal, forwarder count, and standalone claim.

## Current status

| DLL | Exports | Original forwards | Internal original paths | Status |
|---|---:|---:|---:|---|
| RK_FUNCTION | 43 | 0 | 0 | LZ and utility paths verified; uncommon Huffman/UI APIs remain |
| RKC_FILE | 10 | 0 | 0 | Differentially verified |
| RKC_MEMORY | 9 | 0 | 0 | Differentially verified |
| RKC_WINDOW | 7 | 0 | 0 | Core object behavior differentially verified |
| RKC_DIB | 42 | 0 | 0 | Core and lookup tables verified; expand drawing coverage |
| RKC_DSOUND | 43 | 0 | 0 | Local replacement backend; audio timing needs validation |
| RKC_FONTMAKER | 13 | 0 | 0 | Standalone; glyph and complete NJP output verified |
| RKC_RPG_TABLE | 25 | 0 | 0 | Standalone; real game database verified |
| RKC_UPDIB | 81 | 0 | 0 | Standalone; NJP/SDW decode and sprite rendering verified on retail assets |
| RKC_DBFCONTROL | 41 | 0 | 0 | Standalone; differential and render-loop verified |
| RKC_RPGSCRN | 185 | 0 | 0 | Standalone; CAF, OBL, compressed GND, ownership, sorting, and packet paths verified |
| RKC_RPG_SCRIPT | 82 | 0 | 0 | Standalone retail binary/runtime path; text compiler remains |
| RKC_RPG_AICONTROL | 51 | 0 | 0 | Standalone; real AI database verified byte-for-byte |
| RKC_NETWORK | 131 | 0 | 0 | Standalone; lifecycle, queues, socket threads, and live transport verified |

Total ABI surface: 763 exports.
Current external forwarders: 0. All fourteen DLLs are standalone.

## Work order

1. Keep the export/ordinal verifier green for every change.
2. Keep expanding differential probes for RK_FUNCTION, DIB, DSOUND, UPDIB,
   RPGSCRN, and authenticated NETWORK paths.
3. Extend DBFCONTROL validation to its fullscreen DirectDraw path.
4. Finish SCRIPT's unused developer text compiler.
5. Begin reconstructing `ShadowFlare.exe`, using the retail executable as a
   behavioral oracle and keeping the completed DLL layer stable.

## Required checks

```bash
./tests/run.sh
./tests/run.sh --wine
```

The Wine suite works in isolated temporary copies. It does not overwrite the
preserved game installation.

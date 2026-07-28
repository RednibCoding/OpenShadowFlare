# Fidelity inventory

`inventory.json` is the conservative source of truth for reconstruction status.
An export is not considered faithful merely because it is locally bound or happens
not to be used by the current executable.

The inventory is checked by `tools/verify_fidelity.py`, which also verifies:

- every original export name and ordinal against the rebuilt DLL;
- the export totals and direct `o_*.dll` forward counts recorded here;
- source-level references to original DLLs;
- the rule that a DLL marked standalone cannot contain an original-DLL bridge.

After deployment, the verifier and differential suite automatically use the
preserved `o_<name>.dll` copies as their original-side oracle.

Run the complete static verification with:

```bash
./src/build.sh
python3 tools/verify_fidelity.py
```

Behavioral equivalence is tracked separately by the differential probes in
`tests/differential`. A DLL should only progress to `verified` after both its
static ABI checks and relevant behavioral comparisons pass.

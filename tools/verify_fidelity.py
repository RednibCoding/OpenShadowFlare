#!/usr/bin/env python3
"""Verify reconstructed DLL ABI parity and fidelity-inventory consistency."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

from pe_exports import read_exports


ROOT = Path(__file__).resolve().parents[1]
ORIGINAL_DIR = ROOT / "tmp" / "ShadowFlare"
BUILD_DIR = ROOT / "src" / "build-win32"
INVENTORY_PATH = ROOT / "fidelity" / "inventory.json"
ORIGINAL_DLL_PATTERN = re.compile(r'"(o_[A-Za-z0-9_]+\.dll)"', re.IGNORECASE)


def fail(message: str, failures: list[str]) -> None:
    failures.append(message)
    print(f"FAIL: {message}")


def main() -> int:
    inventory = json.loads(INVENTORY_PATH.read_text(encoding="utf-8"))
    failures: list[str] = []

    for dll_name, record in inventory["dlls"].items():
        filename = f"{dll_name}.dll"
        original_path = ORIGINAL_DIR / filename
        preserved_original = ORIGINAL_DIR / f"o_{filename}"
        if preserved_original.is_file():
            original_path = preserved_original
        rebuilt_path = BUILD_DIR / filename
        source_path = ROOT / "src" / dll_name / "src" / "core.cpp"

        if not original_path.is_file():
            fail(f"{filename}: original DLL is missing", failures)
            continue
        if not rebuilt_path.is_file():
            fail(f"{filename}: rebuilt DLL is missing; run ./src/build.sh", failures)
            continue

        original_exports = read_exports(original_path)
        rebuilt_exports = read_exports(rebuilt_path)
        original_abi = [(item.ordinal, item.name) for item in original_exports]
        rebuilt_abi = [(item.ordinal, item.name) for item in rebuilt_exports]
        if original_abi != rebuilt_abi:
            fail(f"{filename}: export names or ordinals differ from the original", failures)

        if len(original_exports) != record["exports"]:
            fail(
                f"{filename}: inventory records {record['exports']} exports, "
                f"original has {len(original_exports)}",
                failures,
            )

        forward_count = sum(
            1
            for item in rebuilt_exports
            if item.forwarder and item.forwarder.lower().startswith("o_")
        )
        if forward_count != record["external_forwards"]:
            fail(
                f"{filename}: inventory records {record['external_forwards']} external "
                f"forwards, rebuilt DLL has {forward_count}",
                failures,
            )

        source = source_path.read_text(encoding="utf-8")
        original_references = sorted(set(ORIGINAL_DLL_PATTERN.findall(source)))
        if record["standalone"] and (forward_count or original_references):
            fail(
                f"{filename}: marked standalone but references original DLLs: "
                f"{', '.join(original_references)}",
                failures,
            )

        state = "standalone" if record["standalone"] else "hybrid"
        print(
            f"PASS: {filename:<24} exports={len(rebuilt_exports):3d} "
            f"external={forward_count:3d} source-originals={len(original_references):2d} "
            f"{state} fidelity={record['fidelity']}"
        )

    if failures:
        print(f"\n{len(failures)} fidelity verification failure(s).", file=sys.stderr)
        return 1
    print("\nStatic fidelity verification passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

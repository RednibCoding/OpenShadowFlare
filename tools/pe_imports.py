#!/usr/bin/env python3
"""Read PE32/PE32+ imports using only the Python standard library."""

from __future__ import annotations

import argparse
import json
import struct
from collections import Counter
from dataclasses import asdict, dataclass
from pathlib import Path


@dataclass(frozen=True)
class Import:
    dll: str
    name: str | None
    ordinal: int | None


class PEFormatError(ValueError):
    pass


def _u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def _u64(data: bytes, offset: int) -> int:
    return struct.unpack_from("<Q", data, offset)[0]


def _cstring(data: bytes, offset: int) -> str:
    end = data.find(b"\0", offset)
    if end < 0:
        raise PEFormatError("unterminated PE string")
    return data[offset:end].decode("ascii")


def read_imports(path: str | Path) -> list[Import]:
    image = Path(path)
    data = image.read_bytes()
    if data[:2] != b"MZ":
        raise PEFormatError(f"{image}: missing DOS signature")
    pe_offset = _u32(data, 0x3C)
    if data[pe_offset:pe_offset + 4] != b"PE\0\0":
        raise PEFormatError(f"{image}: missing PE signature")

    coff = pe_offset + 4
    section_count = _u16(data, coff + 2)
    optional_size = _u16(data, coff + 16)
    optional = coff + 20
    magic = _u16(data, optional)
    if magic == 0x10B:
        directory_base = optional + 96
        pointer_size = 4
        ordinal_mask = 0x80000000
    elif magic == 0x20B:
        directory_base = optional + 112
        pointer_size = 8
        ordinal_mask = 0x8000000000000000
    else:
        raise PEFormatError(
            f"{image}: unsupported optional-header magic {magic:#x}")

    import_rva = _u32(data, directory_base + 8)
    if import_rva == 0:
        return []

    sections_offset = optional + optional_size
    sections: list[tuple[int, int, int, int]] = []
    for index in range(section_count):
        section = sections_offset + index * 40
        virtual_size = _u32(data, section + 8)
        virtual_address = _u32(data, section + 12)
        raw_size = _u32(data, section + 16)
        raw_offset = _u32(data, section + 20)
        sections.append(
            (virtual_address, max(virtual_size, raw_size), raw_offset, raw_size))

    def rva_to_offset(rva: int) -> int:
        for virtual_address, span, raw_offset, raw_size in sections:
            if virtual_address <= rva < virtual_address + span:
                delta = rva - virtual_address
                if delta >= raw_size:
                    raise PEFormatError(
                        f"{image}: RVA {rva:#x} has no raw backing")
                return raw_offset + delta
        raise PEFormatError(f"{image}: unmapped RVA {rva:#x}")

    imports: list[Import] = []
    descriptor = rva_to_offset(import_rva)
    while True:
        original_thunk, timestamp, chain, name_rva, first_thunk = (
            struct.unpack_from("<IIIII", data, descriptor))
        if not any((original_thunk, timestamp, chain, name_rva, first_thunk)):
            break
        dll = _cstring(data, rva_to_offset(name_rva))
        thunk_rva = original_thunk or first_thunk
        thunk = rva_to_offset(thunk_rva)
        while True:
            value = _u32(data, thunk) if pointer_size == 4 else _u64(data, thunk)
            if value == 0:
                break
            if value & ordinal_mask:
                imports.append(
                    Import(dll=dll, name=None, ordinal=value & 0xFFFF))
            else:
                name_offset = rva_to_offset(value)
                imports.append(
                    Import(
                        dll=dll,
                        name=_cstring(data, name_offset + 2),
                        ordinal=None,
                    )
                )
            thunk += pointer_size
        descriptor += 20
    return imports


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--summary", action="store_true")
    args = parser.parse_args()
    imports = read_imports(args.image)
    if args.json:
        print(json.dumps([asdict(item) for item in imports], indent=2))
    elif args.summary:
        for dll, count in sorted(Counter(item.dll for item in imports).items()):
            print(f"{count:4d} {dll}")
    else:
        for item in imports:
            symbol = item.name if item.name is not None else f"ordinal {item.ordinal}"
            print(f"{item.dll:<28} {symbol}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Read named exports from a PE32/PE32+ image using only the standard library."""

from __future__ import annotations

import argparse
import json
import struct
from dataclasses import asdict, dataclass
from pathlib import Path


@dataclass(frozen=True)
class Export:
    ordinal: int
    name: str
    forwarder: str | None


class PEFormatError(ValueError):
    pass


def _u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def _u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def _cstring(data: bytes, offset: int) -> str:
    end = data.find(b"\0", offset)
    if end < 0:
        raise PEFormatError("unterminated PE string")
    return data[offset:end].decode("ascii")


def read_exports(path: str | Path) -> list[Export]:
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
    elif magic == 0x20B:
        directory_base = optional + 112
    else:
        raise PEFormatError(f"{image}: unsupported optional-header magic {magic:#x}")

    export_rva = _u32(data, directory_base)
    export_size = _u32(data, directory_base + 4)
    if export_rva == 0:
        return []

    sections_offset = optional + optional_size
    sections: list[tuple[int, int, int, int]] = []
    for index in range(section_count):
        section = sections_offset + index * 40
        virtual_size = _u32(data, section + 8)
        virtual_address = _u32(data, section + 12)
        raw_size = _u32(data, section + 16)
        raw_offset = _u32(data, section + 20)
        sections.append((virtual_address, max(virtual_size, raw_size), raw_offset, raw_size))

    def rva_to_offset(rva: int) -> int:
        for virtual_address, span, raw_offset, raw_size in sections:
            if virtual_address <= rva < virtual_address + span:
                delta = rva - virtual_address
                if delta >= raw_size:
                    raise PEFormatError(f"{image}: RVA {rva:#x} has no raw backing")
                return raw_offset + delta
        raise PEFormatError(f"{image}: unmapped RVA {rva:#x}")

    export_offset = rva_to_offset(export_rva)
    (
        _characteristics,
        _timestamp,
        _major,
        _minor,
        _dll_name,
        ordinal_base,
        function_count,
        name_count,
        functions_rva,
        names_rva,
        ordinals_rva,
    ) = struct.unpack_from("<IIHHIIIIIII", data, export_offset)

    functions_offset = rva_to_offset(functions_rva)
    names_offset = rva_to_offset(names_rva)
    ordinals_offset = rva_to_offset(ordinals_rva)
    exports: list[Export] = []

    for index in range(name_count):
        name_rva = _u32(data, names_offset + index * 4)
        function_index = _u16(data, ordinals_offset + index * 2)
        if function_index >= function_count:
            raise PEFormatError(f"{image}: invalid export function index")
        function_rva = _u32(data, functions_offset + function_index * 4)
        forwarder = None
        if export_rva <= function_rva < export_rva + export_size:
            forwarder = _cstring(data, rva_to_offset(function_rva))
        exports.append(
            Export(
                ordinal=ordinal_base + function_index,
                name=_cstring(data, rva_to_offset(name_rva)),
                forwarder=forwarder,
            )
        )

    return sorted(exports, key=lambda item: item.ordinal)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    exports = read_exports(args.image)
    if args.json:
        print(json.dumps([asdict(item) for item in exports], indent=2))
    else:
        for item in exports:
            suffix = f" -> {item.forwarder}" if item.forwarder else ""
            print(f"{item.ordinal:4d} {item.name}{suffix}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Decode Mojang's uncompressed Bedrock .brarchive container."""
from __future__ import annotations

import argparse
import pathlib
import struct
import sys

MAGIC = 0x267052A0B125277D
HEADER_SIZE = 16
ENTRY_SIZE = 256


def decode(source: pathlib.Path, output: pathlib.Path) -> list[tuple[str, int]]:
    blob = source.read_bytes()
    if len(blob) < HEADER_SIZE:
        raise ValueError("file is shorter than the 16-byte header")

    magic, count, version = struct.unpack_from("<QII", blob, 0)
    if magic != MAGIC:
        raise ValueError(f"bad magic 0x{magic:016x}")
    if version != 1:
        raise ValueError(f"unsupported brarchive version {version}")

    table_end = HEADER_SIZE + count * ENTRY_SIZE
    if table_end > len(blob):
        raise ValueError("entry table extends beyond end of file")

    content = memoryview(blob)[table_end:]
    output.mkdir(parents=True, exist_ok=True)
    entries: list[tuple[str, int]] = []

    for index in range(count):
        base = HEADER_SIZE + index * ENTRY_SIZE
        name_len = blob[base]
        if name_len > 247:
            raise ValueError(f"entry {index} has invalid name length {name_len}")
        name = blob[base + 1:base + 1 + name_len].decode("utf-8")
        offset, size = struct.unpack_from("<II", blob, base + 248)
        if offset + size > len(content):
            raise ValueError(f"entry {name!r} extends beyond content section")

        rel = pathlib.PurePosixPath(name)
        if rel.is_absolute() or ".." in rel.parts:
            raise ValueError(f"unsafe entry path {name!r}")

        target = output.joinpath(*rel.parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content[offset:offset + size])
        entries.append((name, size))

    return entries


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("archive", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()

    entries = decode(args.archive, args.output)
    print(f"{args.archive}: decoded {len(entries)} entries to {args.output}")
    for name, size in entries:
        print(f"{size:>9}  {name}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"brarchive decode failed: {exc}", file=sys.stderr)
        raise SystemExit(1)

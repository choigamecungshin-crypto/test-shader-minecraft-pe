#!/usr/bin/env python3
"""Encode a directory into Mojang's uncompressed Bedrock .brarchive format."""
from __future__ import annotations

import argparse
import pathlib
import struct
import sys

MAGIC = 0x267052A0B125277D
HEADER = 16
ENTRY_SIZE = 256


def encode(source: pathlib.Path, output: pathlib.Path) -> int:
    files = sorted(p for p in source.rglob("*") if p.is_file())
    descriptors = bytearray()
    contents = bytearray()

    for path in files:
        name = path.relative_to(source).as_posix()
        encoded = name.encode("utf-8")
        if len(encoded) > 247:
            raise ValueError(f"entry path too long: {name}")
        data = path.read_bytes()
        if data:
            offset = len(contents)
            contents.extend(data)
        else:
            offset = 0
        descriptor = bytearray(ENTRY_SIZE)
        descriptor[0] = len(encoded)
        descriptor[1:1 + len(encoded)] = encoded
        struct.pack_into("<II", descriptor, 248, offset, len(data))
        descriptors.extend(descriptor)

    header = struct.pack("<QII", MAGIC, len(files), 1)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(header + descriptors + contents)
    return len(files)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    args = parser.parse_args()
    count = encode(args.source, args.output)
    print(f"encoded {count} entries to {args.output} ({args.output.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"brarchive encode failed: {exc}", file=sys.stderr)
        raise SystemExit(1)

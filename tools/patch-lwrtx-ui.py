#!/usr/bin/env python3
"""Patch the Bedrock video-mode label to expose the LWRTX experimental alias.

The current Minecraft native GraphicsMode enum has no LWRTX binding. This
patch deliberately reuses the existing Deferred binding and changes only its
visible label. Selecting LWRTX therefore selects the Deferred renderer while
the separately injected LWRTX GLES hook transforms compatible chunk shaders.
It does not add a new native GraphicsMode enum value.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import sys
import tempfile

from brarchive_pack import encode
from brarchive_unpack import decode

NEEDLE = '"$radio_label_text":"options.graphicsMode.deferred"'
REPLACEMENT = '"$radio_label_text":"LWRTX"'
TARGET = "general_section.json"


def patch_archive(source: pathlib.Path, output: pathlib.Path) -> None:
    with tempfile.TemporaryDirectory(prefix="lwrtx-ui-") as tmp:
        root = pathlib.Path(tmp) / "unpacked"
        entries = decode(source, root)
        target = root / TARGET
        if not target.is_file():
            raise FileNotFoundError(f"{TARGET} missing from settings UI archive")

        text = target.read_text(encoding="utf-8")
        if text.count(NEEDLE) != 1:
            raise ValueError(
                f"expected exactly one Deferred graphics radio label, found {text.count(NEEDLE)}"
            )

        parsed = json.loads(text)
        text = text.replace(NEEDLE, REPLACEMENT, 1)
        json.loads(text)
        target.write_text(text, encoding="utf-8")

        count = encode(root, output)
        if count != len(entries):
            raise ValueError(f"entry count changed: {len(entries)} -> {count}")

        verify = pathlib.Path(tmp) / "verify"
        decode(output, verify)
        verified = (verify / TARGET).read_text(encoding="utf-8")
        if REPLACEMENT not in verified or NEEDLE in verified:
            raise ValueError("rebuilt UI archive did not retain the LWRTX label")

        print(f"Patched {TARGET}: Deferred label -> LWRTX")
        print("Native selection binding retained: #graphics_mode_radio_deferred")
        print(f"Rebuilt archive: {output} ({output.stat().st_size} bytes)")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_archive", type=pathlib.Path)
    parser.add_argument("output_archive", type=pathlib.Path)
    args = parser.parse_args()
    patch_archive(args.input_archive, args.output_archive)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"LWRTX UI patch failed: {exc}", file=sys.stderr)
        raise SystemExit(1)

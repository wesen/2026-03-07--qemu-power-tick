#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import gzip
import json
import pathlib
import re
import sys
from typing import Any

from PIL import Image


META_RE = re.compile(r"^@@FBSHOT\s+(.*)$")
BEGIN_RE = re.compile(r"^@@FBSHOT_BEGIN tag=(\S+)$")
END_RE = re.compile(r"^@@FBSHOT_END tag=(\S+)$")


def parse_meta_fields(payload: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for token in payload.split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        fields[key] = value
    return fields


def decode_rgb565(raw: bytes, width: int, height: int, stride: int) -> Image.Image:
    out = bytearray(width * height * 3)
    row_bytes = width * 2
    for y in range(height):
        row = raw[y * stride : y * stride + row_bytes]
        for x in range(width):
            value = row[x * 2] | (row[x * 2 + 1] << 8)
            r = ((value >> 11) & 0x1F) * 255 // 31
            g = ((value >> 5) & 0x3F) * 255 // 63
            b = (value & 0x1F) * 255 // 31
            idx = (y * width + x) * 3
            out[idx : idx + 3] = bytes((r, g, b))
    return Image.frombytes("RGB", (width, height), bytes(out))


def save_png_variants(raw: bytes, meta: dict[str, Any], output_dir: pathlib.Path, tag: str) -> list[pathlib.Path]:
    width = int(meta["visible_width"])
    height = int(meta["visible_height"])
    stride = int(meta["stride"])
    bpp = int(meta["bpp"])
    outputs: list[pathlib.Path] = []

    if bpp == 32:
        variants = [("bgrx", "BGRX"), ("rgbx", "RGBX"), ("xrgb", "XRGB")]
        for suffix, raw_mode in variants:
            try:
                image = Image.frombuffer("RGB", (width, height), raw, "raw", raw_mode, stride, 1)
            except Exception:
                continue
            path = output_dir / f"fbshot-{tag}-{suffix}.png"
            image.convert("RGB").save(path)
            outputs.append(path)
        return outputs

    if bpp == 16:
        path = output_dir / f"fbshot-{tag}-rgb565.png"
        decode_rgb565(raw, width, height, stride).save(path)
        outputs.append(path)
        return outputs

    return outputs


def extract(serial_log: pathlib.Path, output_dir: pathlib.Path) -> int:
    output_dir.mkdir(parents=True, exist_ok=True)
    tag: str | None = None
    chunks: list[str] = []
    current_meta: dict[str, str] | None = None
    written = 0

    for raw_line in serial_log.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw_line.strip()
        meta_match = META_RE.match(line)
        if meta_match:
            current_meta = parse_meta_fields(meta_match.group(1))
            continue

        begin = BEGIN_RE.match(line)
        if begin:
            tag = begin.group(1)
            chunks = []
            continue

        end = END_RE.match(line)
        if end and tag is not None:
            end_tag = end.group(1)
            if end_tag != tag:
                raise SystemExit(f"mismatched fbshot tags: begin={tag} end={end_tag}")
            if not current_meta or current_meta.get("tag") != tag:
                raise SystemExit(f"missing metadata for fbshot tag={tag}")
            blob = base64.b64decode("".join(chunks))
            raw = gzip.decompress(blob)
            raw_path = output_dir / f"fbshot-{tag}.raw"
            gz_path = output_dir / f"fbshot-{tag}.raw.gz"
            meta_path = output_dir / f"fbshot-{tag}.json"
            expected_bytes = int(current_meta["stride"]) * int(current_meta["visible_height"])
            if len(raw) < expected_bytes:
                raise SystemExit(
                    f"short fbshot payload for tag={tag}: expected at least {expected_bytes} bytes, got {len(raw)}"
                )
            raw_path.write_bytes(raw)
            gz_path.write_bytes(blob)
            meta: dict[str, Any] = dict(current_meta)
            meta["raw_bytes"] = len(raw)
            meta["gzip_bytes"] = len(blob)
            meta["png_variants"] = [str(path.name) for path in save_png_variants(raw, meta, output_dir, tag)]
            meta_path.write_text(json.dumps(meta, indent=2, sort_keys=True) + "\n", encoding="utf-8")
            print(meta_path)
            written += 1
            tag = None
            current_meta = None
            chunks = []
            continue

        if tag is not None:
            chunks.append(line)

    if tag is not None:
        raise SystemExit(f"unterminated fbshot block for tag={tag}")

    return written


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract guest framebuffer snapshots from a serial log")
    parser.add_argument("--serial-log", required=True, type=pathlib.Path)
    parser.add_argument("--output-dir", required=True, type=pathlib.Path)
    args = parser.parse_args()
    count = extract(args.serial_log, args.output_dir)
    if count == 0:
        print("no fbshots found", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

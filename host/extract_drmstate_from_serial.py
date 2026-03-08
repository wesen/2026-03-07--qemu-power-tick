#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import gzip
import json
import pathlib
import re
import sys


META_RE = re.compile(r"^@@DRMSTATE\s+(.*)$")
BEGIN_RE = re.compile(r"^@@DRMSTATE_BEGIN tag=(\S+)$")
END_RE = re.compile(r"^@@DRMSTATE_END tag=(\S+)$")


def parse_meta_fields(payload: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for token in payload.split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        fields[key] = value
    return fields


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
                raise SystemExit(f"mismatched drmstate tags: begin={tag} end={end_tag}")
            if not current_meta or current_meta.get("tag") != tag:
                raise SystemExit(f"missing metadata for drmstate tag={tag}")
            blob = base64.b64decode("".join(chunks))
            text = gzip.decompress(blob).decode("utf-8", errors="replace")
            txt_path = output_dir / f"drmstate-{tag}.txt"
            gz_path = output_dir / f"drmstate-{tag}.txt.gz"
            meta_path = output_dir / f"drmstate-{tag}.json"
            txt_path.write_text(text, encoding="utf-8")
            gz_path.write_bytes(blob)
            meta = dict(current_meta)
            meta["text_bytes"] = len(text.encode("utf-8"))
            meta["gzip_bytes"] = len(blob)
            meta_path.write_text(json.dumps(meta, indent=2, sort_keys=True) + "\n", encoding="utf-8")
            print(txt_path)
            written += 1
            tag = None
            current_meta = None
            chunks = []
            continue

        if tag is not None:
            chunks.append(line)

    if tag is not None:
        raise SystemExit(f"unterminated drmstate block for tag={tag}")

    return written


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract guest DRM debugfs state blocks from a serial log")
    parser.add_argument("--serial-log", required=True, type=pathlib.Path)
    parser.add_argument("--output-dir", required=True, type=pathlib.Path)
    args = parser.parse_args()
    count = extract(args.serial_log, args.output_dir)
    if count == 0:
        print("no drmstate blocks found", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import pathlib
import re
import sys


BEGIN_RE = re.compile(r"^@@GUESTSHOT_BEGIN tag=(\S+)$")
END_RE = re.compile(r"^@@GUESTSHOT_END tag=(\S+)$")


def extract(serial_log: pathlib.Path, output_dir: pathlib.Path) -> int:
    output_dir.mkdir(parents=True, exist_ok=True)
    tag: str | None = None
    chunks: list[str] = []
    written = 0

    for raw_line in serial_log.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw_line.strip()
        begin = BEGIN_RE.match(line)
        if begin:
            tag = begin.group(1)
            chunks = []
            continue

        end = END_RE.match(line)
        if end and tag is not None:
            end_tag = end.group(1)
            if end_tag != tag:
                raise SystemExit(f"mismatched guestshot tags: begin={tag} end={end_tag}")
            data = base64.b64decode("".join(chunks))
            path = output_dir / f"guestshot-{tag}.png"
            path.write_bytes(data)
            print(path)
            written += 1
            tag = None
            chunks = []
            continue

        if tag is not None:
            chunks.append(line)

    if tag is not None:
        raise SystemExit(f"unterminated guestshot block for tag={tag}")

    return written


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract guest-side screenshot PNGs from a serial log")
    parser.add_argument("--serial-log", required=True, type=pathlib.Path)
    parser.add_argument("--output-dir", required=True, type=pathlib.Path)
    args = parser.parse_args()
    count = extract(args.serial_log, args.output_dir)
    if count == 0:
      print("no guestshots found", file=sys.stderr)
      return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

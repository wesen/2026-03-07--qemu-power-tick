#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image, ImageChops


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {Path(sys.argv[0]).name} IMAGE_A IMAGE_B", file=sys.stderr)
        return 1

    a = Image.open(sys.argv[1]).convert("L")
    b = Image.open(sys.argv[2]).convert("L")

    if a.size != b.size:
        print(f"size_mismatch {a.size} {b.size}")
        return 2

    diff = ImageChops.difference(a, b)
    ae = sum(diff.histogram()[1:])
    print(ae)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

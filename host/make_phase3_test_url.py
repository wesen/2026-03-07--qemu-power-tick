#!/usr/bin/env python3
import argparse
import base64


INPUT_CHECK_HTML = """<!doctype html>
<html>
  <body style="margin:0;background:white;font-family:DejaVu Sans, sans-serif">
    <input id="i" autofocus style="display:block;width:95vw;height:120px;font-size:72px;margin:20px auto" value="">
    <button id="b" onclick="document.body.style.background='lime';document.getElementById('s').textContent='clicked'" style="display:block;font-size:64px;margin:20px auto">CLICK</button>
    <div id="s" style="font-size:72px;text-align:center">idle</div>
  </body>
</html>
"""


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Emit deterministic Chromium data: URLs for phase-3 tests")
    parser.add_argument(
        "--mode",
        choices=["input-check"],
        default="input-check",
        help="Which canned page to emit",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.mode != "input-check":
        raise SystemExit(f"unsupported mode: {args.mode}")

    payload = base64.b64encode(INPUT_CHECK_HTML.encode("utf-8")).decode("ascii")
    print(f"data:text/html;base64,{payload}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

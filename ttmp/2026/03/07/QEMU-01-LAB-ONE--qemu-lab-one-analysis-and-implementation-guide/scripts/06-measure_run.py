#!/usr/bin/env python3
import argparse
import json
import pathlib
import re
import sys


METRIC_RE = re.compile(r"@@METRIC name=(?P<name>[a-zA-Z0-9_]+) value_ms=(?P<value>\d+) cycle=(?P<cycle>\d+)")
STATE_RE = re.compile(r"@@LOG kind=state .* state=(?P<state>[A-Z_]+)(?: .*?)?$")


def parse_log(path: pathlib.Path) -> dict:
    metrics: dict[str, list[dict[str, int]]] = {}
    states: list[str] = []

    for line in path.read_text(errors="ignore").splitlines():
        metric_match = METRIC_RE.search(line)
        if metric_match:
            name = metric_match.group("name")
            metrics.setdefault(name, []).append(
                {
                    "cycle": int(metric_match.group("cycle")),
                    "value_ms": int(metric_match.group("value")),
                }
            )
            continue

        state_match = STATE_RE.search(line)
        if state_match:
            states.append(state_match.group("state"))

    summary = {
        "states_seen": states,
        "metrics": metrics,
        "latest": {name: values[-1] for name, values in metrics.items() if values},
    }
    return summary


def main() -> int:
    parser = argparse.ArgumentParser(description="Parse sleepdemo metric lines from a serial log")
    parser.add_argument("--serial-log", required=True)
    parser.add_argument("--json-out")
    args = parser.parse_args()

    serial_log = pathlib.Path(args.serial_log)
    if not serial_log.exists():
        print(f"serial log not found: {serial_log}", file=sys.stderr)
        return 1

    summary = parse_log(serial_log)

    if args.json_out:
        pathlib.Path(args.json_out).write_text(json.dumps(summary, indent=2) + "\n")

    print(json.dumps(summary, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

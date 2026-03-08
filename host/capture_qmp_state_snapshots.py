#!/usr/bin/env python3
import argparse
import json
import pathlib
import time
from typing import Any

from qmp_harness import QMPClient, wait_for_socket


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture selected QMP and HMP state snapshots before and after resume"
    )
    parser.add_argument("--socket", required=True)
    parser.add_argument("--serial-log", required=True)
    parser.add_argument("--results-dir", required=True)
    parser.add_argument("--pre-delay-seconds", type=float, default=9.0)
    parser.add_argument("--post-resume-delay-seconds", type=float, default=1.0)
    parser.add_argument("--resume-timeout-seconds", type=float, default=60.0)
    parser.add_argument(
        "--hmp-command",
        action="append",
        default=[],
        help="Human monitor command to run if human-monitor-command is supported",
    )
    return parser.parse_args()


def wait_for_resume(serial_log: pathlib.Path, timeout: float) -> None:
    deadline = time.time() + timeout
    offset = 0

    while time.time() < deadline:
        if serial_log.exists():
            with serial_log.open("r", errors="ignore") as handle:
                handle.seek(offset)
                chunk = handle.read()
                offset = handle.tell()
            if "state=RESUMED" in chunk:
                return
        time.sleep(0.1)

    raise TimeoutError(f"timed out waiting for resume marker in {serial_log}")


def query_commands(client: QMPClient) -> list[str]:
    response = client.execute("query-commands")
    return sorted(command["name"] for command in response.get("return", []) if "name" in command)


def capture_state(
    client: QMPClient, command_names: list[str], hmp_commands: list[str]
) -> dict[str, Any]:
    state: dict[str, Any] = {}
    state["query-status"] = client.execute("query-status")
    if "query-display-options" in command_names:
        state["query-display-options"] = client.execute("query-display-options")
    if "query-pci" in command_names:
        state["query-pci"] = client.execute("query-pci")
    if "x-query-virtio" in command_names:
        virtio = client.execute("x-query-virtio")
        state["x-query-virtio"] = virtio
        if "return" in virtio and "x-query-virtio-status" in command_names:
            statuses: dict[str, Any] = {}
            for item in virtio["return"]:
                path = item.get("path")
                if not path:
                    continue
                statuses[path] = client.execute("x-query-virtio-status", {"path": path})
            state["x-query-virtio-status"] = statuses

    if "human-monitor-command" in command_names:
        state["human-monitor-command"] = {}
        for command_line in hmp_commands:
            state["human-monitor-command"][command_line] = client.execute(
                "human-monitor-command", {"command-line": command_line}
            )

    return state


def main() -> int:
    args = parse_args()
    socket_path = pathlib.Path(args.socket)
    serial_log = pathlib.Path(args.serial_log)
    results_dir = pathlib.Path(args.results_dir)
    results_dir.mkdir(parents=True, exist_ok=True)

    wait_for_socket(socket_path, 15.0)
    time.sleep(args.pre_delay_seconds)

    client = QMPClient(str(socket_path))
    try:
        client.handshake()
        command_names = query_commands(client)

        summary: dict[str, Any] = {
            "command_names": command_names,
            "hmp_commands": args.hmp_command,
        }

        summary["pre"] = capture_state(client, command_names, args.hmp_command)
        wait_for_resume(serial_log, args.resume_timeout_seconds)
        time.sleep(args.post_resume_delay_seconds)
        summary["post"] = capture_state(client, command_names, args.hmp_command)
    finally:
        client.close()

    output_path = results_dir / "qmp-state-snapshots.json"
    output_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

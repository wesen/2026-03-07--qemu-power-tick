#!/usr/bin/env python3
import argparse
import json
import pathlib
from typing import Any

from qmp_harness import QMPClient, wait_for_socket


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Probe QMP screendump capabilities on the current QEMU build")
    parser.add_argument("--socket", required=True)
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("--output", required=True)
    return parser.parse_args()


def find_schema_entry(schema: list[dict[str, Any]], name: str) -> dict[str, Any] | None:
    for entry in schema:
        if entry.get("meta-type") == "command" and entry.get("name") == name:
            return entry
    return None


def main() -> int:
    args = parse_args()
    socket_path = pathlib.Path(args.socket)
    wait_for_socket(socket_path, args.timeout)

    client = QMPClient(str(socket_path))
    try:
        client.handshake()

        commands_response = client.execute("query-commands")
        commands = commands_response.get("return", [])
        command_names = sorted(command["name"] for command in commands if "name" in command)

        schema_supported = "query-qmp-schema" in command_names
        screendump_schema = None
        interesting_commands = [name for name in command_names if any(token in name for token in ("screen", "display", "console", "vga"))]

        if schema_supported:
            schema_response = client.execute("query-qmp-schema")
            schema = schema_response.get("return", [])
            screendump_schema = find_schema_entry(schema, "screendump")

        summary = {
            "query_commands_supported": True,
            "query_qmp_schema_supported": schema_supported,
            "screendump_command_present": "screendump" in command_names,
            "interesting_commands": interesting_commands,
            "screendump_schema": screendump_schema,
        }
        output_path = pathlib.Path(args.output)
        output_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(summary, indent=2))
        return 0
    finally:
        client.close()


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
import argparse
import json
import socket
from typing import Any


class QMPClient:
    def __init__(self, socket_path: str) -> None:
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(socket_path)
        self.file = self.sock.makefile("rwb", buffering=0)

    def _read_message(self) -> dict[str, Any]:
        while True:
            line = self.file.readline()
            if not line:
                raise RuntimeError("qmp connection closed")
            obj = json.loads(line.decode("utf-8"))
            if "event" in obj:
                continue
            return obj

    def handshake(self) -> None:
        greeting = self._read_message()
        if "QMP" not in greeting:
            raise RuntimeError(f"unexpected greeting: {greeting}")
        response = self.execute("qmp_capabilities")
        if "return" not in response:
            raise RuntimeError(f"failed qmp_capabilities: {response}")

    def execute(self, command: str, arguments: dict[str, Any] | None = None) -> dict[str, Any]:
        payload: dict[str, Any] = {"execute": command}
        if arguments:
            payload["arguments"] = arguments
        self.file.write((json.dumps(payload) + "\n").encode("utf-8"))
        return self._read_message()

    def close(self) -> None:
        self.file.close()
        self.sock.close()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Ad hoc QMP/HMP debug helper used during phase-2 input bring-up")
    parser.add_argument("--socket", required=True)

    subparsers = parser.add_subparsers(dest="command", required=True)

    schema = subparsers.add_parser("schema")
    schema.add_argument("--name", action="append", required=True, help="Type or command name to print; may be repeated")

    hmp = subparsers.add_parser("hmp")
    hmp.add_argument("--cmd", required=True, help="Human monitor command, for example: mouse_set 6")

    raw = subparsers.add_parser("raw")
    raw.add_argument("--execute", required=True, help="QMP execute command name")
    raw.add_argument("--arguments", default="{}", help="JSON object for QMP arguments")

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    client = QMPClient(args.socket)
    try:
        client.handshake()

        if args.command == "schema":
            response = client.execute("query-qmp-schema")
            items = response.get("return", [])
            wanted = set(args.name)
            matched = [item for item in items if item.get("name") in wanted]
            print(json.dumps(matched, indent=2))
        elif args.command == "hmp":
            response = client.execute("human-monitor-command", {"command-line": args.cmd})
            print(json.dumps(response, indent=2))
        elif args.command == "raw":
            response = client.execute(args.execute, json.loads(args.arguments))
            print(json.dumps(response, indent=2))
        else:
            raise RuntimeError(f"unsupported command: {args.command}")
        return 0
    finally:
        client.close()


if __name__ == "__main__":
    raise SystemExit(main())

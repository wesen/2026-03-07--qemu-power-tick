#!/usr/bin/env python3
import argparse
import json
import pathlib
import socket
import time
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
            message = json.loads(line.decode("utf-8"))
            if "event" in message:
                continue
            return message

    def handshake(self) -> None:
        greeting = self._read_message()
        if "QMP" not in greeting:
            raise RuntimeError(f"unexpected qmp greeting: {greeting}")
        response = self.execute("qmp_capabilities")
        if "return" not in response:
            raise RuntimeError(f"failed to enable qmp capabilities: {response}")

    def execute(self, command: str, arguments: dict[str, Any] | None = None) -> dict[str, Any]:
        payload: dict[str, Any] = {"execute": command}
        if arguments:
            payload["arguments"] = arguments
        self.file.write((json.dumps(payload) + "\n").encode("utf-8"))
        return self._read_message()

    def close(self) -> None:
        self.file.close()
        self.sock.close()


def wait_for_socket(socket_path: pathlib.Path, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if socket_path.exists():
            return
        time.sleep(0.1)
    raise RuntimeError(f"timed out waiting for qmp socket: {socket_path}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Minimal QMP harness for phase-2 lab automation")
    parser.add_argument("--socket", required=True)
    parser.add_argument("--timeout", type=float, default=10.0)

    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("query-status")

    screendump = subparsers.add_parser("screendump")
    screendump.add_argument("--file", required=True)

    send_key = subparsers.add_parser("send-key")
    send_key.add_argument("--key", action="append", required=True)

    pointer_move = subparsers.add_parser("pointer-move")
    pointer_move.add_argument("--x", type=int, required=True)
    pointer_move.add_argument("--y", type=int, required=True)

    pointer_button = subparsers.add_parser("pointer-button")
    pointer_button.add_argument("--button", choices=["left", "middle", "right"], required=True)
    pointer_button.add_argument("--down", action="store_true")

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    socket_path = pathlib.Path(args.socket)
    wait_for_socket(socket_path, args.timeout)

    client = QMPClient(str(socket_path))
    try:
        client.handshake()

        if args.command == "query-status":
            response = client.execute("query-status")
        elif args.command == "screendump":
            response = client.execute("screendump", {"filename": args.file})
        elif args.command == "send-key":
            response = client.execute(
                "send-key",
                {"keys": [{"type": "qcode", "data": key} for key in args.key]},
            )
        elif args.command == "pointer-move":
            response = client.execute(
                "input-send-event",
                {
                    "events": [
                        {"type": "abs", "data": {"axis": "x", "value": args.x}},
                        {"type": "abs", "data": {"axis": "y", "value": args.y}},
                    ]
                },
            )
        elif args.command == "pointer-button":
            button_map = {
                "left": "left",
                "middle": "middle",
                "right": "right",
            }
            response = client.execute(
                "input-send-event",
                {
                    "events": [
                        {
                            "type": "btn",
                            "data": {"button": button_map[args.button], "down": args.down},
                        }
                    ]
                },
            )
        else:
            raise RuntimeError(f"unsupported command: {args.command}")

        print(json.dumps(response, indent=2))
        return 0
    finally:
        client.close()


if __name__ == "__main__":
    raise SystemExit(main())

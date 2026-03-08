#!/usr/bin/env python3
import argparse
import pathlib
import select
import socket
import sys
import time


def log(message: str) -> None:
    print(f"@@HOST {message}", flush=True)


def wait_for_pattern(path: pathlib.Path, pattern: str, timeout: float) -> None:
    deadline = time.time() + timeout
    offset = 0

    while time.time() < deadline:
        if path.exists():
            with path.open("r", errors="ignore") as handle:
                handle.seek(offset)
                chunk = handle.read()
                offset = handle.tell()
            if pattern in chunk:
                return
        time.sleep(0.1)

    raise TimeoutError(f"timed out waiting for pattern {pattern!r} in {path}")


def serve(host: str, port: int, interval: float, runtime: float) -> int:
    deadline = time.time() + runtime
    clients: list[socket.socket] = []

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((host, port))
        sock.listen()
        sock.setblocking(False)
        log(f"listening host={host} port={port}")

        while time.time() < deadline:
            readable, _, _ = select.select([sock], [], [], interval)
            if readable:
                conn, addr = sock.accept()
                conn.setblocking(False)
                clients.append(conn)
                log(f"accepted peer={addr[0]}:{addr[1]}")

            live_clients: list[socket.socket] = []
            for conn in clients:
                try:
                    conn.sendall(b"tick\n")
                    live_clients.append(conn)
                except OSError:
                    conn.close()
            clients = live_clients

        for conn in clients:
            conn.close()

    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Start a drip server after a serial-log resume marker")
    parser.add_argument("--serial-log", required=True)
    parser.add_argument("--pattern", default="state=RESUMED")
    parser.add_argument("--timeout", type=float, default=30.0)
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=5555)
    parser.add_argument("--interval", type=float, default=0.5)
    parser.add_argument("--runtime", type=float, default=10.0)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    serial_log = pathlib.Path(args.serial_log)

    try:
        wait_for_pattern(serial_log, args.pattern, args.timeout)
    except TimeoutError as exc:
        print(str(exc), file=sys.stderr)
        return 1

    return serve(args.host, args.port, args.interval, args.runtime)


if __name__ == "__main__":
    raise SystemExit(main())

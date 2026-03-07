#!/usr/bin/env python3
import argparse
import select
import socket
import sys
import time


def log(message: str) -> None:
    ts = time.strftime("%Y-%m-%dT%H:%M:%S", time.localtime())
    print(f"@@HOST ts={ts} {message}", flush=True)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Simple host byte source for the QEMU sleep lab")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=5555)
    parser.add_argument("--interval", type=float, default=0.5)
    parser.add_argument("--payload", default="tick\n")
    parser.add_argument("--burst-size", type=int, default=1)
    parser.add_argument("--active-seconds", type=float, default=3.0)
    parser.add_argument("--pause-seconds", type=float, default=8.0)
    parser.add_argument("--disconnect-on-pause", action="store_true")
    parser.add_argument("--stop-after", type=float, default=0.0)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    payload = args.payload.encode("utf-8")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.host, args.port))
    sock.listen(4)
    sock.setblocking(False)

    phase_started = time.monotonic()
    in_pause = False
    pause_disconnect_done = False
    conn = None
    next_send = time.monotonic()
    accepted = 0

    log(f"listening host={args.host} port={args.port}")

    while True:
        now = time.monotonic()
        if args.stop_after and now - phase_started >= args.stop_after:
            log("stop_after reached")
            break

        if not in_pause and now - phase_started >= args.active_seconds:
            in_pause = True
            phase_started = now
            log("phase=pause")
        elif in_pause and now - phase_started >= args.pause_seconds:
            in_pause = False
            pause_disconnect_done = False
            phase_started = now
            log("phase=active")

        readable, _, _ = select.select([sock], [], [], 0.1)
        if readable:
            client, addr = sock.accept()
            client.setblocking(False)
            if conn is not None:
                log("closing previous client for new connection")
                conn.close()
            conn = client
            accepted += 1
            log(f"accepted index={accepted} peer={addr[0]}:{addr[1]}")

        if conn is not None and in_pause and args.disconnect_on_pause and not pause_disconnect_done:
            log("disconnecting client at pause boundary")
            conn.close()
            conn = None
            pause_disconnect_done = True

        if conn is None or in_pause or now < next_send:
            continue

        try:
            for _ in range(args.burst_size):
                conn.sendall(payload)
            log(f"sent bytes={len(payload) * args.burst_size}")
        except OSError as exc:
            log(f"send_failed error={exc}")
            conn.close()
            conn = None

        next_send = now + args.interval

    if conn is not None:
        conn.close()
    sock.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())


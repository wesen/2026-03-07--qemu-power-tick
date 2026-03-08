#!/usr/bin/env python3
import argparse
import json
import pathlib
import signal
import socket
import subprocess
import time
from typing import Any


class QMPClient:
    def __init__(self, socket_path: pathlib.Path) -> None:
        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(str(socket_path))
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


def normalized_to_abs(value: float) -> int:
    value = max(0.0, min(1.0, value))
    return int(round(value * 0x7FFF))


def wait_for_file(path: pathlib.Path, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if path.exists():
            return
        time.sleep(0.1)
    raise RuntimeError(f"timed out waiting for file: {path}")


def wait_for_log(log_path: pathlib.Path, needle: str, timeout: float) -> None:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if log_contains(log_path, needle):
            return
        time.sleep(0.1)
    raise RuntimeError(f"timed out waiting for log marker '{needle}' in {log_path}")


def log_contains(log_path: pathlib.Path, needle: str) -> bool:
    if log_path.exists():
        with log_path.open("r", encoding="utf-8", errors="replace") as handle:
            return needle in handle.read()
    return False


def screendump(client: QMPClient, output_path: pathlib.Path) -> None:
    response = client.execute("screendump", {"filename": str(output_path)})
    if "return" not in response:
        raise RuntimeError(f"screendump failed: {response}")


def send_key(client: QMPClient, key: str) -> None:
    response = client.execute("send-key", {"keys": [{"type": "qcode", "data": key}]})
    if "return" not in response:
        raise RuntimeError(f"send-key failed: {response}")


def pointer_move(client: QMPClient, x: float, y: float) -> None:
    response = client.execute(
        "input-send-event",
        {
            "events": [
                {"type": "abs", "data": {"axis": "x", "value": normalized_to_abs(x)}},
                {"type": "abs", "data": {"axis": "y", "value": normalized_to_abs(y)}},
            ]
        },
    )
    if "return" not in response:
        raise RuntimeError(f"pointer move failed: {response}")


def pointer_click(client: QMPClient) -> None:
    for down in (True, False):
        response = client.execute(
            "input-send-event",
            {"events": [{"type": "btn", "data": {"button": "left", "down": down}}]},
        )
        if "return" not in response:
            raise RuntimeError(f"pointer click failed: {response}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Capture a no-suspend phase-2 checkpoint set")
    parser.add_argument("--kernel", required=True)
    parser.add_argument("--initramfs", required=True)
    parser.add_argument("--results-dir", required=True)
    parser.add_argument("--mode", choices=["interactive", "suspend"], default="interactive")
    parser.add_argument("--runtime-seconds", type=int, default=30)
    parser.add_argument("--timeout", type=float, default=30.0)
    return parser.parse_args()


def run_interactive_capture(
    repo_root: pathlib.Path,
    client: QMPClient,
    serial_log: pathlib.Path,
    results_dir: pathlib.Path,
    timeout: float,
) -> subprocess.Popen[bytes]:
    wait_for_log(serial_log, "[init-phase2] wayland ready", timeout)
    time.sleep(1.0)
    screendump(client, results_dir / "00-boot.ppm")

    wait_for_log(serial_log, "seat-listener-added", timeout)
    time.sleep(0.5)
    screendump(client, results_dir / "01-first-frame.ppm")

    drip_proc = subprocess.Popen(
        [
            "python3",
            str(repo_root / "host" / "drip_server.py"),
            "--host",
            "0.0.0.0",
            "--port",
            "5555",
            "--interval",
            "0.5",
            "--active-seconds",
            "20",
            "--pause-seconds",
            "1",
        ],
        cwd=repo_root,
    )
    wait_for_log(serial_log, "kind=state connected", timeout)
    time.sleep(0.2)
    screendump(client, results_dir / "02-network.ppm")

    send_key(client, "a")
    wait_for_log(serial_log, "kind=input KEY=30 STATE=1", timeout)
    time.sleep(0.2)
    screendump(client, results_dir / "03-keyboard.ppm")

    pointer_seen = False
    for _ in range(5):
        pointer_move(client, 0.5, 0.5)
        time.sleep(0.2)
        pointer_click(client)
        time.sleep(0.5)
        if log_contains(serial_log, "kind=input BUTTON 272 STATE 1"):
            pointer_seen = True
            break
    if not pointer_seen:
        raise RuntimeError(f"timed out waiting for pointer button event in {serial_log}")
    time.sleep(0.2)
    screendump(client, results_dir / "04-pointer.ppm")
    return drip_proc


def run_suspend_capture(client: QMPClient, serial_log: pathlib.Path, results_dir: pathlib.Path, timeout: float) -> None:
    wait_for_log(serial_log, "[init-phase2] wayland ready", timeout)
    time.sleep(1.0)
    screendump(client, results_dir / "00-boot.ppm")

    wait_for_log(serial_log, "seat-listener-added", timeout)
    time.sleep(0.5)
    screendump(client, results_dir / "01-first-frame.ppm")

    time.sleep(1.0)
    screendump(client, results_dir / "05-pre-suspend.ppm")

    wait_for_log(serial_log, "state=RESUMED cycle=1", timeout + 15)
    time.sleep(0.5)
    screendump(client, results_dir / "06-post-resume.ppm")


def main() -> int:
    args = parse_args()
    repo_root = pathlib.Path(__file__).resolve().parents[1]
    results_dir = (repo_root / args.results_dir).resolve()
    results_dir.mkdir(parents=True, exist_ok=True)
    serial_log = results_dir / "guest-serial.log"
    qmp_socket = results_dir / "qmp.sock"

    if args.mode == "interactive":
        append_extra = (
            f"phase2_no_suspend=1 phase2_runtime_seconds={args.runtime_seconds} "
            "phase2_idle_seconds=60 phase2_max_suspend_cycles=0"
        )
    else:
        append_extra = (
            f"phase2_idle_seconds=3 phase2_max_suspend_cycles=1 phase2_wake_seconds=5 "
            f"phase2_pm_test=devices phase2_runtime_seconds={args.runtime_seconds}"
        )

    qemu_cmd = [
        str(repo_root / "guest" / "run-qemu-phase2.sh"),
        "--kernel",
        args.kernel,
        "--initramfs",
        args.initramfs,
        "--results-dir",
        str(results_dir),
        "--append-extra",
        append_extra,
    ]
    qemu_proc = subprocess.Popen(qemu_cmd, cwd=repo_root)
    drip_proc: subprocess.Popen[bytes] | None = None

    try:
        wait_for_file(qmp_socket, args.timeout)
        client = QMPClient(qmp_socket)
        try:
            client.handshake()
            if args.mode == "interactive":
                drip_proc = run_interactive_capture(repo_root, client, serial_log, results_dir, args.timeout)
            else:
                run_suspend_capture(client, serial_log, results_dir, args.timeout)
        finally:
            client.close()

        qemu_proc.wait(timeout=max(args.runtime_seconds + 10, 20))
        return qemu_proc.returncode
    finally:
        if drip_proc and drip_proc.poll() is None:
            drip_proc.send_signal(signal.SIGTERM)
            try:
                drip_proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                drip_proc.kill()
        if qemu_proc.poll() is None:
            qemu_proc.send_signal(signal.SIGINT)
            try:
                qemu_proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                qemu_proc.kill()


if __name__ == "__main__":
    raise SystemExit(main())

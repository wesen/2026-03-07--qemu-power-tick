#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import time


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
QMP_HARNESS = REPO_ROOT / "host" / "qmp_harness.py"


def run_qmp(socket_path: pathlib.Path, *args: str) -> None:
    subprocess.run(
        ["python3", str(QMP_HARNESS), "--socket", str(socket_path), *args],
        check=True,
    )


def screendump(socket_path: pathlib.Path, output: pathlib.Path) -> None:
    run_qmp(socket_path, "screendump", "--file", str(output.with_suffix(".ppm")))
    subprocess.run(
        ["convert", str(output.with_suffix(".ppm")), str(output.with_suffix(".png"))],
        check=True,
    )


def send_text(socket_path: pathlib.Path, text: str, key_delay_ms: int) -> None:
    for ch in text:
        if ch == " ":
            qcode = "spc"
        elif "a" <= ch <= "z" or "0" <= ch <= "9":
            qcode = ch
        else:
            raise SystemExit(f"unsupported text character for qcode injection: {ch!r}")
        run_qmp(socket_path, "send-key", "--key", qcode)
        time.sleep(key_delay_ms / 1000.0)


def click(socket_path: pathlib.Path, x: float, y: float) -> None:
    run_qmp(socket_path, "pointer-move-normalized", "--x", str(x), "--y", str(y))
    run_qmp(socket_path, "pointer-button", "--button", "left", "--down")
    run_qmp(socket_path, "pointer-button", "--button", "left")


def metric_ae(before: pathlib.Path, after: pathlib.Path) -> str:
    result = subprocess.run(
        ["compare", "-metric", "AE", str(before), str(after), "null:"],
        capture_output=True,
        text=True,
        check=False,
    )
    return (result.stderr or result.stdout).strip()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Capture reproducible stage-3 keyboard/pointer checkpoints")
    parser.add_argument("--socket", required=True)
    parser.add_argument("--results-dir", required=True)
    parser.add_argument("--text", default="hello")
    parser.add_argument("--settle-seconds", type=float, default=8.0)
    parser.add_argument("--key-delay-ms", type=int, default=120)
    parser.add_argument("--button-x", type=float, default=0.50)
    parser.add_argument("--button-y", type=float, default=0.25)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    socket_path = pathlib.Path(args.socket)
    results_dir = pathlib.Path(args.results_dir)
    results_dir.mkdir(parents=True, exist_ok=True)

    time.sleep(args.settle_seconds)

    before = results_dir / "00-before-input"
    after_keyboard = results_dir / "01-after-keyboard"
    after_pointer = results_dir / "02-after-pointer"

    screendump(socket_path, before)
    send_text(socket_path, args.text, args.key_delay_ms)
    time.sleep(0.5)
    screendump(socket_path, after_keyboard)
    click(socket_path, args.button_x, args.button_y)
    time.sleep(0.5)
    screendump(socket_path, after_pointer)

    print(f"keyboard_ae={metric_ae(before.with_suffix('.png'), after_keyboard.with_suffix('.png'))}")
    print(f"pointer_ae={metric_ae(after_keyboard.with_suffix('.png'), after_pointer.with_suffix('.png'))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
